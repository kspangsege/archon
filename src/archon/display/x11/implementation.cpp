/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

using Xlib_Time     = Time;
using Xlib_Display  = Display;
using Xlib_Screen   = Screen;
using Xlib_Colormap = Colormap;
using Xlib_Window   = Window;
using Xlib_KeySym   = KeySym;
using Xlib_Cursor   = Cursor;
using Xlib_Drawable = Drawable;

#include <csetjmp>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <bitset>
#include <limits>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>

#include <poll.h>

#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/assert.hpp>
#include <archon/core/iterator.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/weak_ptr.hpp>
#include <archon/core/text.hpp>
#include <archon/core/sys.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/writer.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/integer_buffer_format.hpp>
#include <archon/image/oper.hpp>
#include <archon/display/x11/implementation.hpp>

#ifdef ARCHON_HAVE_GLX
#  include <GL/glx.h>
#endif

#ifdef ARCHON_HAVE_XRENDER
#  include <X11/extensions/Xrender.h>
#endif

#ifdef ARCHON_HAVE_XINPUT2
#  include <X11/extensions/XInput2.h>
#endif

using namespace archon::core;
using namespace archon::util;
using namespace archon::image;
using namespace archon::display;

using Arch_Drawable = archon::display::Drawable;
using Arch_Window   = archon::display::Window;
using Arch_KeySym   = archon::display::KeySym;
using Arch_Cursor   = archon::display::Cursor;


/*

Thread safty issues:

The current implementation is not working as it was designed to do for
multithreaded applications. This is most easily seen by running
../test/multi_threaded -n 32 -s 16,16 and pressing escape repeatedly
while the windows are being opened.

Wrapping all Xlib access in XLockDisplay/XUnlockDisplay does not help,
instead it causes occasional deadlocks.

Although it could be due to an error in the implementation below, it
is probably more likely that it is an issue with the XCB-XLIB bridge.
See http://xcb.freedesktop.org/opengl/. There are indications that it
would work if XCB was bypassed, and Xlib was used in the traditional
way.




Reference map:




Findings from attempt to read out pixels from the frame buffer:

Not possible in a reliable way. When there is no double buffering,
there is only a front buffer. However the contents of this buffer can
be overwritten at any time by the window manager, and the application
appears to have no control over it.

When double buffering is enabled, things improve, but a couple of
problems remain. First, if the window is resized between the time of
rendering and the time of read back, the buffer will have been
cleared, or mostly cleared. For some weird reason, there is a tendency
that some of the last rendered 3-D primitives will remain and
everything else will have been cleared. This is so even when the
remaining primitive is depthwise intermingled with the now vanished
primitives. Alternatively, the remaining primitives are not actually
remaining, but rather redrawn.

On top of that, glReadPixels seems to sometimes fail and write more
data to the callers buffer than expected, causing potential memory
corruption in the application. This seems to happen when the window
is horizontally resized more than once, and possibly only when the
event mask of the window includes StructureNotify.

*/


#define GUARD(state, statement)                 \
    {                                           \
        if ((state).error)                      \
            (state)._throw();                   \
        if (!sigsetjmp((state).jmpbuf, 1)) {    \
            statement;                          \
        }                                       \
        else {                                  \
            (state)._throw();                   \
        }                                       \
    }

#define RAISE(state, msg)                       \
    {                                           \
        (state).error = true;                   \
        (state).message = (msg);                \
        siglongjmp((state).jmpbuf, 1);          \
    }


namespace {

// X11 Library error handling:
//
// It is really nasty that Xlib terminates the entire process if
// the error handler returns. This happens for example if the
// connection to the server is lost.
//
// The workaround is to long-jump back to the application (in this
// case the Archon display library). There is a few problems though,
// first Xlib may leak resources when this is done. This will depend
// on whether Xlib is designed with this posibility in
// mind. However, there are a few other popular programs that does
// the same, one example is the VIM editor. This seems to suggest
// that it is not a completely insane idea.
//
// Another problem is that each and every call to an Xlib function
// must be wrapped in some boilerplate code to set up the long-jump
// stuff. This, in turn, comes with a significant amount of
// nastyness because long-jumping mixes very poorly with C++.
//
// The general rules that one must adhere to, are these:
//
// - Keep the setjump/longjump construction on the following
//   canonical form:
//
//   <pre>
//
//     if (!setjmp(jmpbuf)) { // try
//         // code that may potentially issue a long jump (throw)
//     }
//     else { // catch
//         // code that handles the long jump (exception)
//     }
//
//   </pre>
//
// - Keep the try-scope completely free of declarations of variables
//   whos type have either an explicit or an implied
//   destructor. This also applies to any sub-scope through which a
//   long jump may pass. A sub-scope through which a long jump
//   cannot pass, may safly define stack objects with destructors.
//
// - For each C function call that may lead to a long jump, make
//   sure that the surrounding expression does not involve creation
//   of any temporary objects of a type that has either an explicit
//   or an implied destructor.
//
// - If any of the C function calls that may lead to a long jump
//   calls a C++ function (eg. as a call back function) then make
//   sure that the called C++ function does not issue a long jump or
//   lead to one being issued in a place where stack variables with
//   destructors are "live".
//
// - Never long jump out of a C++ try block or catch block.
//
//
// In the interest of simplicity and consistency, this scheme has
// been semi-automated with a couple of macros (GUARD, RAISE).

struct GuardState {
    sigjmp_buf jmpbuf;
    bool error = false;
    std::string message;
    void _throw() const
    {
        throw BadConnectionException(message);
    }
};


// Used to protect all Xlib access. Unfortunately it seems that
// there are a few interactions between Xlib and OpenGL that are not
// thread-safe by default, and since we do not want to hold a lock
// on this mutex while doing OpenGL rendering, we have aproblem. The
// workaround, that appears to be working well, is to also call
// XInitThreads. See get_implementation_x11.
std::mutex g_xlib_mutex;


class ConnectionImpl;
class WindowImpl;
class EventProcessorImpl;


enum EventType { // For intermediate representation
    ev_mousedown, // Mouse button was pressed down
    ev_mouseup,   // Mouse button was released
    ev_keydown,   // Key on keyboard was pressed down
    ev_keyup,     // Key on keyboard was released
    ev_mousemove, // Mouse motion
    ev_resize,    // Window resize
    ev_mouseover, // Mouse enters window
    ev_mouseout,  // Mouse leaves window
    ev_focus,     // Window gains focus
    ev_blur,      // Window looses focus
    ev_show,      // Window becomes visible
    ev_hide,      // Window becomes completely hidden
    ev_damage,    // A part of the window has been damaged and needs refresh
    ev_close      // Request to close window
};

struct EventAuxTime  { Xlib_Time time; };
struct EventAuxMouse { int x,y; short button; Xlib_Time time; };
struct EventAuxKey   { Xlib_KeySym key; Xlib_Time time; };
struct EventAuxSize  { int width, height; };
struct EventAuxArea  { int x, y, width, height; };

union EventAltUnion {
    EventAuxTime  time;
    EventAuxMouse mouse;
    EventAuxKey   key;
    EventAuxSize  size;
    EventAuxArea  area;
};

struct EventSlot {
    int cookie;
    EventType type;
    EventAltUnion alt;
};




class KeySymMapper {
public:
    Arch_KeySym xlib_to_archon(Xlib_KeySym xlib_key_sym) const
    {
        auto page = m_xlib_to_arch_page_map.find(xlib_key_sym >> 8);
        if (page == m_xlib_to_arch_page_map.end())
            return KeySym_None;
        return page->second[xlib_key_sym & 255];
    }

    Xlib_KeySym archon_to_xlib(Arch_KeySym arch_key_sym) const
    {
        auto page = m_arch_to_xlib_page_map.find(arch_key_sym >> 8);
        if (page == m_arch_to_xlib_page_map.end())
            return NoSymbol;
        return page->second[arch_key_sym & 255];
    }

    KeySymMapper();

private:
    std::map<long, std::unique_ptr<Arch_KeySym[]>> m_xlib_to_arch_page_map;
    std::map<long, std::unique_ptr<Xlib_KeySym[]>> m_arch_to_xlib_page_map;

    /// We use the fact that Xlib KeySyms are organized into relatively few
    /// pages each with 256 entries. since the same is true for Archon KeySyms,
    /// we use the same technique for the reverse mapping from Archon to Xlib.
    void add(Xlib_KeySym xlib_key_sym, Arch_KeySym arch_key_sym)
    {
        if (arch_key_sym == KeySym_None) {
            throw std::runtime_error("Invalid mapping from X KeySym '"+
                                     Text::print(long(xlib_key_sym))+"' to 'None'");
        }
        // Prepare update of map from Xlib to Archon
        Arch_KeySym* arch_sym;
        {
            std::unique_ptr<Arch_KeySym[]>& page = m_xlib_to_arch_page_map[xlib_key_sym >> 8];
            if (!page) {
                // Allocate memory for each new page
                page = std::make_unique<Arch_KeySym[]>(256); // Throws
                std::fill(page.get(), page.get() + 256, KeySym_None);
            }
            arch_sym = &page[xlib_key_sym & 255];
            if (*arch_sym != KeySym_None) {
                throw std::runtime_error("Redefinition of Xlib KeySym '"+
                                         Text::print(long(xlib_key_sym))+"'");
            }
        }
        // Update map from Archon to Xlib
        Xlib_KeySym* xlib_sym;
        {
            std::unique_ptr<Xlib_KeySym[]>& page = m_arch_to_xlib_page_map[arch_key_sym >> 8];
            if (!page) {
                // Allocate memory for each new page
                page = std::make_unique<Xlib_KeySym[]>(256); // Throws
                std::fill(page.get(), page.get() + 256, NoSymbol);
            }
            xlib_sym = &page[arch_key_sym & 255];
            if (*xlib_sym != NoSymbol) {
                throw std::runtime_error("Redefinition of Archon KeySym '"+
                                         Text::print(long(arch_key_sym))+"'");
            }
        }

        *arch_sym = arch_key_sym;
        *xlib_sym = xlib_key_sym;
    }
};




class ImageFormat {
public:
    Image::Ref setup_transcode(XImage& ximg, int width, int height) const;

    static void single_channel_format(const ConnectionImpl* conn, int depth,
                                      ImageFormat& img_fmt);
    static void xvisual_format(const ConnectionImpl* conn, const XVisualInfo* vis_info,
                               ImageFormat &img_fmt);

#ifdef ARCHON_HAVE_XRENDER
    static void xrender_format(const ConnectionImpl* conn, const XRenderPictFormat* xrender_fmt,
                               ImageFormat& img_fmt);
#endif

private:
    IntegerBufferFormat::ConstRef m_buffer_format;
    ColorSpace::ConstRef m_color_space;
    bool m_has_alpha;
    int m_ximg_byte_order;
    int m_ximg_bitmap_unit;
    int m_ximg_bitmap_bit_order;
    int m_ximg_bitmap_pad;
    int m_ximg_depth;

    static void init_format(const ConnectionImpl* conn,
                            IntegerBufferFormat::ChannelLayout& channels,
                            ImageFormat& img_fmt);

    template<class T> static IntegerBufferFormat::Channel make_channel(T mask)
    {
        int offset = find_least_sig_bit(mask);
        ARCHON_ASSERT_1(0 <= offset, "No mask");
        return make_channel(offset, bit_shift_right(mask, offset));
    }

    template<class T> static IntegerBufferFormat::Channel make_channel(int offset, T mask)
    {
        ARCHON_STATIC_ASSERT(~-22073 == 22072, "Need two's complement type");
        int width = find_most_sig_bit(mask) + 1;
        ARCHON_ASSERT_1(width && mask == bit_range<T>(width), "Bad mask");
        return IntegerBufferFormat::Channel(offset, width);
    }
};




class ConnectionImpl: public Connection {
public:
    struct VisualSpec;
    struct ScreenSpec;

    int get_default_screen() const override
    {
        return default_screen;
    }

    int get_default_visual(int scr) const override
    {
        return get_screen(scr).default_visual;
    }

    Arch_Window::Ptr new_window(int width, int height, int scr, int vis) override;

    PixelBuffer::Ptr new_pixel_buffer(int width, int height, int scr, int vis) override;

    bool has_gl_support() const override
    {
        return have_glx;
    }

    int choose_gl_visual(int scr,
                         bool double_buffer,
                         bool stereo,
                         int red, int green,
                         int blue, int alpha,
                         int depth, int stencil,
                         int accum_red, int accum_green,
                         int accum_blue, int accum_alpha) const override;

    Context::Ptr new_gl_context(int scr, int vis, bool direct, Context::Arg share_with) override;

    EventProcessor::Ptr new_event_processor(EventHandler*) override;

    std::unique_ptr<Arch_Cursor> new_cursor(Image::Ref image, int hotspot_x,
                                            int hotspot_y, int scr) override;

    void flush_output() override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(guard_state, XFlush(dpy)); // Throws
    }

    int get_num_screens() const override
    {
        return m_screens.size();
    }

    int get_screen_width(int scr) const override
    {
        return get_screen(scr).width;
    }

    int get_screen_height(int scr) const override
    {
        return get_screen(scr).height;
    }

    double get_horiz_dot_pitch(int scr) const override
    {
        return get_screen(scr).horiz_dot_pitch;
    }

    double get_vert_dot_pitch(int scr) const override
    {
        return get_screen(scr).vert_dot_pitch;
    }

    int get_num_visuals(int scr) const override
    {
        return get_screen(scr).visual_specs.size();
    }

    int get_gl_buf_width(BufferType t, int scr, int vis) const override;

    ConnectionImpl(Xlib_Display*);
    ~ConnectionImpl() noexcept override;

    void register_event_window(Xlib_Window w, EventProcessorImpl* p, int cookie)
    {
        std::lock_guard<std::mutex> lock{m_event_wins_mutex};
        if (!m_event_wins.insert(std::make_pair(w, EventWinProps(p, cookie))).second)
            throw std::runtime_error("Multiple event processor registrations of same window");
    }

    void unregister_event_window(Xlib_Window w) throw()
    {
        std::lock_guard<std::mutex> lock{m_event_wins_mutex};
        m_event_wins.erase(w);
    }

    void receive_events(EventProcessorImpl*, std::chrono::steady_clock::time_point timeout);


    Xlib_Display* const dpy;

    // X11 extensions availble through this connection. Consider them to be
    // immutable.
    bool have_glx = false, have_xrender = false, have_xinput2 = false;

    // Allows construction of SharedPtr's when 'this' is the only
    // thing you've got. Is constant after construction.
    WeakPtr<ConnectionImpl> weak_self;

    Atom atom_del_win, atom_net_wm_state, atom_net_wm_state_fullscreen;


    struct ScreenSpec;

    struct VisualSpec {
        XVisualInfo* info;
#ifdef ARCHON_HAVE_GLX
        bool gl_support;
        int width_red, width_green, width_blue, width_alpha, width_depth, width_stencil,
            width_accum_red, width_accum_green, width_accum_blue, width_accum_alpha;
#endif
        ImageFormat image_format;
        const ScreenSpec* screen;
        int index = -1; // -1 means this visual entry is uninitialized
    };

    struct ScreenSpec {
        XVisualInfo* infos;
        mutable std::vector<VisualSpec> visual_specs;
        Xlib_Window root_win;
        int default_visual;
        int width, height;
        double horiz_dot_pitch, vert_dot_pitch;
        int index = -1; // -1 means this screen entry is uninitialized
    };

    const ScreenSpec& get_screen(int scr) const;
    ScreenSpec& get_screen_nlk(int scr) const;
    void get_screen_helper(int scr, VisualID& default_id, int& num_visuals, XVisualInfo*& infos,
                           Xlib_Window& root, int& width, int& height,
                           int& width_mm, int& height_mm) const;
    const VisualSpec& get_visual(int scr, int vis) const;
#ifdef ARCHON_HAVE_GLX
    void get_visual_helper(XVisualInfo*, int& gl_support, VisualSpec&, bool& error) const;
#endif

    struct ImgFmtDetail {
        int bits_per_pixel;
        int scanline_pad; // Align each scanline at an integer multiple of this number of bits.
        ImgFmtDetail(int bpp, int sp):
            bits_per_pixel(bpp),
            scanline_pad(sp)
        {
        }
    };

    // Provides details on ZPixmap formats for each supported
    // bit-depth. Must be considered constant after connection
    // construction.
    std::map<int, ImgFmtDetail> image_formats;

    int default_screen;   // Consider it to be const after connection construction
    int image_byte_order; // Consider it to be const after connection construction
    int bitmap_bit_order; // Consider it to be const after connection construction
    int bitmap_pad;       // Consider it to be const after connection construction
    int bitmap_unit;      // Consider it to be const after connection construction

private:
    mutable std::mutex m_screens_mutex; // Must be acquired before g_xlib_mutex when both are needed.
    mutable std::vector<ScreenSpec> m_screens; // Elements protected by `m_screens_mutex` (vector is constant length)

    using KeyStates = std::bitset<256>;

    struct EventWinProps {
        const WeakPtr<EventProcessorImpl> proc;
        const int cookie;
        int width = -1, height = -1; // Last seen size. Accessed only by master.
        bool mapped = false, visible = false; // Last seen visibility status. Accessed only by master.
        KeyStates key_states; // One bit per key, 1 means down. Accessed only by master.
        EventWinProps(EventProcessorImpl* p, int cookie);
    };

    struct EventWinPropsRef {
        EventWinProps* const props; // Null if there are no props for the window
        const SharedPtr<EventProcessorImpl> proc;
        EventWinPropsRef(EventWinProps* s, const SharedPtr<EventProcessorImpl>& p):
            props(s),
            proc(p)
        {
        }
    };

    std::mutex m_event_wins_mutex;
    std::map<Xlib_Window, EventWinProps> m_event_wins; // Has an entry for every window currently associated with an event procesor of this connection. Protected by `m_event_wins_mutex`

    int m_conn_file_des = -1; // Accessed only by master

    struct EventAndKeySym {
        XEvent xevent;
        Xlib_KeySym key_sym;
    };

    int read_xevents();
    void read_xevents_helper(bool& no_more, int& used, int& free);
    void put_back_xevent(const EventAndKeySym&);

    // Must have a size of at least two, such that repeating key
    // events can be filtered out.
    static constexpr int s_xevent_buf_size = 64; // 64*sizeof(XEvent) = 12K (approx.)
    const std::unique_ptr<EventAndKeySym[]> m_xevent_buf; // Accessed only by master
    int m_unread_xevents = 0;
    int m_max_xevents_per_read = 0;
    bool m_have_xevent_put_back = false;

    void choose_gl_visual_helper(int scr, int* attribs, VisualID& id, bool& bad) const;

    void new_window_helper(const VisualSpec&, int width, int height, Xlib_Colormap&, Xlib_Window&,
                           bool& has_colmap, bool& has_win);
#ifdef ARCHON_HAVE_GLX
    void new_pixel_buffer_helper(int width, int height, const VisualSpec&,
                                 GLXPixmap&, Pixmap&, bool& has_pxm);
#endif
#ifdef ARCHON_HAVE_GLX
    void new_gl_context_helper(const VisualSpec&, GLXContext share_list, bool direct, GLXContext&);
#endif
#ifdef ARCHON_HAVE_XRENDER
    void new_cursor_helper_1(const ScreenSpec&, int width, int height,
                             unsigned& good_width, unsigned& good_height,
                             bool& good, XRenderPictFormat*&);
#else
    void new_cursor_helper_1(const ScreenSpec&, int width, int height,
                             unsigned& good_width, unsigned& good_height, bool& good);
#endif
#ifdef ARCHON_HAVE_XRENDER
    void new_cursor_helper_2(const ScreenSpec&, XRenderPictFormat*, int width, int height,
                             int hotspot_x, int hotspot_y, XImage&, Pixmap&, GC&, Picture&,
                             Xlib_Cursor&, bool& has_pxm, bool& has_gc, bool& has_pic,
                             bool& has_curs);
#endif
    void new_cursor_helper_3(const ScreenSpec&, int width, int height, int hotspot_x,
                             int hotspot_y, XImage& ximg_base, XImage& ximg_mask, Pixmap& base_pxm,
                             Pixmap& mask_pxm, GC&, Xlib_Cursor&, bool& has_base_pxm,
                             bool& has_mask_pxm, bool& has_gc, bool& has_curs);

public: // Needs to be accessed from EventProcessor
    std::mutex event_proc_mutex;
    EventProcessorImpl* event_proc_master = nullptr; // Current master event receiver. Protected by `event_proc_mutex`
    std::list<EventProcessorImpl*> event_proc_waiters; // Current waiting event receivers. Protected by `event_proc_mutex`
    std::vector<std::unique_ptr<EventSlot[]>> event_proc_free_buffers; // Protected by `event_proc_mutex`

    const KeySymMapper keysym_mapper;

    mutable GuardState guard_state; // For long-jumping from Xlib's error handlers. Protected by `g_xlib_mutex`
};




std::mutex g_connections_mutex; // Must not be locked while holding a lock on `g_xlib_mutex`
std::map<Xlib_Display*, ConnectionImpl*> g_connections; // Protected by `g_connections_mutex`

GuardState g_super_guard_state; // Protected by `g_xlib_mutex`


/// This function is always called from within Xlib, so it will always be called
/// with a lock on `g_xlib_mutex`. The same is true for
/// xlib_fatal_error_handler().
///
/// \todo FIXME: We should attempt to extract more information from
/// the error event to the exception message.
int xlib_error_handler(Xlib_Display* dpy, XErrorEvent* error) throw()
{
    GuardState* s;
    char buf[512];
    {
        std::lock_guard<std::mutex> lock{g_connections_mutex};
        auto i = g_connections.find(dpy);
        if (i == g_connections.end()) {
            s = &g_super_guard_state;
        }
        else {
            s = &i->second->guard_state;
        }
        if (error) {
            XGetErrorText(dpy, error->error_code, buf, sizeof buf);
        }
        else {
            strncpy(buf, "Fatal error", sizeof buf);
        }
        buf[sizeof buf - 1] = '\0';
    }
    RAISE(*s, buf);
    return 0;
}

int xlib_fatal_error_handler(Xlib_Display* dpy) throw()
{
    xlib_error_handler(dpy, 0);
    return 0;
}



class ImplementationImpl: public Implementation {
public:
    std::string get_mnemonic() const override
    {
        return "xlib";
    }

    Connection::Ptr new_connection() override
    {
        std::string name, env_name = sys::getenv("DISPLAY");
        const char* name_ptr;
        Xlib_Display* dpy;

        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            name = m_display_name.empty() ? env_name : m_display_name;
            name_ptr = name.empty() ? ":0.0" : name.c_str();
            new_connection_helper(name_ptr, dpy); // Throws
        }
        if (!dpy)
            throw NoDisplayException("Could not connect to display '"+std::string(name_ptr)+"'");

        // FIXME: An out of memory would cause a leak of X resources.
        SharedPtr<ConnectionImpl> c(new ConnectionImpl(dpy));
        c->weak_self = c;
        return c;
    }

    void set_param(std::string name, std::string value) override
    {
        if (name != "display")
            throw BadParamException("Unrecognized Xlib paramter '"+name+"'");
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        m_display_name = value;
    }

    ImplementationImpl()
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};

        // This seems to be necessary even though all Xlib access is guarded
        // explicitely by mutexes. It seems that when XInitThreads is omitted,
        // there are race conditions when OpenGL calls overlap Xlib interaction.
        //
        // On the other hand, one might wonder if it is necessary to have the
        // g_xlib_mutex when we call XInitThreads, since then Xlib is supposed
        // to be thread safe. That, however, is clearly not the case.
        //
        // Fortunately, everything appears to work great when both kinds of
        // synchronization s are employed.
        if (!XInitThreads())
            throw std::runtime_error("XInitThreads failed");

        XSetErrorHandler(&xlib_error_handler);
        XSetIOErrorHandler(&xlib_fatal_error_handler);
    }

    ~ImplementationImpl() noexcept override
    {
    }

private:
    // When empty, the value of the DISPLAY environment variable will be used.
    std::string m_display_name;

    void new_connection_helper(const char* name_ptr, Xlib_Display*& dpy)
    {
        GUARD(g_super_guard_state, dpy = XOpenDisplay(name_ptr)); // Throws
    }
};




class DrawableImpl: public virtual Arch_Drawable {
public:
    const SharedPtr<ConnectionImpl> conn;
    Xlib_Display* const dpy;
    const int scr, vis;

    void put_image(Image::ConstRefArg, Box clip, Point position, PackedTRGB background) override;

    int get_screen() const override
    {
        return scr;
    }

    int get_visual() const override
    {
        return vis;
    }

    virtual Xlib_Drawable get_xlib_drawable() const = 0;

    DrawableImpl(ConnectionImpl* c, int scr, int vis):
        conn(c->weak_self),
        dpy(c->dpy),
        scr(scr),
        vis(vis)
    {
    }

    ~DrawableImpl() noexcept override
    {
    }

private:
    void put_image_helper(Xlib_Drawable, bool first_block, bool last_block,
                          int x, int y, int w, int h, XImage&,
                          bool& init_failed, bool& gc_failed, GC&);
};




class WindowImpl: public DrawableImpl, public Arch_Window {
public:
    void show() override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        if (m_is_visible)
            return;
        GUARD(conn->guard_state, XMapWindow(dpy, win)); // Throws
        m_is_visible = true;
    }

    void hide() override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        if (m_is_visible) {
            GUARD(conn->guard_state, XUnmapWindow(dpy, win)); // Throws
            m_is_visible = false;
        }
    }

    void set_title(std::string new_title) override;

    void set_position(int x, int y) override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        if (m_is_visible)
            GUARD(conn->guard_state, XMoveWindow(dpy, win, x, y)); // Throws
    }

    void set_size(int w, int h) override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, XResizeWindow(dpy, win, w, h)); // Throws
    }

    void set_bg_color(long rgb) override;

    void set_cursor(Arch_Cursor&) override;
    void reset_cursor() override;

    void set_fullscreen_enabled(bool enable) override;

    std::pair<int, int> get_position() const override
    {
        int x,y;
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, XGetGeometry(dpy, win, 0, &x, &y, 0, 0, 0, 0)); // Throws
        }
        return std::make_pair(x,y);
    }

    std::pair<int, int> get_size() const override
    {
        unsigned w,h;
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, XGetGeometry(dpy, win, 0, 0, 0, &w, &h, 0, 0)); // Throws
        }
        return std::make_pair<int, int>(w,h);
    }

/*
    void foo() override
    {
//        int ret;
        bool xinput_select_events_failed = false;
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, {
                    unsigned char mask[4]{};
                    XISetMask(mask, XI_RawMotion);
                    XISetMask(mask, XI_RawButtonPress);
                    XISetMask(mask, XI_RawButtonRelease);
                    XIEventMask mask_2{};
                    mask_2.deviceid = XIAllDevices;
                    mask_2.mask_len = sizeof mask;
                    mask_2.mask = mask;
                    Status status = XISelectEvents(dpy, win, &mask_2, 1);
                    if (status != Success)
                        xinput_select_events_failed = true;
                });
//            GUARD(conn->guard_state, ret = XGrabPointer(dpy, win, True, PointerMotionMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime)); // Throws                            
        }
//        std::cerr << "ret = " << ret << " ("<<GrabNotViewable<<","<<AlreadyGrabbed<<","<<GrabFrozen<<")\n";
        if (xinput_select_events_failed)
            throw std::runtime_error("Foo");
    }
*/

    void report_mouse_move(bool enable) override
    {
        std::lock_guard<std::mutex> lock{m_events_mutex};
        if (m_mouse_motion_always == enable)
            return;
        if (m_events_enabled)
            update_xlib_event_mask(m_events_enabled, enable); // Throws
        m_mouse_motion_always = enable;
    }

    void enable_relative_mouse_motion(bool enable) override
    {
        static_cast<void>(enable);                                           
    }

    void swap_buffers() override
    {
#ifdef ARCHON_HAVE_GLX
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, glXSwapBuffers(dpy, win)); // Throws
#endif
    }

    Xlib_Drawable get_xlib_drawable() const override
    {
        return static_cast<Xlib_Drawable>(win);
    }

    WindowImpl(ConnectionImpl* c, int scr, int vis, Xlib_Window w, Xlib_Colormap m):
        DrawableImpl(c, scr, vis),
        win(w),
        colmap(m)
    {
    }

    ~WindowImpl() noexcept override;

    bool set_event_proc(EventProcessorImpl*);
    void unset_event_proc();

    // Must be called with lock on `m_events_mutex`
    void update_xlib_event_mask(bool enabled, bool motion_always)
    {
        int base_mask = FocusChangeMask | EnterWindowMask | LeaveWindowMask |
            ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyRelease | StructureNotifyMask |
            VisibilityChangeMask | ExposureMask;

        XSetWindowAttributes swa;
        swa.event_mask = (enabled ? base_mask | (motion_always ? PointerMotionMask : ButtonMotionMask) : 0);

        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, XChangeWindowAttributes(dpy, win, CWEventMask, &swa)); // Throws
        }
    }

    const Xlib_Window win;
    const Xlib_Colormap colmap;
    bool has_gl_support;

private:
    bool m_is_visible = false; // Protected by `g_xlib_mutex`

    std::mutex m_events_mutex;
    WeakPtr<EventProcessorImpl> m_event_proc; // Protected by `m_events_mutex`
    bool m_events_enabled = false;    // Protected by `m_events_mutex`
    bool m_mouse_motion_always = false; // Protected by `m_events_mutex`
};




#ifdef ARCHON_HAVE_GLX
class PixelBufferImpl: public DrawableImpl, public PixelBuffer {
public:
    const Pixmap pxm;
    const GLXPixmap glx_pxm;
    const int width, height;

    Image::Ref get_image() override
    {
        const ConnectionImpl::VisualSpec& v = conn->get_visual(scr, vis);

        XImage ximg;
        Image::Ref img = v.image_format.setup_transcode(ximg, width, height);

        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, // Throws
                  {
                      // CAUTION: Before changing anything in this block, read
                      // the comments about GUARD at the start of this file.
                      XInitImage(&ximg);
                      XGetSubImage(dpy, pxm, 0, 0, width, height, AllPlanes, ZPixmap, &ximg, 0, 0);
                  });
        }

        return img;
    }

    Xlib_Drawable get_xlib_drawable() const override
    {
        return static_cast<Xlib_Drawable>(pxm);
    }

    PixelBufferImpl(ConnectionImpl* c, int scr, int vis, Pixmap p, GLXPixmap q, int w, int h):
        DrawableImpl(c, scr, vis),
        pxm(p),
        glx_pxm(q),
        width(w),
        height(h)
    {
    }

    ~PixelBufferImpl() noexcept override
    {
    }
};
#endif // defined ARCHON_HAVE_GLX




#ifdef ARCHON_HAVE_GLX
class ContextImpl: public Context {
public:
    const SharedPtr<ConnectionImpl> conn;
    Xlib_Display* const dpy;
    const int scr, vis;
    const GLXContext ctx;

    bool bound = false; // Protected by `g_xlib_mutex`
    std::condition_variable unbind_cond; // Protected by `g_xlib_mutex`

    bool is_direct() const override
    {
        bool r;
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, r = glXIsDirect(dpy, ctx)); // Throws
        }
        return r;
    }

    ContextImpl(ConnectionImpl* c, int s, int v, GLXContext ctx):
        conn(c->weak_self),
        dpy(c->dpy),
        scr(s),
        vis(v),
        ctx(ctx)
    {
    }

    ~ContextImpl() noexcept override
    {
        try {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, glXDestroyContext(dpy, ctx)); // Throws
        }
        catch (BadConnectionException&) {
            // Don't care
        }
    }

private:
    void bind(Arch_Drawable::Arg d, bool block)
        throw(ContextAlreadyBoundException, NestedBindingException, std::invalid_argument) override
    {
        DrawableImpl* d2;
        GLXDrawable drb;
        if (WindowImpl* w = dynamic_cast<WindowImpl*>(d.get())) {
            d2 = w;
            drb = static_cast<GLXDrawable>(w->win);
        }
        else if (PixelBufferImpl* b = dynamic_cast<PixelBufferImpl*>(d.get())) {
            d2 = b;
            drb = static_cast<GLXDrawable>(b->glx_pxm);
        }
        else {
            throw std::invalid_argument("Implementation mismatch while binding OpenGL context");
        }

        if (d2->conn != conn)
            throw std::invalid_argument("Connection mismatch while binding OpenGL context");

        if (d2->scr != scr || d2->vis != vis)
            throw std::invalid_argument("Screen and/or visual mismatch while binding OpenGL context");

        bool nested, again = false, good;
        {
            std::unique_lock<std::mutex> lock{g_xlib_mutex};
            bind_helper(drb, block, nested, again, good, lock); // Throws
        }

        if (nested)
            throw NestedBindingException();
        if (again)
            throw ContextAlreadyBoundException();
        if (!good)
            throw std::runtime_error("'glXMakeCurrent' failed");
    }

    void unbind() override
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, glXMakeCurrent(dpy, None, 0)); // Throws
        bound = false;
        unbind_cond.notify_all();
    }

    void bind_helper(GLXDrawable drb, bool block, bool& nested, bool& again, bool& good,
                     std::unique_lock<std::mutex>& lock)
    {
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read the
                  // comments about GUARD at the start of this file.

                  // Check that this thread is not already bound to a context
                  nested = glXGetCurrentContext();
                  if (!nested) {
                      // Attempt to bind to the context
                      while (bound) {
                          if (!block) {
                              again = true;
                              break;
                          }
                          unbind_cond.wait(lock);
                      }

                      if (!again) {
                          good = glXMakeCurrent(dpy, drb, ctx);
                          if (good)
                              bound = true;
                      }
                  }
              });
    }
};
#endif // defined ARCHON_HAVE_GLX




class EventProcessorImpl: public EventProcessor {
public:
    const SharedPtr<ConnectionImpl> conn;
    EventHandler* const handler;

    // Allows construction of SharedPtr's when 'this' is the only
    // thing you've got. Is constant after construction.
    WeakPtr<EventProcessorImpl> weak_self;

    void register_window(Arch_Window::Arg w, int cookie) override
    {
        SharedPtr<WindowImpl> win = dynamic_pointer_cast<WindowImpl>(w);
        if (!win)
            throw std::invalid_argument("Implementation mismatch in event window registration");

        if (win->conn != conn)
            throw std::invalid_argument("Connection mismatch in event window registration");

        // If the window is already registered with this processor, do nothing.
        {
            std::lock_guard<std::mutex> lock{m_windows_mutex};
            if (m_windows.find(win->win) != m_windows.end())
                return;
        }

        if (!win->set_event_proc(this))
            throw std::invalid_argument("Window already registered with other event processor");

        conn->register_event_window(win->win, this, cookie);

        {
            std::lock_guard<std::mutex> lock{m_windows_mutex};
            m_windows[win->win] = win;
        }
    }


    void process(std::chrono::steady_clock::time_point timeout) override;

    void get_key_sym_names(const std::vector<Arch_KeySym>& key_syms,
                           std::vector<std::string>& names) override;

    EventProcessorImpl(ConnectionImpl* c, EventHandler* h):
        conn{c->weak_self},
        handler{h},
        waiter_cond{} // Throws
    {
    }

    ~EventProcessorImpl() noexcept override
    {
        try {
            for (const auto& entry: m_windows) {
                conn->unregister_event_window(entry.first);
                if (SharedPtr<WindowImpl> w = entry.second.lock())
                    w->unset_event_proc();
            }
        }
        catch (BadConnectionException&) {
            // Don't care
        }
    }

    // Called only from ~WindowImpl but possibly by many threads simultaneously.
    void unregister_window(Xlib_Window w) throw()
    {
        {
            std::lock_guard<std::mutex> lock{m_windows_mutex};
            m_windows.erase(w);
        }

        conn->unregister_event_window(w);
    }


    // Calling thread must currently be the master.
    EventSlot& get_event_slot()
    {
        if (first_free_slot == s_slots_per_buf) {
            {
                std::lock_guard<std::mutex> lock{conn->event_proc_mutex};
                if (conn->event_proc_free_buffers.empty()) {
                    std::unique_ptr<EventSlot[]> buf =
                        std::make_unique<EventSlot[]>(s_slots_per_buf); // Throws
                    last_buffer = buf.get();
                    buffers.push_back(std::move(buf)); // Throws
                }
                else {
                    std::unique_ptr<EventSlot[]>& buf = conn->event_proc_free_buffers.back();
                    last_buffer = buf.get();
                    buffers.push_back(std::move(buf)); // Throws
                    conn->event_proc_free_buffers.pop_back();
                }
                committed += uncommitted;
            }
            uncommitted = 0;
            first_free_slot = 0;
        }
        ++uncommitted;
        return last_buffer[first_free_slot++];
    }


    /// \todo FIXME: Handle wrap around after 49 days on 32-bit platforms
    /// (Xlib/X protocol issue).
    TimedEvent::Timestamp map_time(Xlib_Time t)
    {
        return TimedEvent::Timestamp{t};
    }

    Arch_KeySym map_keysym(Xlib_KeySym s)
    {
        // To decouple Archon symbol identifiers from Xlib we
        // map them once again to Archon KeySym identifiers.
        return conn->keysym_mapper.xlib_to_archon(s);
    }

private:
    static constexpr int s_slots_per_buf = 128; // 128 gives a buffer size of 4KB assuming 32 bytes per slot.

    std::mutex m_windows_mutex;
    std::map<Xlib_Window, WeakPtr<WindowImpl>> m_windows; // Protected by `m_windows_mutex`

public:
    // 'buffers', 'committed', and 'waiter_cond' may be accessed only
    // by the master thread and the thread that owns this event
    // processor. This implies that the master may access these
    // attributes in its own event processor without acqquiring the
    // mutex.
    std::deque<std::unique_ptr<EventSlot[]>> buffers; // Protected by `conn->event_proc_mutex`. Buffer memory is owned by this class.
    int committed = 0; // Numbber of committed event slots. Protected by `conn->event_proc_mutex`.
    std::condition_variable waiter_cond; // Signalled when events are available, or when the master disappears. Protected by `conn->event_proc_mutex`.

    int first_free_slot = s_slots_per_buf; // Accessed only by master.
    EventSlot* last_buffer = nullptr; // Accessed only by master.
    int uncommitted = 0; // Number of uncommitted event slots. Accessed only by master.

    int first_available_slot = 0; // Accessed only by this processor.
    EventSlot* first_buffer = nullptr; // Accessed only by this processor.
    int available = 0; // Accessed only by this processor.
};



class CursorImpl: public Arch_Cursor {
public:
    const SharedPtr<ConnectionImpl> conn;
    Xlib_Display* const dpy;
    const int scr;
    const Xlib_Cursor cursor;

    CursorImpl(ConnectionImpl* c, int scr, Xlib_Cursor cur):
        conn(c->weak_self),
        dpy(c->dpy),
        scr(scr),
        cursor(cur)
    {
    }

    ~CursorImpl() noexcept override
    {
        try {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, XFreeCursor(dpy, cursor)); // Throws
        }
        catch (BadConnectionException&) {
             // Don't care
        }
    }
};







Image::Ref ImageFormat::setup_transcode(XImage& ximg, int width, int height) const
{
    BufferedImage::Ref img =
        BufferedImage::new_image(width, height, m_color_space, m_has_alpha, m_buffer_format);

    ximg.width            = width;
    ximg.height           = height;
    ximg.xoffset          = 0;
    ximg.format           = ZPixmap;
    ximg.data             = reinterpret_cast<char*>(img->get_buffer_ptr());
    ximg.byte_order       = m_ximg_byte_order;
    ximg.bitmap_unit      = m_ximg_bitmap_unit;
    ximg.bitmap_bit_order = m_ximg_bitmap_bit_order;
    ximg.bitmap_pad       = m_ximg_bitmap_pad;
    ximg.depth            = m_ximg_depth;
    ximg.bytes_per_line   = m_buffer_format->get_bytes_per_strip(width);
    ximg.bits_per_pixel   = m_buffer_format->get_bits_per_pixel();

    return img;
}




void ImageFormat::single_channel_format(const ConnectionImpl* conn, int depth,
                                        ImageFormat& img_fmt)
{
    img_fmt.m_color_space = ColorSpace::get_Lum();
    img_fmt.m_has_alpha   = false;
    img_fmt.m_ximg_depth  = depth;

    IntegerBufferFormat::ChannelLayout channels(depth);
    init_format(conn, channels, img_fmt);
}




void ImageFormat::xvisual_format(const ConnectionImpl* conn, const XVisualInfo* vis_info,
                                 ImageFormat& img_fmt)
{
    // Non-decomposed color maps will be handled as simple single
    // channel images.
    if (vis_info->c_class != TrueColor && vis_info->c_class != DirectColor) {
        single_channel_format(conn, vis_info->depth, img_fmt);
        return;
    }

    img_fmt.m_color_space = ColorSpace::get_RGB();
    img_fmt.m_has_alpha   = false;
    img_fmt.m_ximg_depth  = vis_info->depth;

    IntegerBufferFormat::ChannelLayout channels(make_channel(vis_info->red_mask),
                                                make_channel(vis_info->green_mask),
                                                make_channel(vis_info->blue_mask));
    init_format(conn, channels, img_fmt);
}




#ifdef ARCHON_HAVE_XRENDER
void ImageFormat::xrender_format(const ConnectionImpl* conn,
                                 const XRenderPictFormat* xrender_fmt,
                                 ImageFormat& img_fmt)
{
    // Non-decomposed color maps will be handled as simple single
    // channel images.
    if (xrender_fmt->type == PictTypeIndexed) {
        single_channel_format(conn, xrender_fmt->depth, img_fmt);
        return;
    }

    img_fmt.m_color_space = ColorSpace::get_RGB();
    img_fmt.m_has_alpha   = true;
    img_fmt.m_ximg_depth  = xrender_fmt->depth;

    const XRenderDirectFormat& direct = xrender_fmt->direct;
    IntegerBufferFormat::ChannelLayout channels(make_channel(direct.red,   direct.redMask),
                                                make_channel(direct.green, direct.greenMask),
                                                make_channel(direct.blue,  direct.blueMask),
                                                make_channel(direct.alpha, direct.alphaMask));
    init_format(conn, channels, img_fmt);
}
#endif // defined ARCHON_HAVE_XRENDER




void ImageFormat::init_format(const ConnectionImpl* conn,
                              IntegerBufferFormat::ChannelLayout& channels,
                              ImageFormat& img_fmt)
{
    // The story about XImage -> XInitImage -> XPutImage:
    // --------------------------------------------------
    //
    // First we need to clearly define the implied order of pixels in the image
    // when we say such things as 'the first pixel' or 'two consecutive
    // pixels'. For the moment, this has nothing to do with the order that
    // pixels are store in memory.
    //
    // The implied order of pixels is 'row-major' (or scanline-order) starting
    // at the upper left corner. Thus, the 'first pixel' refers to the pixel in
    // the upper left corder of the image, while the 'second pixel' refers to
    // the one appearing immediately to the right of the first one, or if the
    // width of the image is 1, then the one appearing immediately below the
    // first one.
    //
    // Now, this implied order, is also the overall order of pixels in memory,
    // however there are some details that will be spelled out below.
    //
    //
    // The three color masks in the XImage structure are never used. The masks
    // are always determined by the visual on which the image data eventially
    // will be displayed. Further more, masks are only used for TrueColor and
    // DirectColor visual classes.
    //
    // When bytes_per_line is specified, bitmap_pad is never used. Otherwise it
    // is used only to compute bytes_per_line, and must be an integer multiple
    // of bits_per_byte.
    //
    // Only specific values of bits_per_pixel are handled properly, but it is
    // not clear exactly which ones are and which ones are not, but it is always
    // valid to choose the value specified in the list of supported pixmap
    // formats under the selected depth. This is also the choice that leads to
    // the least amount of data rearrangement inside Xlib.
    //
    // The following assumes that format is ZImage, xoffset is zero, and that
    // bits_per_pixel is set according to the list of supported pixmap formats
    // of the display.
    //
    // When 1 < bits_per_pixel:
    //
    //   bitmap_unit and bitmap_bit_order are not used.
    //
    //   When bits_per_byte <= bits_per_pixel:
    //
    //     N = bits_per_pixel / bits_per_byte must be an integer (not
    //     necessarily a power of 2.)
    //
    //     Each pixel consist of N memory consecutive bytes. Two of
    //     these bytes are adjacent in memory if, and only if, they
    //     occupy adjacent significance (bit-positions) within the
    //     pixel value.
    //
    //     If byte_order is LSBFirst, then the byte at the lowest
    //     memory address is the one whose bits has least
    //     significance, otherwise, if byte_order is MSBFirst, then it
    //     is the byte whose bits has most significance.
    //
    //   When bits_per_pixel < bits_per_byte:
    //
    //     N = bits_per_byte / bits_per_pixel must be an integer.
    //
    //     Each byte is 'sliced' into N pieces, and the number of bits
    //     in each piece is bits_per_pixel. Each piece holds a pixel.
    //
    //     If two pixels from the same byte are horizontally adjacent
    //     in the image, then the corresponding pieces occupy adjacent
    //     significance (bit-positions) within the byte value.
    //
    //     If byte_order is LSBFirst, then the first pixel (according
    //     to the implied order of pixels in the image) is the one
    //     that occupies bits of least significance, otherwise, if
    //     byte_order is MSBFirst, then it is the pixel that occupies
    //     bits of most significance.
    //
    // When bits_per_pixel == 1:
    //
    //   Depth has to be 1.
    //
    //   N = bitmap_unit / bits_per_byte must be an integer.
    //
    //   For each scanline, memory is first divided into words of N
    //   bytes each. Each bit in a word corresponds with a pixel.
    //
    //   If two pixels from the same word are horizontally adjacent in
    //   the image, then the bits occupy adjacent significance
    //   (bit-position) within the word value.
    //
    //   If bit_order is LSBFirst, then the first pixel (according to
    //   the implied order of pixels in the image) is the one that
    //   occupies the least significant bit, otherwise, if byte_order
    //   is MSBFirst, then it is the pixel that occupies the most
    //   significant bit.
    //
    //   Two bytes from the same word are adjacent in memory if, and
    //   only if, they occupy adjacent significance (bit-positions)
    //   within the word value.
    //
    //   If byte_order is LSBFirst, then the byte at the lowest memory
    //   address is the one whose bits has least significance,
    //   otherwise, if byte_order is MSBFirst, then it is the byte
    //   whose bits has most significance.

    auto i = conn->image_formats.find(img_fmt.m_ximg_depth);
    ARCHON_ASSERT_1(i != conn->image_formats.end(), "Bad depth");
    const ConnectionImpl::ImgFmtDetail &fmt = i->second;
    int bits_per_pixel = channels.bits_per_pixel = fmt.bits_per_pixel;
    ARCHON_ASSERT_1(img_fmt.m_ximg_depth <= bits_per_pixel,
                    "Inconsitency between depth and bits per pixel");
    ARCHON_ASSERT_1(int(channels.channels.size()) <= img_fmt.m_ximg_depth,
                    "Inconsitency between depth and number of channels");

    // Choose some decent fallback values
    WordType word_type              = word_type_UChar;
    bool most_sig_bit_first         = false;
    img_fmt.m_ximg_byte_order       = LSBFirst;
    img_fmt.m_ximg_bitmap_unit      = conn->bitmap_unit;
    img_fmt.m_ximg_bitmap_bit_order = conn->bitmap_bit_order;
    img_fmt.m_ximg_bitmap_pad       = fmt.scanline_pad; // Not relevant, but need sane value

    int bits_per_byte = std::numeric_limits<unsigned char>::digits;
    if (bits_per_pixel == 1) {
        // Try to let Xlib decide on the word type based on its preferred number
        // of bits per scanline unit. This, however, can only work if the
        // endianness of the platform is "clean cut" little-endian or
        // big-endian. If it is a mixed endianness, we must fall back on \c char
        // as the word type.
        if (is_clean_endian) {
            try {
                word_type = get_word_type_by_bit_width(conn->bitmap_unit);
                if (is_big_endian)
                    img_fmt.m_ximg_byte_order = MSBFirst;
            }
            catch (NoSuchWordTypeException&) {
                img_fmt.m_ximg_bitmap_unit = bits_per_byte;
            }
        }
        if (img_fmt.m_ximg_bitmap_bit_order == MSBFirst)
            most_sig_bit_first = true;
    }
    else if (bits_per_pixel < bits_per_byte) {
        // More than one pixel per byte
        int pixels_per_byte = bits_per_byte / bits_per_pixel;
        ARCHON_ASSERT_1(pixels_per_byte * bits_per_pixel ==  bits_per_byte,
                        "Bits per pixel does not divide bits per byte");
        img_fmt.m_ximg_byte_order = conn->image_byte_order;
        most_sig_bit_first = img_fmt.m_ximg_byte_order == MSBFirst;
    }
    else {
        // One or more bytes per pixel
        int bytes_per_pixel = bits_per_pixel / bits_per_byte;
        ARCHON_ASSERT_1(bytes_per_pixel * bits_per_byte ==  bits_per_pixel,
                        "Bits per bytes does not divide bits per pixel");
        // Pick a suitable word type and byte order; if the native byte order is
        // "clean cut" (little-endian or big-endian) and there is a word type
        // whose width is equal to the number of bit per pixel, then choose the
        // native byte order and the the mentioned word type. Otherwise choose
        // byte/char as the word type and then byte-order is immaterial.
        if (is_clean_endian) {
            try {
                // Try to let Xlib decide on the word type
                word_type = get_word_type_by_bit_width(bits_per_pixel);
                if (is_big_endian)
                    img_fmt.m_ximg_byte_order = MSBFirst;
            }
            catch (NoSuchWordTypeException&) {
                // Use fallbacks
            }
        }
    }

    img_fmt.m_buffer_format =
        IntegerBufferFormat::get_format(word_type, channels, most_sig_bit_first,
                                        true);  // True means word-aligned scanlines
}




#ifdef ARCHON_HAVE_GLX
int ConnectionImpl::choose_gl_visual(int scr, bool double_buffer, bool stereo,
                                     int red, int green, int blue, int alpha,
                                     int depth, int stencil,
                                     int accum_red, int accum_green,
                                     int accum_blue, int accum_alpha) const
{
    if (!have_glx)
        throw NoGLException();

    const ScreenSpec& s = get_screen(scr);
    scr = s.index;

    int attribs[32];
    {
        int i = 0;

        attribs[i++] = GLX_RGBA;

        if (double_buffer)
            attribs[i++] = GLX_DOUBLEBUFFER;
        if (stereo)
            attribs[i++] = GLX_STEREO;

        if (red) {
            attribs[i++] = GLX_RED_SIZE;
            attribs[i++] = red;
        }
        if (green) {
            attribs[i++] = GLX_GREEN_SIZE;
            attribs[i++] = green;
        }
        if (blue) {
            attribs[i++] = GLX_BLUE_SIZE;
            attribs[i++] = blue;
        }
        if (alpha) {
            attribs[i++] = GLX_ALPHA_SIZE;
            attribs[i++] = alpha;
        }
        if (depth) {
            attribs[i++] = GLX_DEPTH_SIZE;
            attribs[i++] = depth;
        }
        if (stencil) {
            attribs[i++] = GLX_STENCIL_SIZE;
            attribs[i++] = stencil;
        }

        if (accum_red) {
            attribs[i++] = GLX_ACCUM_RED_SIZE;
            attribs[i++] = accum_red;
        }

        if (accum_green) {
            attribs[i++] = GLX_ACCUM_GREEN_SIZE;
            attribs[i++] = accum_green;
        }

        if (accum_blue) {
            attribs[i++] = GLX_ACCUM_BLUE_SIZE;
            attribs[i++] = accum_blue;
        }

        if (accum_alpha) {
            attribs[i++] = GLX_ACCUM_ALPHA_SIZE;
            attribs[i++] = accum_alpha;
        }

        attribs[i] = None;
    }

    bool bad = false;
    VisualID id;
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        choose_gl_visual_helper(scr, attribs, id, bad); // Throws
    }
    if (bad)
        throw NoSuchVisualException();

    // Find visual index
    for (std::size_t i = 0, n = s.visual_specs.size(); i < n; ++i) {
        if (s.infos[i].visualid == id)
            return i;
    }
    throw std::runtime_error("Unable to find visual ID");
}

void ConnectionImpl::choose_gl_visual_helper(int scr, int* attribs, VisualID& id, bool& bad) const
{
    GUARD(guard_state, // Throws
          {
              XVisualInfo* info = glXChooseVisual(dpy, scr, attribs);
              if (info) {
                  id = info->visualid;
                  XFree(info);
              }
              else {
                  bad = true;
              }
          });
}
#else // ! defined ARCHON_HAVE_GLX
int ConnectionImpl::choose_gl_visual(int, bool, bool, int, int, int, int, int, int,
                                     int, int, int, int) const
{
    throw NoGLException();
}
#endif // ! defined ARCHON_HAVE_GLX




#ifdef ARCHON_HAVE_GLX
int ConnectionImpl::get_gl_buf_width(BufferType t, int scr, int vis) const
{
    if (!have_glx)
        throw NoGLException();
    const VisualSpec& v = get_visual(scr, vis);
    switch (t) {
        case buf_red:
            return v.width_red;
        case buf_green:
            return v.width_green;
        case buf_blue:
            return v.width_blue;
        case buf_alpha:
            return v.width_alpha;
        case buf_depth:
            return v.width_depth;
        case buf_stencil:
            return v.width_stencil;
        case buf_accum_red:
            return v.width_accum_red;
        case buf_accum_green:
            return v.width_accum_green;
        case buf_accum_blue:
            return v.width_accum_blue;
        case buf_accum_alpha:
            return v.width_accum_alpha;
    }
    throw std::invalid_argument("Unexpected buffer type");
}
#else // ! defined ARCHON_HAVE_GLX
int ConnectionImpl::get_gl_buf_width(BufferType, int, int) const
{
    throw NoGLException();
}
#endif // ! defined ARCHON_HAVE_GLX




Arch_Window::Ptr ConnectionImpl::new_window(int width, int height, int scr, int vis)
{
    const VisualSpec& v = get_visual(scr, vis);
    scr = v.screen->index;
    vis = v.index;

    Xlib_Colormap colmap;
    Xlib_Window win;
    bool has_colmap = false, has_win = false;
    try {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        new_window_helper(v, width, height, colmap, win, has_colmap, has_win); // Throws
    }
    catch (BadConnectionException&) {
        // Connection is bad, but it could be non-fatal, so we should
        // still attempt to clean up.
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(guard_state, // Throws
                  {
                      if (has_win)
                          XDestroyWindow(dpy, win);
                      if (has_colmap)
                          XFreeColormap(dpy, colmap);
                  });
        }
        throw;
    }

    // FIXME: An out of memory would cause a leak of X resources.
    Arch_Window::Ptr w(new WindowImpl(this, scr, vis, win, colmap));
    return w;
}

void ConnectionImpl::new_window_helper(const VisualSpec& v, int width, int height,
                                       Xlib_Colormap& colmap, Xlib_Window& win,
                                       bool& has_colmap, bool& has_win)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.

              colmap = XCreateColormap(dpy, v.screen->root_win, v.info->visual, AllocNone);
              has_colmap = true;

              XSetWindowAttributes swa;
              // No events are reported by default. This is reconfigured later
              // when the window is bound to an event processor.
              swa.event_mask = 0;
              swa.colormap = colmap;
              win = XCreateWindow(dpy, v.screen->root_win, 0, 0, width, height, 0, v.info->depth,
                                  InputOutput, v.info->visual, CWEventMask|CWColormap, &swa);
              has_win = true;

              // Ask X to notify rather than kill us when the user attempts to
              // close the window.
              XSetWMProtocols(dpy, win, &atom_del_win, 1);
          });
}




#ifdef ARCHON_HAVE_GLX
PixelBuffer::Ptr ConnectionImpl::new_pixel_buffer(int width, int height, int scr, int vis)
{
    if (!have_glx)
        throw NoGLException();

    const VisualSpec& v = get_visual(scr, vis);
    scr = v.screen->index;
    vis = v.index;

    if (!v.gl_support)
        throw std::invalid_argument("Visual lacks OpenGL support");

    GLXPixmap glx_pxm;
    Pixmap pxm;
    bool has_pxm = false;
    try {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        new_pixel_buffer_helper(width, height, v, glx_pxm, pxm, has_pxm); // Throws
    }
    catch (BadConnectionException&) {
        // Connection is bad, but it could be non-fatal, so we should still
        // attempt to clean up.
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(guard_state, // Throws
                  {
                      if (has_pxm)
                          XFreePixmap(dpy, pxm);
                  });
        }
        throw;
    }

    // FIXME: An out of memory would cause a leak of X resources.
    PixelBuffer::Ptr b(new PixelBufferImpl(this, scr, vis, pxm, glx_pxm, width, height));
    return b;
}

void ConnectionImpl::new_pixel_buffer_helper(int width, int height, const VisualSpec& v,
                                             GLXPixmap& glx_pxm, Pixmap& pxm, bool& has_pxm)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.

              pxm = XCreatePixmap(dpy, v.screen->root_win, width, height, v.info->depth);
              has_pxm = true;
              glx_pxm = glXCreateGLXPixmap(dpy, v.info, pxm);
          });

}
#else // ! defined ARCHON_HAVE_GLX
PixelBuffer::Ptr ConnectionImpl::new_pixel_buffer(int, int, int, int)
{
    throw NoGLException();
}
#endif // ! defined ARCHON_HAVE_GLX




#ifdef ARCHON_HAVE_GLX
Context::Ptr ConnectionImpl::new_gl_context(int scr, int vis, bool direct, Context::Arg share_with)
{
    if (!have_glx)
        throw NoGLException();

    const VisualSpec& v = get_visual(scr, vis);
    scr = v.screen->index;
    vis = v.index;

    if (!v.gl_support)
        throw std::invalid_argument("Visual lacks OpenGL support");

    ContextImpl* c = dynamic_cast<ContextImpl*>(share_with.get());
    if (share_with && !c)
        throw std::invalid_argument("Implementation mismatch");

    GLXContext share_list = (c ? c->ctx : nullptr);
    GLXContext ctx;
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        new_gl_context_helper(v, share_list, direct, ctx); // Throws
    }

    // FIXME: An out of memory would cause a leak of X resources.
    return Context::Ptr(new ContextImpl(this, scr, vis, ctx));
}

void ConnectionImpl::new_gl_context_helper(const VisualSpec& v, GLXContext share_list, bool direct,
                                           GLXContext& ctx)
{
    GUARD(guard_state, ctx = glXCreateContext(dpy, v.info, share_list, direct)); // Throws
}
#else // ! defined ARCHON_HAVE_GLX
Context::Ptr ConnectionImpl::new_gl_context(int, int, bool, Context::Arg)
{
    throw NoGLException();
}
#endif // ! defined ARCHON_HAVE_GLX




EventProcessor::Ptr ConnectionImpl::new_event_processor(EventHandler* h)
{
    SharedPtr<EventProcessorImpl> p(new EventProcessorImpl(this, h));
    p->weak_self = p;
    return p;
}




std::unique_ptr<Arch_Cursor> ConnectionImpl::new_cursor(Image::Ref image, int hotspot_x,
                                                        int hotspot_y, int scr)
{
    const ScreenSpec& s = get_screen(scr);
    scr = s.index;

    int width  = image->get_width();
    int height = image->get_height();
    image = Oper::flip(image, false, true); // Upside down

    unsigned good_width, good_height;
    bool good;
#ifdef ARCHON_HAVE_XRENDER
    XRenderPictFormat *xrender_fmt;
#endif

    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
#ifdef ARCHON_HAVE_XRENDER
        new_cursor_helper_1(s, width, height, good_width, good_height, good, xrender_fmt); // Throws
#else
        new_cursor_helper_1(s, width, height, good_width, good_height, good); // Throws
#endif
    }

    if (!good)
        throw std::runtime_error("XQueryBestCursor failed");
#ifdef ARCHON_HAVE_XRENDER
    if (!xrender_fmt)
        throw std::runtime_error("XRenderFindStandardFormat failed");
#endif

    width  = good_width;
    height = good_height;
    hotspot_x = clamp(hotspot_x, 0, width-1);
    hotspot_y = clamp(hotspot_y, 0, height-1);

    Xlib_Cursor cursor;

#ifdef ARCHON_HAVE_XRENDER

    if (have_xrender) {
        ImageFormat img_fmt;
        ImageFormat::xrender_format(this, xrender_fmt, img_fmt);
        XImage ximg;
        Image::Ref img = img_fmt.setup_transcode(ximg, width, height);

        img->put_image(image, 0, 0, false);

        Pixmap pxm;
        GC gc;
        Picture pic;
        bool has_pxm = false, has_gc = false, has_pic = false, has_curs = false;
        try {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            new_cursor_helper_2(s, xrender_fmt, width, height, hotspot_x, hotspot_y, ximg, pxm, gc,
                                pic, cursor, has_pxm, has_gc, has_pic, has_curs); // Throws
        }
        catch (BadConnectionException&) {
            // Connection is bad, but it could be non-fatal, so we should
            // still attempt to clean up.
            {
                std::lock_guard<std::mutex> lock{g_xlib_mutex};
                GUARD(guard_state, // Throws
                      {
                          if (has_curs)
                              XFreeCursor(dpy, cursor);
                          if (has_pic)
                              XRenderFreePicture(dpy, pic);
                          if (has_gc)
                              XFreeGC(dpy, gc);
                          if (has_pxm)
                              XFreePixmap(dpy, pxm);
                      });
            }
            throw;
        }
    }

#endif // ARCHON_HAVE_XRENDER

    if (!have_xrender) {
        ImageFormat img_fmt;
        ImageFormat::single_channel_format(this, 1, img_fmt);
        XImage ximg_base, ximg_mask;

        Image::Ref img_base = img_fmt.setup_transcode(ximg_base, width, height);
        img_base->put_image(Oper::discard_alpha(image), 0, 0, false);

        Image::Ref img_mask = img_fmt.setup_transcode(ximg_mask, width, height);
        if (image->has_alpha_channel()) {
            img_mask->put_image(Oper::pick_channel(image, image->get_num_channels()-1), 0, 0, false);
        }
        else {
            img_mask->fill(color::white);
        }

        Pixmap base_pxm, mask_pxm;
        GC gc;
        bool has_base_pxm = false, has_mask_pxm, has_gc = false, has_curs = false;
        try {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            new_cursor_helper_3(s, width, height, hotspot_x, hotspot_y, ximg_base, ximg_mask,
                                base_pxm, mask_pxm, gc, cursor, has_base_pxm, has_mask_pxm,
                                has_gc, has_curs); // Throws
        }
        catch (BadConnectionException&) {
            // Connection is bad, but it could be non-fatal, so we should
            // still attempt to clean up.
            {
                std::lock_guard<std::mutex> lock{g_xlib_mutex};
                GUARD(guard_state, // Throws
                      {
                          if (has_curs)
                              XFreeCursor(dpy, cursor);
                          if (has_gc)
                              XFreeGC(dpy, gc);
                          if (has_mask_pxm)
                              XFreePixmap(dpy, mask_pxm);
                          if (has_base_pxm)
                              XFreePixmap(dpy, base_pxm);
                      });
            }
            throw;
        }
    }

    // FIXME: An out of memory would cause a leak of X resources.
    return std::make_unique<CursorImpl>(this, scr, cursor);
}

#ifdef ARCHON_HAVE_XRENDER
void ConnectionImpl::new_cursor_helper_1(const ScreenSpec& s, int width, int height,
                                         unsigned& good_width, unsigned& good_height,
                                         bool& good, XRenderPictFormat*& xrender_fmt)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              good = XQueryBestCursor(dpy, s.root_win, width, height, &good_width, &good_height);
              xrender_fmt = XRenderFindStandardFormat(dpy, PictStandardARGB32);
          });
}
#else
void ConnectionImpl::new_cursor_helper_1(const ScreenSpec& s, int width, int height,
                                         unsigned& good_width, unsigned& good_height, bool& good)
{
    GUARD(guard_state, // Throws
          good = XQueryBestCursor(dpy, s.root_win, width, height, &good_width, &good_height));
}
#endif

#ifdef ARCHON_HAVE_XRENDER
void ConnectionImpl::new_cursor_helper_2(const ScreenSpec& s, XRenderPictFormat* xrender_fmt,
                                         int width, int height, int hotspot_x, int hotspot_y,
                                         XImage& ximg, Pixmap& pxm, GC& gc, Picture& pic,
                                         Xlib_Cursor& cursor, bool& has_pxm, bool& has_gc,
                                         bool& has_pic, bool& has_curs)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              XInitImage(&ximg);
              pxm = XCreatePixmap(dpy, s.root_win, width, height, 32);
              has_pxm = true;
              gc = XCreateGC(dpy, pxm, 0, 0);
              has_gc = true;
              XPutImage(dpy, pxm, gc, &ximg, 0, 0, 0, 0, width, height);
              has_gc = false;
              XFreeGC(dpy, gc);

              XRenderPictureAttributes attr;
              pic = XRenderCreatePicture(dpy, pxm, xrender_fmt, 0, &attr);
              has_pic = true;
              has_pxm = false;
              XFreePixmap(dpy, pxm);
              cursor = XRenderCreateCursor(dpy, pic, hotspot_x, hotspot_y);
              has_curs = true;
              has_pic = false;
              XRenderFreePicture(dpy, pic);
          });
}
#endif

void ConnectionImpl::new_cursor_helper_3(const ScreenSpec& s, int width, int height, int hotspot_x,
                                         int hotspot_y, XImage& ximg_base, XImage& ximg_mask,
                                         Pixmap& base_pxm, Pixmap& mask_pxm, GC& gc,
                                         Xlib_Cursor& cursor, bool& has_base_pxm,
                                         bool& has_mask_pxm, bool& has_gc, bool& has_curs)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              XInitImage(&ximg_base);
              base_pxm = XCreatePixmap(dpy, s.root_win, width, height, 1);
              has_base_pxm = true;
              gc = XCreateGC(dpy, base_pxm, 0, 0);
              has_gc = true;
              XPutImage(dpy, base_pxm, gc, &ximg_base, 0, 0, 0, 0, width, height);

              XInitImage(&ximg_mask);
              mask_pxm = XCreatePixmap(dpy, s.root_win, width, height, 1);
              has_mask_pxm = true;
              XPutImage(dpy, mask_pxm, gc, &ximg_mask, 0, 0, 0, 0, width, height);
              has_gc = false;
              XFreeGC(dpy, gc);

              int scr = s.index;
              XColor black;
              XColor white;
              black.pixel = BlackPixel(dpy, scr);
              white.pixel = WhitePixel(dpy, scr);
              Xlib_Colormap colmap = DefaultColormap(dpy, scr);
              XQueryColor(dpy, colmap, &black);
              XQueryColor(dpy, colmap, &white);

              cursor = XCreatePixmapCursor(dpy, base_pxm, mask_pxm, &white, &black, hotspot_x, hotspot_y);
              has_curs = true;
              has_mask_pxm = false;
              XFreePixmap(dpy, mask_pxm);
              has_base_pxm = false;
              XFreePixmap(dpy, base_pxm);
          });
}



/// Called by an event processor in a request to wait for incoming events. If
/// events are already available, this method returns immediately, otherwise if
/// the connection has no current master event thread, this thread becomes the
/// master. Otherwise it will simply wait for the master thread to deliver
/// events. If the master exits, one of the waiting threads will become the
/// master.
///
/// \throw InterruptException If the calling thread has received an interruption
/// request.
void ConnectionImpl::receive_events(EventProcessorImpl* proc,
                                    std::chrono::steady_clock::time_point timeout)
{
    if (proc->available)
        return;

    // A receiver is a thread that is currently executing this method
    // (receive_events).
    //
    // A waiter is a receiver that is waiting for notification on its
    // 'waiter_cond'.
    //
    // A master is a receiver that is appointed the role of listening for
    // incoming data on the X11 connection socket, and when events arrive,
    // distributing them to the designated event processors.
    //
    // A master may or may not also be a waiter, however if it is a waiter, it
    // must be ready to run, that is, it must have been notified on its
    // 'waiter_cond'.
    //
    // Invariants (when no threads hold a lock on event_proc_mutex):
    //
    //  - A thread is a waiter if, and only if its associated event processor is
    //    in 'conn->event_proc_waiters'.
    //
    //  - A thread is a master if, and only if 'conn->event_proc_master' is a
    //    pointer to the thread's associated associated event processor.
    //
    //  - There is at most one master.
    //
    //  - If there are waiters, there is also a master.


    //  Must be instantiated in a scope with a lock on 'event_proc_mutex'.
    struct WaiterSentry {
        bool no_exit = false;

        WaiterSentry(ConnectionImpl& c, EventProcessorImpl& p):
            m_conn(c),
            m_proc(p),
            m_i(c.event_proc_waiters.insert(c.event_proc_waiters.end(), &p)) // Throws
        {
        }

        ~WaiterSentry() noexcept
        {
            m_conn.event_proc_waiters.erase(m_i);
            if (no_exit)
                return;
            m_proc.available += m_proc.committed;
            m_proc.committed = 0;
            if (m_proc.available && !m_proc.first_buffer)
                m_proc.first_buffer = m_proc.buffers.front().get();
            if (m_conn.event_proc_master != &m_proc)
                return;
            if (m_conn.event_proc_waiters.empty()) {
                m_conn.event_proc_master = nullptr;
                return;
            }
            EventProcessorImpl* m = m_conn.event_proc_waiters.front();
            m_conn.event_proc_master = m;
            m->waiter_cond.notify_all();
        }

    private:
        ConnectionImpl& m_conn;
        EventProcessorImpl& m_proc;
        const std::list<EventProcessorImpl*>::iterator m_i;
    };


    {
        std::unique_lock<std::mutex> lock{event_proc_mutex};

        if (proc->committed > 0) {
            proc->available += proc->committed;
            proc->committed = 0;
            if (proc->available > 0 && !proc->first_buffer)
                proc->first_buffer = proc->buffers.front().get();
            return;
        }

        if (event_proc_master) {
            // There is already a master, so this thread becomes a waiter

            WaiterSentry sentry{*this, *proc};
            for (;;) {
                std::cv_status status = std::cv_status::no_timeout;
                if (timeout.time_since_epoch().count() == 0) {
                    proc->waiter_cond.wait(lock);
                }
                else {
                    status = proc->waiter_cond.wait_until(lock, timeout);
                }
                if (status == std::cv_status::timeout || proc->committed != 0)
                    return;
                if (event_proc_master == proc) {
                    sentry.no_exit = true; // Disable master reassignment
                    break; // Fall through and become master
                }
            }
        }
        else {
            event_proc_master = proc; // Become master
        }
    }


    // Makes sure that the master role is reassigned to a waiter when the master
    // exits. Must be instantiated in a scope without a lock on
    // 'event_proc_mutex'.
    struct MasterSentry {
        MasterSentry(ConnectionImpl& c):
            m_conn(c)
        {
        }
        ~MasterSentry()
        {
            if (m_done)
                return;
            std::lock_guard<std::mutex> lock{m_conn.event_proc_mutex};
            reassign_caller_locked();
        }
        void reassign_caller_locked() // The caller is always the master at entry
        {
            m_done = true;
            if (m_conn.event_proc_waiters.empty()) {
                m_conn.event_proc_master = nullptr;
                return;
            }
            // Search for the first waiter that does not have committed
            // events. This is the one that have been waiting for the longest
            // time, and therefore also the one that is expected to have to
            // continue to wait the longest for another event. Choosing this one
            // as master is good because then there will be fewer master
            // reassignments. In particualr, if an event processor is created
            // with the sole purpose of acting as master, and therfore have no
            // associated windows, then it will eventually become master. If all
            // waiters have events, then just select any.
            for (EventProcessorImpl* proc: m_conn.event_proc_waiters) {
                if (!proc->committed) {
                    m_conn.event_proc_master = proc;
                    proc->waiter_cond.notify_all();
                    return;
                }
            }
            EventProcessorImpl* proc = m_conn.event_proc_waiters.front();
            m_conn.event_proc_master = proc;
            proc->waiter_cond.notify_all();
        }
    private:
        ConnectionImpl& m_conn;
        bool m_done = false;
    };

    MasterSentry sentry{*this};
    for (;;) { // Outer master loop
        {
            // This map serves two purposes, first it holds a reference count on
            // the associated event processor, keeping it alive, this, in turn,
            // guarantees that the referenced properties stay in
            // 'm_event_wins'. Since this referring map is destroyed before each
            // sleep, it does not keep abandoned event processors alive for too
            // long. This map also serves as a cache in that it will genrally
            // contain fewer entries than 'm_event_wins', and therfore the
            // lookup will benefit.
            std::map<Xlib_Window, EventWinPropsRef> props_ref_map;

            bool prev_win_undef = true;
            Xlib_Window prev_win_id;
            EventWinPropsRef* props_ref;
            int num_distrib = 0, event_index, events_read;
            bool stop_reading = false;
            goto start;
            for (;;) {
                if (++event_index == events_read) {
                    // If we have generated 512 events or more, stop reading
                    // more. This is to prevent lockout of event delivery when
                    // we are flooded with events.
                    if (stop_reading || num_distrib >= 512)
                        break;

                  start:
                    events_read = read_xevents(); // Throws
                    if (events_read == 0)
                        break;
                    if (events_read < s_xevent_buf_size)
                        stop_reading = true; // We've got everything
                    event_index = 0;
                }

                EventAndKeySym& entry = m_xevent_buf[event_index];
                XEvent& event = entry.xevent;

                // Map the Xlib window ID to a slot reference.
                if (prev_win_undef || prev_win_id != event.xany.window) {
                    auto i = props_ref_map.find(event.xany.window);
                    if (i == props_ref_map.end()) {
                        EventWinProps* s;
                        SharedPtr<EventProcessorImpl> p;
                        {
                            std::lock_guard<std::mutex> lock{m_event_wins_mutex};
                            auto j = m_event_wins.find(event.xany.window);
                            if (j != m_event_wins.end())
                                p = j->second.proc.lock();
                            s = (p ? &j->second : 0);
                        }
                        i = props_ref_map.insert(std::make_pair(event.xany.window,
                                                                EventWinPropsRef(s,p))).first;
                    }
                    props_ref = &i->second;
                    prev_win_id = event.xany.window;
                    prev_win_undef = false;
                }

                // Skip event if there is no associated event processor
                if (!props_ref->props)
                    continue;

                // Map the Xlib event into the intermediate representation,
                // EventEntry, with a smaller footprint. This also has the
                // advantage that Xlib need not be consulted in the further
                // procesing of the event.
                switch (event.type) {
                    case MotionNotify: {
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = ev_mousemove;
                        e.alt.mouse.x = event.xmotion.x;
                        e.alt.mouse.y = event.xmotion.y;
                        e.alt.mouse.time = event.xmotion.time;
                        break;
                    }
                    case ConfigureNotify: {
                        EventWinProps* s = props_ref->props;
                        if (event.xconfigure.width == s->width && event.xconfigure.height == s->height)
                            continue;
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = ev_resize;
                        e.alt.size.width  = s->width  = event.xconfigure.width;
                        e.alt.size.height = s->height = event.xconfigure.height;
                        break;
                    }
                    case Expose: {
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = ev_damage;
                        e.alt.area.x = event.xexpose.x;
                        e.alt.area.y = event.xexpose.y;
                        e.alt.area.width  = event.xexpose.width;
                        e.alt.area.height = event.xexpose.height;
                        break;
                    }
                    case VisibilityNotify: {
                        bool visible = event.xvisibility.state != VisibilityFullyObscured;
                        EventWinProps* s = props_ref->props;
                        if (visible == s->visible)
                            continue;
                        s->visible = visible;
                        if (!s->mapped)
                            continue;
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = visible ? ev_show : ev_hide;
                        break;
                    }
                    case MapNotify:
                    case UnmapNotify: {
                        bool mapped = (event.type == MapNotify);
                        EventWinProps* s = props_ref->props;
                        if (mapped == s->mapped)
                            continue;
                        s->mapped = mapped;
                        if (!s->visible)
                            continue;
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = mapped ? ev_show : ev_hide;
                        break;
                    }
                    case ClientMessage: {
                        bool is_close = (event.xclient.format == 32 &&
                                         static_cast<Atom>(event.xclient.data.l[0]) ==
                                         atom_del_win);
                        if (!is_close)
                            continue;
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = ev_close;
                        break;
                    }
                    case ButtonPress:
                    case ButtonRelease: {
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = event.type == ButtonPress ? ev_mousedown : ev_mouseup;
                        e.alt.mouse.x = event.xbutton.x;
                        e.alt.mouse.y = event.xbutton.y;
                        e.alt.mouse.button = event.xbutton.button;
                        e.alt.mouse.time = event.xbutton.time;
                        break;
                    }
                    case EnterNotify:
                    case LeaveNotify: {
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = event.type == EnterNotify ? ev_mouseover : ev_mouseout;
                        e.alt.time.time = event.xcrossing.time;
                        break;
                    }
                    case FocusIn:
                    case FocusOut: {
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = event.type == FocusIn ? ev_focus : ev_blur;
                        break;
                    }
                    case KeyPress:
                    case KeyRelease: {
                        if (entry.key_sym == NoSymbol)
                            continue; // No keysym defined for this key
                        bool press = event.type == KeyPress;
                        if (!press) {
                            // Filter out events from repeating keys. We cannot
                            // use XAutoRepeatOff/XAutoRepeatOn since they are
                            // not local to each window. The following trick is
                            // "lifted" from LibSDL. The trick assumes that
                            // whenever a key is repeating a KeyRelease event is
                            // immediately followed by the next KeyPress event
                            // on the queue without any intermediate events.
                            if (event_index+1 == s_xevent_buf_size) {
                                // We need to see the next event, so put the
                                // current one back, and ask for some more.
                                put_back_xevent(entry);
                                continue;
                            }
                            if (event_index+1 < events_read) {
                                XEvent& next = m_xevent_buf[event_index+1].xevent;
                                if (next.type == KeyPress &&
                                    next.xkey.keycode == event.xkey.keycode &&
                                    next.xkey.time - event.xkey.time < 2)
                                {
                                    // Repeating key detected, so skip this event and
                                    // the next one.
                                    ++event_index;
                                    continue;
                                }
                            }
                        }

                        // Xlibs KeyCodes (or scan codes) have the property of
                        // being small, therefore they can be used as indices
                        // into an array.
                        KeyStates::reference p = props_ref->props->key_states[event.xkey.keycode];
                        if (p == press)
                            continue; // Ignore events that does not change state
                        p = press; // Update state
                        EventSlot& e = props_ref->proc->get_event_slot(); // Throws
                        e.type = press ? ev_keydown : ev_keyup;
                        e.alt.key.key = entry.key_sym;
                        e.alt.key.time = event.xkey.time;
                        break;
                    }
                    default:
                        std::cerr << event.type << "\n";
                        continue;
                }

                // Go back and process the next event
                ++num_distrib;
            }

            if (!props_ref_map.empty()) {
                std::unique_lock<std::mutex> lock(event_proc_mutex, std::defer_lock);
                bool master_exit = false;
                for (const auto& entry: props_ref_map) {
                    EventProcessorImpl* p = entry.second.proc.get();
                    if (!p)
                        continue;
                    if (p == proc) { // To self - no need to lock the mutex
                        p->available += p->committed + p->uncommitted;
                        p->committed = p->uncommitted = 0;
                        if (p->available) {
                            if (!proc->first_buffer)
                                proc->first_buffer = proc->buffers.front().get();
                            master_exit = true;
                        }
                    }
                    else {
                        if (!lock)
                            lock.lock();
                        p->committed += p->uncommitted;
                        p->uncommitted = 0;
                        if (p->committed)
                            p->waiter_cond.notify_all();
                    }
                }
                if (master_exit) {
                    // Since we may have a lock on 'event_proc_mutex' we do not
                    // want the sentry to obtain one also.
                    if (!lock)
                        lock.lock();
                    sentry.reassign_caller_locked();
                    return;
                }
            }

            // Fall-through and wait for more events
        }

        // At this point we want to block waiting for more input, but to prevent
        // unnecessary output latencies, we choose to flush the output buffer
        // first.
        flush_output(); // Throws

        // We want to react to events as soon as they arrive on the network
        // connection. This can be achieved by using poll() on the file
        // descriptor associated with the network connection, however, this
        // method suffers from the following inherent race condition: An
        // asynchronous call to an Xlib function can read events from the
        // connection and enqueue them after we last checked the queue, but
        // before we start waiting for the connection to become readable. If
        // this happens, we might end up in a blocking wait event though events
        // are immediately available. To work around this problem, we never wait
        // for more than a 20th of a second before rechecking the event queue.
        using clock = std::chrono::steady_clock;
        using time_point = clock::time_point;
        using duration = clock::duration;
        duration max_poll_duration = std::chrono::milliseconds(50); // A 20th of a second
        if (timeout.time_since_epoch().count() != 0) {
            time_point now = clock::now();
            if (timeout <= now)
                return;
            duration d = timeout - now;
            if (d < max_poll_duration)
                max_poll_duration = d;
        }
        int max_poll_duration_msecs =
            std::chrono::duration_cast<std::chrono::milliseconds>(max_poll_duration).count();
        pollfd poll_slots[1] {};
        poll_slots[0].events = POLLIN;
        int ret = ::poll(poll_slots, 1, max_poll_duration_msecs);
        if (ARCHON_UNLIKELY(ret == -1)) {
            int err = errno; // Avoid clobering
            std::error_code ec(err, std::system_category());
            throw std::system_error(ec);
        }

        // Return to the beginning of the outer master loop
    }
}


/// Fill up the local event buffer, `m_xevent_buf`, with as many events from the
/// Xlib connection as are availble, and do this with as few read requests as
/// possible, and without flushing the output.
///
/// If this method returns a number that is less than `s_xevent_buf_size`, it
/// means that all available events have been read, and therefore this method
/// should not be called again imediately. On the other hand, if the returned
/// number is equal to `s_xevent_buf_size`, then more events may be immediately
/// available, and this method should be called again as soon as the previously
/// events have been processed.
///
/// The calling thread must be acting as master.
int ConnectionImpl::read_xevents()
{
    bool no_more = false;
    int used = 0, free = s_xevent_buf_size;
    if (m_have_xevent_put_back) {
        ++used;
        --free;
        m_have_xevent_put_back = false;
    }
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        read_xevents_helper(no_more, used, free); // Throws
    }
    return used;
}

void ConnectionImpl::read_xevents_helper(bool& no_more, int& used, int& free)
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              for (;;) {
                  bool full = (free <= m_unread_xevents);
                  int n = (full ? free : m_unread_xevents);
                  int m = used + n;
                  for (int i = used; i < m; ++i) {
                      EventAndKeySym& e =  m_xevent_buf[i];
                      XEvent& event = e.xevent;
                      XNextEvent(dpy, &event);
                      // Map KeyCodes to a keyboard independent symbol
                      // identifier (in general the symbol in the upper left
                      // corner on the corresponding key)
                      if (event.type == KeyPress || event.type == KeyRelease)
                          e.key_sym = XkbKeycodeToKeysym(dpy, event.xkey.keycode, XkbGroup1Index, 0);
                  }
                  used = m;
                  if (no_more || full) {
                      m_unread_xevents -= n;
                      break;
                  }
                  // `XEventsQueued(dpy, QueuedAfterReading)` performs a
                  // nonblocking read if `XEventsQueued(dpy, QueuedAlready)`
                  // would have returned zero.
                  m_unread_xevents = XEventsQueued(dpy, QueuedAfterReading);
                  if (m_unread_xevents == 0)
                      break; // Nothing more can be read at this time
                  // Keep track of how many events Xlib can read at a time,
                  // assuming there is a limit.
                  if (m_max_xevents_per_read < m_unread_xevents)
                      m_max_xevents_per_read = m_unread_xevents;
                  // If we got fewer events than what we know Xlib is capable of
                  // delivering, then we know we should not attempt another read
                  // at this time.
                  if (m_unread_xevents < m_max_xevents_per_read)
                      no_more = true;
                  free -= n;
              }
          });
}


/// This method must be called only by master.
void ConnectionImpl::put_back_xevent(const EventAndKeySym& entry)
{
    m_xevent_buf[0] = entry;
    m_have_xevent_put_back = true;
}




// Requires: No lock on 'm_screens_mutex' and no lock on `g_xlib_mutex`
const ConnectionImpl::ScreenSpec& ConnectionImpl::get_screen(int scr) const
{
    std::lock_guard<std::mutex> lock{m_screens_mutex};
    return get_screen_nlk(scr);
}

// Requires: Lock on 'm_screens_mutex' and no lock on `g_xlib_mutex`
ConnectionImpl::ScreenSpec& ConnectionImpl::get_screen_nlk(int scr) const
{
    if (scr < 0) {
        scr = default_screen;
    }
    else if (m_screens.size() <= std::size_t(scr)) {
        throw std::out_of_range("Screen index out of range");
    }
    ScreenSpec& s = m_screens[scr];
    if (s.index < 0) {
        VisualID default_id;
        int num_visuals;
        XVisualInfo* infos = nullptr;
        Xlib_Window root;
        int width, height, width_mm, height_mm;
        try {
            {
                std::lock_guard<std::mutex> lock{g_xlib_mutex};
                get_screen_helper(scr, default_id, num_visuals, infos, root, width, height,
                                  width_mm, height_mm); // Throws
            }
            if (!infos)
                throw std::runtime_error("Got no visuals");
            int default_vis = -1;
            for (int i = 0; i < num_visuals; ++i) {
                if (infos[i].visualid == default_id) {
                    default_vis = i;
                    break;
                }
            }
            if (default_vis < 0)
                throw std::runtime_error("Default visual not found");

            s.infos = infos;
            s.visual_specs.resize(num_visuals); // Throws
            s.root_win = root;
            s.default_visual = default_vis;
            s.width  = width;
            s.height = height;
            s.horiz_dot_pitch = double(width_mm)  / width  / 1000;
            s.vert_dot_pitch  = double(height_mm) / height / 1000;
            s.index = scr;
        }
        catch (...) {
            if (infos) {
                std::lock_guard<std::mutex> lock{g_xlib_mutex};
                GUARD(guard_state, XFree(infos)); // Throws
            }
            throw;
        }
    }
    return s;
}

void ConnectionImpl::get_screen_helper(int scr, VisualID& default_id, int& num_visuals,
                                       XVisualInfo*& infos, Xlib_Window& root, int& width,
                                       int& height, int& width_mm, int& height_mm) const
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              Xlib_Screen* t = ScreenOfDisplay(dpy, scr);
              default_id = XVisualIDFromVisual(DefaultVisualOfScreen(t));
              XVisualInfo criteria;
              criteria.screen = scr;
              infos = XGetVisualInfo(dpy, VisualScreenMask, &criteria, &num_visuals);
              root      = RootWindowOfScreen(t);
              width     = WidthOfScreen(t);
              height    = HeightOfScreen(t);
              width_mm  = WidthMMOfScreen(t);
              height_mm = HeightMMOfScreen(t);
          });
}

// Requires: No lock on 'm_screens_mutex' and no lock on `g_xlib_mutex`
const ConnectionImpl::VisualSpec& ConnectionImpl::get_visual(int scr, int vis) const
{
    std::lock_guard<std::mutex> lock{m_screens_mutex};
    const ScreenSpec& s = get_screen_nlk(scr);
    if (vis < 0) {
        vis = s.default_visual;
    }
    else if (s.visual_specs.size() <= std::size_t(vis)) {
        throw std::out_of_range("Visual index out of range");
    }
    VisualSpec& v = s.visual_specs[vis];
    if (v.index < 0) {
        XVisualInfo* info = s.infos + vis;
        v.info = info;
#ifdef ARCHON_HAVE_GLX
        if (have_glx) {
            int gl_support;
            bool error;
            {
                std::lock_guard<std::mutex> lock_2{g_xlib_mutex};
                get_visual_helper(info, gl_support, v, error); // Throws
            }
            if (error)
                throw std::runtime_error("glXGetConfig failed");
            v.gl_support = gl_support == False ? false : true;
        }
#endif

        // Prepare for pixel transcoding. May throw.
        ImageFormat::xvisual_format(this, info, v.image_format);

        v.screen = &s;
        v.index = vis;
    }
    return v;
}

#ifdef ARCHON_HAVE_GLX
void ConnectionImpl::get_visual_helper(XVisualInfo* info, int& gl_support, VisualSpec& v,
                                       bool& error) const
{
    GUARD(guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              error =
                  glXGetConfig(dpy, info, GLX_USE_GL,           &gl_support)          ||
                  glXGetConfig(dpy, info, GLX_RED_SIZE,         &v.width_red)         ||
                  glXGetConfig(dpy, info, GLX_GREEN_SIZE,       &v.width_green)       ||
                  glXGetConfig(dpy, info, GLX_BLUE_SIZE,        &v.width_blue)        ||
                  glXGetConfig(dpy, info, GLX_ALPHA_SIZE,       &v.width_alpha)       ||
                  glXGetConfig(dpy, info, GLX_DEPTH_SIZE,       &v.width_depth)       ||
                  glXGetConfig(dpy, info, GLX_STENCIL_SIZE,     &v.width_stencil)     ||
                  glXGetConfig(dpy, info, GLX_ACCUM_RED_SIZE,   &v.width_accum_red)   ||
                  glXGetConfig(dpy, info, GLX_ACCUM_GREEN_SIZE, &v.width_accum_green) ||
                  glXGetConfig(dpy, info, GLX_ACCUM_BLUE_SIZE,  &v.width_accum_blue)  ||
                  glXGetConfig(dpy, info, GLX_ACCUM_ALPHA_SIZE, &v.width_accum_alpha);
          });
}
#endif




ConnectionImpl::ConnectionImpl(Xlib_Display* d):
    dpy(d),
    m_xevent_buf(std::make_unique<EventAndKeySym[]>(s_xevent_buf_size)) // Throws
{
    int conn_file_des, num_screens;
    XPixmapFormatValues* volatile formats = 0; // Volatile due to long-jump
    try {
        // Allow the error handlers to access the long-jump state of
        // this connection
        {
            std::lock_guard<std::mutex> lock{g_connections_mutex};
            g_connections[dpy] = this;
        }

        {
            int num_formats;
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(guard_state, // Throws
                  {
                      // CAUTION: Before changing anything in this block, read
                      // the comments about GUARD at the start of this file.
                      default_screen    = DefaultScreen(dpy);
                      num_screens       = ScreenCount(dpy);
                      conn_file_des     = ConnectionNumber(dpy);
                      image_byte_order  = ImageByteOrder(dpy);
                      bitmap_bit_order  = BitmapBitOrder(dpy);
                      bitmap_pad        = BitmapPad(dpy);
                      bitmap_unit       = BitmapUnit(dpy);
                      atom_del_win      = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
                      atom_net_wm_state = XInternAtom(dpy, "_NET_WM_STATE",    False);
                      atom_net_wm_state_fullscreen =
                          XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
                      formats = XListPixmapFormats(dpy, &num_formats);
                  });
            if (!atom_del_win || !atom_net_wm_state || !atom_net_wm_state_fullscreen)
                throw std::runtime_error("XInternAtom failed");

            if (!formats)
                throw std::runtime_error("XListPixmapFormats failed");

            for (int i = 0; i < num_formats; ++i) {
                XPixmapFormatValues* f = formats + i;
                image_formats.insert(std::make_pair(f->depth,
                                                    ImgFmtDetail(f->bits_per_pixel,
                                                                 f->scanline_pad))); // Throws
            }

            {
                XPixmapFormatValues* f = formats;
                formats = 0; // Don't try to deallocate again
                GUARD(guard_state, XFree(f)); // Throws
            }

#ifdef ARCHON_HAVE_GLX
            GUARD(guard_state, // Throws
                  {
                      int dummy;
                      if (glXQueryExtension(dpy, &dummy, &dummy))
                          have_glx = true;
                  });
#endif

#ifdef ARCHON_HAVE_XRENDER
            GUARD(guard_state, // Throws
                  {
                      int dummy;
                      if (XRenderQueryExtension(dpy, &dummy, &dummy)) {
                          int major;
                          int minor;
                          if (XRenderQueryVersion(dpy, &major, &minor)) {
                              if ((major == 0 && 5 <= minor) || major > 0)
                                  have_xrender = true;
                          }
                      }
                  });
#endif

#ifdef ARCHON_HAVE_XINPUT2
            bool xi_query_version_failed = false;
            int xi_major_opcode, xi_first_event, xi_first_error;
            GUARD(guard_state, // Throws
                  {
                      bool have = XQueryExtension(dpy, "XInputExtension", &xi_major_opcode,
                                                  &xi_first_event, &xi_first_error);
                      if (have) {
                          int major = 2;
                          int minor = 0;
                          Status status = XIQueryVersion(dpy, &major, &minor);
                          if (status != Success) {
                              if (status != BadRequest)
                                  xi_query_version_failed = true;
                          }
                          else {
                              have_xinput2 = true;
                          }
                      }
                  });
            if (xi_query_version_failed)
                throw std::runtime_error("XIQueryVersion() failed");
#endif
        }

        m_conn_file_des = conn_file_des;
        m_screens.resize(num_screens);
    }
    catch (...) {
        try {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(guard_state, // Throws
                  {
                      if (formats)
                          XFree(formats);
                      XCloseDisplay(dpy);
                  });
        }
        catch (BadConnectionException&) {
            // Don't care
        }

        {
            std::lock_guard<std::mutex> lock{g_connections_mutex};
            g_connections.erase(dpy);
        }

        throw;
    }
}


ConnectionImpl::~ConnectionImpl()
{
    try {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block,
                  // read the comments about GUARD at the start of this
                  // file.
                  for (ScreenSpec& s: m_screens) {
                      if (s.infos)
                          XFree(s.infos);
                  }
                  XCloseDisplay(dpy);
              });
    }
    catch (BadConnectionException&) {
        // Don't care
    }

    {
        std::lock_guard<std::mutex> lock{g_connections_mutex};
        g_connections.erase(dpy);
    }
}




inline ConnectionImpl::EventWinProps::EventWinProps(EventProcessorImpl* p, int cookie):
    proc(p->weak_self),
    cookie(cookie)
{
}




void DrawableImpl::put_image(Image::ConstRefArg img, Box clip, Point position, PackedTRGB background)
{
    ImageReader reader(Oper::flip(img, false, true)); // Upside down
    reader.set_background_color(background);

    const ConnectionImpl::VisualSpec& v = conn->get_visual(scr, vis);
    XImage ximg;
    int tray_width  = 64;
    int tray_height = 64;
    Image::Ref tray = v.image_format.setup_transcode(ximg, tray_width, tray_height);
    ImageWriter writer(tray);

    int width  = clip.width;
    int height = clip.height;
    int cols = (width  + tray_width  - 1) / tray_width;
    int rows = (height + tray_height - 1) / tray_height;

    int img_offset_x = clip.x - position.x;
    int img_offset_y = clip.y - position.y;

    Xlib_Drawable drw = get_xlib_drawable();

    bool init_failed = false, gc_failed = false;
    bool first_block = true;
    GC gc = None;
    try {
        for (int i = 0; i < rows; ++i) {
            bool last_row = (i == rows - 1);
            int y = i * tray_height;
            int h = last_row ? height - y : tray_height;
            for (int j = 0; j < cols; ++j) {
                bool last_col = (j == cols - 1);
                int x = j * tray_width;
                int w = last_col ? width - x : tray_width;

                reader.set_pos(img_offset_x + x, img_offset_y + y);
                writer.put_image(reader, w, h);

                {
                    std::lock_guard<std::mutex> lock{g_xlib_mutex};
                    put_image_helper(drw, first_block, last_row && last_col,
                                     clip.x + x, clip.y + y, w, h, ximg,
                                     init_failed, gc_failed, gc); // Throws
                }

                if (first_block) {
                    if (init_failed)
                        throw std::runtime_error("XInitImage failed");
                    if (gc_failed)
                        throw std::runtime_error("XCreateGC failed");
                    first_block = false;
                }
            }
        }
    }
    catch (BadConnectionException&) {
        // Connection is bad, but it could be non-fatal, so we should
        // still attempt to clean up.
        {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, // Throws
                  {
                      if (gc != None)
                          XFreeGC(dpy, gc);
                  });
        }
        throw;
    }
}


void DrawableImpl::put_image_helper(Xlib_Drawable drw, bool first_block, bool last_block,
                                    int x, int y, int w, int h, XImage& ximg,
                                    bool& init_failed, bool& gc_failed, GC& gc)
{
    GUARD(conn->guard_state, // Throws
          {
              // CAUTION: Before changing anything in this block, read the
              // comments about GUARD at the start of this file.
              if (first_block) {
                  if (XInitImage(&ximg)) {
                      gc = XCreateGC(dpy, drw, 0, 0);
                      if (gc == None)
                          gc_failed = true;
                  }
                  else {
                      init_failed = true;
                  }
              }
              if (gc != None) {
                  XPutImage(dpy, drw, gc, &ximg, 0, 0, x, y, w, h);
                  if (last_block) {
                      GC g = gc;
                      gc = None;
                      XFreeGC(dpy, g);
                  }
              }
          });
}




void WindowImpl::set_title(std::string new_title)
{
    unsigned char* volatile p = 0; // Volatile due to long-jump
    const char* t = new_title.c_str();
    try {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read
                  // the comments about GUARD at the start of this file.
                  XTextProperty text_prop;
                  XStringListToTextProperty(const_cast<char**>(&t), 1, &text_prop);
                  p = text_prop.value;
                  XSetWMName(dpy, win, &text_prop);
                  XSetWMIconName(dpy, win, &text_prop);
                  p = 0;
                  XFree(text_prop.value);
              });
    }
    catch (...) {
        if (p) {
            std::lock_guard<std::mutex> lock{g_xlib_mutex};
            GUARD(conn->guard_state, XFree(p)); // Throws
        }
        throw;
    }
}


/// \todo FIXME: Should the allocated color be freed when a new color is set?
void WindowImpl::set_bg_color(long rgb)
{
    int red   = rgb >> 16 & 0xFF;
    int green = rgb >>  8 & 0xFF;
    int blue  = rgb >>  0 & 0xFF;

    XColor col;
    col.red   = frac_adjust_bit_width(red,   8, 16);
    col.green = frac_adjust_bit_width(green, 8, 16);
    col.blue  = frac_adjust_bit_width(blue,  8, 16);

    bool good;
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read
                  // the comments about GUARD at the start of this file.
                  good = XAllocColor(dpy, colmap, &col);
                  if (good) {
                      XSetWindowAttributes swa;
                      swa.background_pixel = col.pixel;
                      XChangeWindowAttributes(dpy, win, CWBackPixel, &swa);
                  }
              });
    }
    if (!good)
        throw std::runtime_error("WindowImpl::set_bg_color: Could not allocate color");
}


void WindowImpl::set_cursor(Arch_Cursor& c)
{
    CursorImpl* cursor = dynamic_cast<CursorImpl*>(&c);
    if (!cursor)
        throw std::invalid_argument("Implementation mismatch while setting cursor");

    if (cursor->conn != conn)
        throw std::invalid_argument("Connection mismatch while setting cursor");

    if (cursor->scr != scr)
        throw std::invalid_argument("Screen mismatch while setting cursor");

    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, XDefineCursor(dpy, win, cursor->cursor)); // Throws
    }
}


void WindowImpl::reset_cursor()
{
    std::lock_guard<std::mutex> lock{g_xlib_mutex};
    GUARD(conn->guard_state, XUndefineCursor(dpy, win)); // Throws
}


/// This way of triggering fullscreen mode was 'lifted' from gtk+-2.16.1
/// (gdk/x11/gdkwindow-x11.c:4528). Nice code - thanks.
void WindowImpl::set_fullscreen_enabled(bool enable)
{
    XClientMessageEvent msg;
    {
        char* p = reinterpret_cast<char*>(&msg);
        std::fill(p, p+sizeof msg, 0);
    }
    msg.type = ClientMessage;
    msg.window = win;
    msg.message_type = conn->atom_net_wm_state;
    msg.format = 32;
    msg.data.l[0] = (enable ? 1 : 0);
    msg.data.l[1] = conn->atom_net_wm_state_fullscreen;

    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read the
                  // comments about GUARD at the start of this file.
                  XSendEvent(dpy, RootWindow(dpy,scr), False,
                             SubstructureRedirectMask | SubstructureNotifyMask,
                             reinterpret_cast<XEvent*>(&msg));
              });
    }

    // The following snippet was taken from
    // http://tonyobryan.com/index.php?article=9. It instructs the window
    // manager to remove decorations. The problem with this method is that the
    // window gets stuck in the fullscreen mode, and I cannot get it restored to
    // a normal window.
/*
    unsigned long data[5] = { 2, 0, enable?0:1 };
    if (!XChangeProperty(dpy, win, intern_atom("_MOTIF_WM_HINTS"), intern_atom("_MOTIF_WM_HINTS"), 32,
                         PropModeReplace, reinterpret_cast<unsigned char *>(data), 5))
        throw std::runtime_error("set_fullscreen_enabled: XChangeProperty failed");
*/

    // This is how FreeGLUT does it. Problem with this method is that it neither
    // removes window decorations, nor does it expand the window byond the
    // 'free' destop area.
/*
    int x, y;
    Xlib_Window w;
    // Triggers the window manager (GNOME) to enter fullscreen mode, because the position is (0,0) and the size is equal to the screen size
    XMoveResizeWindow(dpy, win, 0, 0, scr_width, scr_height);
    XFlush(dpy); // This is needed
    XTranslateCoordinates(dpy, win, RootWindow(dpy, scr), 0, 0, &x, &y, &w);
    if (x||y) {
        XMoveWindow(dpy, win, -x, -y);
        XFlush(dpy); // XXX Shouldn't need this
    }
*/
    // Interestingly, if the two failed methods above are combined (tonyobryan +
    // GLUT) then the window really goes into fullscreen mode and is stripped
    // off its decorations, but it is stuck there, and cannot be restored.
}


WindowImpl::~WindowImpl()
{
    try {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read the
                  // comments about GUARD at the start of this file.
                  XDestroyWindow(dpy, win);
                  XFreeColormap(dpy, colmap);
              });
    }
    catch (BadConnectionException&) {
        // Don't care
    }
    SharedPtr<EventProcessorImpl> p = m_event_proc.lock();
    if (p)
        p->unregister_window(win);
}


// Called only by EventProcessor::register_window. Returns false iff window
// currently has an associaten with an event processor.
bool WindowImpl::set_event_proc(EventProcessorImpl* p)
{
    std::lock_guard<std::mutex> lock{m_events_mutex};
    if (m_events_enabled)
        return false;
    update_xlib_event_mask(true, m_mouse_motion_always); // Throws
    m_event_proc = p->weak_self;
    m_events_enabled = true;
    return true;
}


// Called only by ~EventProcessor.
void WindowImpl::unset_event_proc()
{
    std::lock_guard<std::mutex> lock{m_events_mutex};
    m_event_proc.reset();
    update_xlib_event_mask(false, m_mouse_motion_always); // Throws
    m_events_enabled = false;
}





void EventProcessorImpl::process(std::chrono::steady_clock::time_point timeout)
{
    struct SlotReleaser {
        ConnectionImpl& conn;
        EventProcessorImpl& proc;
        SlotReleaser(ConnectionImpl& c, EventProcessorImpl& p) noexcept:
            conn{c},
            proc{p}
        {
        }
        ~SlotReleaser() noexcept
        {
            if (++proc.first_available_slot == s_slots_per_buf) {
                // Switch to next buffer
                {
                    std::lock_guard<std::mutex> lock{conn.event_proc_mutex};
                    std::unique_ptr<EventSlot[]> buf = std::move(proc.buffers.front());
                    proc.buffers.pop_front();
                    try {
                        conn.event_proc_free_buffers.push_back(std::move(buf));
                    }
                    catch (std::bad_alloc&) {
                        // FIXME: Maybe avoid the possibility of an exception
                        // here by preallocating space in
                        // conn.event_proc_free_buffers for a sufficient number
                        // of free buffers.
                    }
                    proc.first_buffer = proc.buffers.empty() ? nullptr : proc.buffers.front().get();
                }
                proc.first_available_slot = 0;
            }
            --proc.available;
        }
    };

    for (;;) {
        bool proceed = handler->before_sleep(); // Throws
        if (!proceed)
            break;

        conn->receive_events(this, timeout); // Throws
        if (!available)
            break; // Timed out

        do {
            SlotReleaser i(*conn, *this);
            EventSlot& e = first_buffer[first_available_slot];
            switch (e.type) {
                case ev_mousemove: {
                    auto ev = make_event<MouseEvent>(e.cookie, map_time(e.alt.mouse.time),
                                                     e.alt.mouse.x, e.alt.mouse.y);
                    handler->on_mousemove(std::move(ev));
                    break;
                }
                case ev_resize: {
                    auto ev = make_event<SizeEvent>(e.cookie, e.alt.size.width, e.alt.size.height);
                    handler->on_resize(std::move(ev));
                    break;
                }
                case ev_damage: {
                    auto ev = make_event<AreaEvent>(e.cookie, e.alt.area.x, e.alt.area.y,
                                                    e.alt.area.width, e.alt.area.height);
                    handler->on_damage(std::move(ev));
                    break;
                }
                case ev_mousedown: {
                    auto ev = make_event<MouseButtonEvent>(e.cookie, map_time(e.alt.mouse.time),
                                                           e.alt.mouse.x, e.alt.mouse.y,
                                                           e.alt.mouse.button);
                    handler->on_mousedown(std::move(ev));
                    break;
                }
                case ev_mouseup: {
                    auto ev = make_event<MouseButtonEvent>(e.cookie, map_time(e.alt.mouse.time),
                                                           e.alt.mouse.x, e.alt.mouse.y,
                                                           e.alt.mouse.button);
                    handler->on_mouseup(std::move(ev));
                    break;
                }
                case ev_keydown: {
                    auto ev = make_event<KeyEvent>(e.cookie, map_time(e.alt.key.time),
                                                   map_keysym(e.alt.key.key));
                    handler->on_keydown(std::move(ev));
                    break;
                }
                case ev_keyup: {
                    auto ev = make_event<KeyEvent>(e.cookie, map_time(e.alt.key.time),
                                                   map_keysym(e.alt.key.key));
                    handler->on_keyup(std::move(ev));
                    break;
                }
                case ev_mouseover: {
                    auto ev = make_event<TimedEvent>(e.cookie, map_time(e.alt.time.time));
                    handler->on_mouseover(std::move(ev));
                    break;
                }
                case ev_mouseout: {
                    auto ev = make_event<TimedEvent>(e.cookie, map_time(e.alt.time.time));
                    handler->on_mouseout(std::move(ev));
                    break;
                }
                case ev_focus:
                    handler->on_focus(make_event<Event>(e.cookie));
                    break;

                case ev_blur:
                    handler->on_blur(make_event<Event>(e.cookie));
                    break;

                case ev_show:
                    handler->on_show(make_event<Event>(e.cookie));
                    break;

                case ev_hide:
                    handler->on_hide(make_event<Event>(e.cookie));
                    break;

                case ev_close:
                    handler->on_close(make_event<Event>(e.cookie));
                    break;
            }
        }
        while (available);
    }
}



void EventProcessorImpl::get_key_sym_names(const std::vector<Arch_KeySym>& key_syms,
                                           std::vector<std::string>& names)
{
    std::size_t n = key_syms.size();
    std::vector<Xlib_KeySym> xlib_v{n};
    for (std::size_t i = 0; i < n; ++i)
        xlib_v[i] = conn->keysym_mapper.archon_to_xlib(key_syms[i]);
    std::vector<std::string> name_v{n};
    {
        std::lock_guard<std::mutex> lock{g_xlib_mutex};
        GUARD(conn->guard_state, // Throws
              {
                  // CAUTION: Before changing anything in this block, read
                  // the comments about GUARD at the start of this file.
                  for (std::size_t i = 0; i < n; ++i) {
                      char* p = XKeysymToString(xlib_v[i]);
                      if (p)
                          name_v[i] = p;
                  }
              });
    }
    names = name_v;
}




KeySymMapper::KeySymMapper()
{
    // TTY Functions
    add(XK_BackSpace,                   KeySym_BackSpace);
    add(XK_Tab,                         KeySym_Tab);
    add(XK_Linefeed,                    KeySym_Linefeed);
    add(XK_Clear,                       KeySym_Clear);
    add(XK_Return,                      KeySym_Return);
    add(XK_Pause,                       KeySym_Pause);
    add(XK_Scroll_Lock,                 KeySym_Scroll_Lock);
    add(XK_Sys_Req,                     KeySym_Sys_Req);
    add(XK_Escape,                      KeySym_Escape);
    add(XK_Delete,                      KeySym_Delete);

    // International & multi-key character composition
    add(XK_Multi_key,                   KeySym_Multi_key);
    add(XK_Codeinput,                   KeySym_Codeinput);
    add(XK_SingleCandidate,             KeySym_SingleCandidate);
    add(XK_MultipleCandidate,           KeySym_MultipleCandidate);
    add(XK_PreviousCandidate,           KeySym_PreviousCandidate);

    // Japanese keyboard support
    add(XK_Kanji,                       KeySym_Kanji);
    add(XK_Muhenkan,                    KeySym_Muhenkan);
    add(XK_Henkan_Mode,                 KeySym_Henkan_Mode);
    add(XK_Romaji,                      KeySym_Romaji);
    add(XK_Hiragana,                    KeySym_Hiragana);
    add(XK_Katakana,                    KeySym_Katakana);
    add(XK_Hiragana_Katakana,           KeySym_Hiragana_Katakana);
    add(XK_Zenkaku,                     KeySym_Zenkaku);
    add(XK_Hankaku,                     KeySym_Hankaku);
    add(XK_Zenkaku_Hankaku,             KeySym_Zenkaku_Hankaku);
    add(XK_Touroku,                     KeySym_Touroku);
    add(XK_Massyo,                      KeySym_Massyo);
    add(XK_Kana_Lock,                   KeySym_Kana_Lock);
    add(XK_Kana_Shift,                  KeySym_Kana_Shift);
    add(XK_Eisu_Shift,                  KeySym_Eisu_Shift);
    add(XK_Eisu_toggle,                 KeySym_Eisu_toggle);

    // Cursor control & motion
    add(XK_Home,                        KeySym_Home);
    add(XK_Left,                        KeySym_Left);
    add(XK_Up,                          KeySym_Up);
    add(XK_Right,                       KeySym_Right);
    add(XK_Down,                        KeySym_Down);
    add(XK_Prior,                       KeySym_Prior);
    add(XK_Next,                        KeySym_Next);
    add(XK_End,                         KeySym_End);
    add(XK_Begin,                       KeySym_Begin);

    // Misc Functions
    add(XK_Select,                      KeySym_Select);
    add(XK_Print,                       KeySym_Print);
    add(XK_Execute,                     KeySym_Execute);
    add(XK_Insert,                      KeySym_Insert);
    add(XK_Undo,                        KeySym_Undo);
    add(XK_Redo,                        KeySym_Redo);
    add(XK_Menu,                        KeySym_Menu);
    add(XK_Find,                        KeySym_Find);
    add(XK_Cancel,                      KeySym_Cancel);
    add(XK_Help,                        KeySym_Help);
    add(XK_Break,                       KeySym_Break);
    add(XK_Mode_switch,                 KeySym_Mode_switch);
    add(XK_Num_Lock,                    KeySym_Num_Lock);

    // Keypad Functions
    add(XK_KP_Space,                    KeySym_KP_Space);
    add(XK_KP_Tab,                      KeySym_KP_Tab);
    add(XK_KP_Enter,                    KeySym_KP_Enter);
    add(XK_KP_F1,                       KeySym_KP_F1);
    add(XK_KP_F2,                       KeySym_KP_F2);
    add(XK_KP_F3,                       KeySym_KP_F3);
    add(XK_KP_F4,                       KeySym_KP_F4);
    add(XK_KP_Home,                     KeySym_KP_Home);
    add(XK_KP_Left,                     KeySym_KP_Left);
    add(XK_KP_Up,                       KeySym_KP_Up);
    add(XK_KP_Right,                    KeySym_KP_Right);
    add(XK_KP_Down,                     KeySym_KP_Down);
    add(XK_KP_Prior,                    KeySym_KP_Prior);
    add(XK_KP_Next,                     KeySym_KP_Next);
    add(XK_KP_End,                      KeySym_KP_End);
    add(XK_KP_Begin,                    KeySym_KP_Begin);
    add(XK_KP_Insert,                   KeySym_KP_Insert);
    add(XK_KP_Delete,                   KeySym_KP_Delete);
    add(XK_KP_Equal,                    KeySym_KP_Equal);
    add(XK_KP_Multiply,                 KeySym_KP_Multiply);
    add(XK_KP_Add,                      KeySym_KP_Add);
    add(XK_KP_Separator,                KeySym_KP_Separator);
    add(XK_KP_Subtract,                 KeySym_KP_Subtract);
    add(XK_KP_Decimal,                  KeySym_KP_Decimal);
    add(XK_KP_Divide,                   KeySym_KP_Divide);

    add(XK_KP_0,                        KeySym_KP_0);
    add(XK_KP_1,                        KeySym_KP_1);
    add(XK_KP_2,                        KeySym_KP_2);
    add(XK_KP_3,                        KeySym_KP_3);
    add(XK_KP_4,                        KeySym_KP_4);
    add(XK_KP_5,                        KeySym_KP_5);
    add(XK_KP_6,                        KeySym_KP_6);
    add(XK_KP_7,                        KeySym_KP_7);
    add(XK_KP_8,                        KeySym_KP_8);
    add(XK_KP_9,                        KeySym_KP_9);

    // Auxilliary Functions
    add(XK_F1,                          KeySym_F1);
    add(XK_F2,                          KeySym_F2);
    add(XK_F3,                          KeySym_F3);
    add(XK_F4,                          KeySym_F4);
    add(XK_F5,                          KeySym_F5);
    add(XK_F6,                          KeySym_F6);
    add(XK_F7,                          KeySym_F7);
    add(XK_F8,                          KeySym_F8);
    add(XK_F9,                          KeySym_F9);
    add(XK_F10,                         KeySym_F10);
    add(XK_F11,                         KeySym_F11);
    add(XK_F12,                         KeySym_F12);
    add(XK_F13,                         KeySym_F13);
    add(XK_F14,                         KeySym_F14);
    add(XK_F15,                         KeySym_F15);
    add(XK_F16,                         KeySym_F16);
    add(XK_F17,                         KeySym_F17);
    add(XK_F18,                         KeySym_F18);
    add(XK_F19,                         KeySym_F19);
    add(XK_F20,                         KeySym_F20);
    add(XK_F21,                         KeySym_F21);
    add(XK_F22,                         KeySym_F22);
    add(XK_F23,                         KeySym_F23);
    add(XK_F24,                         KeySym_F24);
    add(XK_F25,                         KeySym_F25);
    add(XK_F26,                         KeySym_F26);
    add(XK_F27,                         KeySym_F27);
    add(XK_F28,                         KeySym_F28);
    add(XK_F29,                         KeySym_F29);
    add(XK_F30,                         KeySym_F30);
    add(XK_F31,                         KeySym_F31);
    add(XK_F32,                         KeySym_F32);
    add(XK_F33,                         KeySym_F33);
    add(XK_F34,                         KeySym_F34);
    add(XK_F35,                         KeySym_F35);

    // Modifiers
    add(XK_Shift_L,                     KeySym_Shift_L);
    add(XK_Shift_R,                     KeySym_Shift_R);
    add(XK_Control_L,                   KeySym_Control_L);
    add(XK_Control_R,                   KeySym_Control_R);
    add(XK_Caps_Lock,                   KeySym_Caps_Lock);
    add(XK_Shift_Lock,                  KeySym_Shift_Lock);

    add(XK_Meta_L,                      KeySym_Meta_L);
    add(XK_Meta_R,                      KeySym_Meta_R);
    add(XK_Alt_L,                       KeySym_Alt_L);
    add(XK_Alt_R,                       KeySym_Alt_R);
    add(XK_Super_L,                     KeySym_Super_L);
    add(XK_Super_R,                     KeySym_Super_R);
    add(XK_Hyper_L,                     KeySym_Hyper_L);
    add(XK_Hyper_R,                     KeySym_Hyper_R);

    // ISO 9995 Function and Modifier Keys
    add(XK_ISO_Lock,                    KeySym_ISO_Lock);
    add(XK_ISO_Level2_Latch,            KeySym_ISO_Level2_Latch);
    add(XK_ISO_Level3_Shift,            KeySym_ISO_Level3_Shift);
    add(XK_ISO_Level3_Latch,            KeySym_ISO_Level3_Latch);
    add(XK_ISO_Level3_Lock,             KeySym_ISO_Level3_Lock);
    add(XK_ISO_Group_Latch,             KeySym_ISO_Group_Latch);
    add(XK_ISO_Group_Lock,              KeySym_ISO_Group_Lock);
    add(XK_ISO_Next_Group,              KeySym_ISO_Next_Group);
    add(XK_ISO_Next_Group_Lock,         KeySym_ISO_Next_Group_Lock);
    add(XK_ISO_Prev_Group,              KeySym_ISO_Prev_Group);
    add(XK_ISO_Prev_Group_Lock,         KeySym_ISO_Prev_Group_Lock);
    add(XK_ISO_First_Group,             KeySym_ISO_First_Group);
    add(XK_ISO_First_Group_Lock,        KeySym_ISO_First_Group_Lock);
    add(XK_ISO_Last_Group,              KeySym_ISO_Last_Group);
    add(XK_ISO_Last_Group_Lock,         KeySym_ISO_Last_Group_Lock);

    add(XK_ISO_Left_Tab,                KeySym_ISO_Left_Tab);
    add(XK_ISO_Move_Line_Up,            KeySym_ISO_Move_Line_Up);
    add(XK_ISO_Move_Line_Down,          KeySym_ISO_Move_Line_Down);
    add(XK_ISO_Partial_Line_Up,         KeySym_ISO_Partial_Line_Up);
    add(XK_ISO_Partial_Line_Down,       KeySym_ISO_Partial_Line_Down);
    add(XK_ISO_Partial_Space_Left,      KeySym_ISO_Partial_Space_Left);
    add(XK_ISO_Partial_Space_Right,     KeySym_ISO_Partial_Space_Right);
    add(XK_ISO_Set_Margin_Left,         KeySym_ISO_Set_Margin_Left);
    add(XK_ISO_Set_Margin_Right,        KeySym_ISO_Set_Margin_Right);
    add(XK_ISO_Release_Margin_Left,     KeySym_ISO_Release_Margin_Left);
    add(XK_ISO_Release_Margin_Right,    KeySym_ISO_Release_Margin_Right);
    add(XK_ISO_Release_Both_Margins,    KeySym_ISO_Release_Both_Margins);
    add(XK_ISO_Fast_Cursor_Left,        KeySym_ISO_Fast_Cursor_Left);
    add(XK_ISO_Fast_Cursor_Right,       KeySym_ISO_Fast_Cursor_Right);
    add(XK_ISO_Fast_Cursor_Up,          KeySym_ISO_Fast_Cursor_Up);
    add(XK_ISO_Fast_Cursor_Down,        KeySym_ISO_Fast_Cursor_Down);
    add(XK_ISO_Continuous_Underline,    KeySym_ISO_Continuous_Underline);
    add(XK_ISO_Discontinuous_Underline, KeySym_ISO_Discontinuous_Underline);
    add(XK_ISO_Emphasize,               KeySym_ISO_Emphasize);
    add(XK_ISO_Center_Object,           KeySym_ISO_Center_Object);
    add(XK_ISO_Enter,                   KeySym_ISO_Enter);

    add(XK_dead_grave,                  KeySym_dead_grave);
    add(XK_dead_acute,                  KeySym_dead_acute);
    add(XK_dead_circumflex,             KeySym_dead_circumflex);
    add(XK_dead_tilde,                  KeySym_dead_tilde);
    add(XK_dead_macron,                 KeySym_dead_macron);
    add(XK_dead_breve,                  KeySym_dead_breve);
    add(XK_dead_abovedot,               KeySym_dead_abovedot);
    add(XK_dead_diaeresis,              KeySym_dead_diaeresis);
    add(XK_dead_abovering,              KeySym_dead_abovering);
    add(XK_dead_doubleacute,            KeySym_dead_doubleacute);
    add(XK_dead_caron,                  KeySym_dead_caron);
    add(XK_dead_cedilla,                KeySym_dead_cedilla);
    add(XK_dead_ogonek,                 KeySym_dead_ogonek);
    add(XK_dead_iota,                   KeySym_dead_iota);
    add(XK_dead_voiced_sound,           KeySym_dead_voiced_sound);
    add(XK_dead_semivoiced_sound,       KeySym_dead_semivoiced_sound);
    add(XK_dead_belowdot,               KeySym_dead_belowdot);
    add(XK_dead_hook,                   KeySym_dead_hook);
    add(XK_dead_horn,                   KeySym_dead_horn);

    add(XK_First_Virtual_Screen,        KeySym_First_Virtual_Screen);
    add(XK_Prev_Virtual_Screen,         KeySym_Prev_Virtual_Screen);
    add(XK_Next_Virtual_Screen,         KeySym_Next_Virtual_Screen);
    add(XK_Last_Virtual_Screen,         KeySym_Last_Virtual_Screen);
    add(XK_Terminate_Server,            KeySym_Terminate_Server);

    add(XK_AccessX_Enable,              KeySym_AccessX_Enable);
    add(XK_AccessX_Feedback_Enable,     KeySym_AccessX_Feedback_Enable);
    add(XK_RepeatKeys_Enable,           KeySym_RepeatKeys_Enable);
    add(XK_SlowKeys_Enable,             KeySym_SlowKeys_Enable);
    add(XK_BounceKeys_Enable,           KeySym_BounceKeys_Enable);
    add(XK_StickyKeys_Enable,           KeySym_StickyKeys_Enable);
    add(XK_MouseKeys_Enable,            KeySym_MouseKeys_Enable);
    add(XK_MouseKeys_Accel_Enable,      KeySym_MouseKeys_Accel_Enable);
    add(XK_Overlay1_Enable,             KeySym_Overlay1_Enable);
    add(XK_Overlay2_Enable,             KeySym_Overlay2_Enable);
    add(XK_AudibleBell_Enable,          KeySym_AudibleBell_Enable);

    add(XK_Pointer_Left,                KeySym_Pointer_Left);
    add(XK_Pointer_Right,               KeySym_Pointer_Right);
    add(XK_Pointer_Up,                  KeySym_Pointer_Up);
    add(XK_Pointer_Down,                KeySym_Pointer_Down);
    add(XK_Pointer_UpLeft,              KeySym_Pointer_UpLeft);
    add(XK_Pointer_UpRight,             KeySym_Pointer_UpRight);
    add(XK_Pointer_DownLeft,            KeySym_Pointer_DownLeft);
    add(XK_Pointer_DownRight,           KeySym_Pointer_DownRight);
    add(XK_Pointer_Button_Dflt,         KeySym_Pointer_Button_Dflt);
    add(XK_Pointer_Button1,             KeySym_Pointer_Button1);
    add(XK_Pointer_Button2,             KeySym_Pointer_Button2);
    add(XK_Pointer_Button3,             KeySym_Pointer_Button3);
    add(XK_Pointer_Button4,             KeySym_Pointer_Button4);
    add(XK_Pointer_Button5,             KeySym_Pointer_Button5);
    add(XK_Pointer_DblClick_Dflt,       KeySym_Pointer_DblClick_Dflt);
    add(XK_Pointer_DblClick1,           KeySym_Pointer_DblClick1);
    add(XK_Pointer_DblClick2,           KeySym_Pointer_DblClick2);
    add(XK_Pointer_DblClick3,           KeySym_Pointer_DblClick3);
    add(XK_Pointer_DblClick4,           KeySym_Pointer_DblClick4);
    add(XK_Pointer_DblClick5,           KeySym_Pointer_DblClick5);
    add(XK_Pointer_Drag_Dflt,           KeySym_Pointer_Drag_Dflt);
    add(XK_Pointer_Drag1,               KeySym_Pointer_Drag1);
    add(XK_Pointer_Drag2,               KeySym_Pointer_Drag2);
    add(XK_Pointer_Drag3,               KeySym_Pointer_Drag3);
    add(XK_Pointer_Drag4,               KeySym_Pointer_Drag4);
    add(XK_Pointer_Drag5,               KeySym_Pointer_Drag5);

    add(XK_Pointer_EnableKeys,          KeySym_Pointer_EnableKeys);
    add(XK_Pointer_Accelerate,          KeySym_Pointer_Accelerate);
    add(XK_Pointer_DfltBtnNext,         KeySym_Pointer_DfltBtnNext);
    add(XK_Pointer_DfltBtnPrev,         KeySym_Pointer_DfltBtnPrev);

/*
    // 3270 Terminal Keys
    add(XK_3270_Duplicate,              KeySym_3270_Duplicate);
    add(XK_3270_FieldMark,              KeySym_3270_FieldMark);
    add(XK_3270_Right2,                 KeySym_3270_Right2);
    add(XK_3270_Left2,                  KeySym_3270_Left2);
    add(XK_3270_BackTab,                KeySym_3270_BackTab);
    add(XK_3270_EraseEOF,               KeySym_3270_EraseEOF);
    add(XK_3270_EraseInput,             KeySym_3270_EraseInput);
    add(XK_3270_Reset,                  KeySym_3270_Reset);
    add(XK_3270_Quit,                   KeySym_3270_Quit);
    add(XK_3270_PA1,                    KeySym_3270_PA1);
    add(XK_3270_PA2,                    KeySym_3270_PA2);
    add(XK_3270_PA3,                    KeySym_3270_PA3);
    add(XK_3270_Test,                   KeySym_3270_Test);
    add(XK_3270_Attn,                   KeySym_3270_Attn);
    add(XK_3270_CursorBlink,            KeySym_3270_CursorBlink);
    add(XK_3270_AltCursor,              KeySym_3270_AltCursor);
    add(XK_3270_KeyClick,               KeySym_3270_KeyClick);
    add(XK_3270_Jump,                   KeySym_3270_Jump);
    add(XK_3270_Ident,                  KeySym_3270_Ident);
    add(XK_3270_Rule,                   KeySym_3270_Rule);
    add(XK_3270_Copy,                   KeySym_3270_Copy);
    add(XK_3270_Play,                   KeySym_3270_Play);
    add(XK_3270_Setup,                  KeySym_3270_Setup);
    add(XK_3270_Record,                 KeySym_3270_Record);
    add(XK_3270_ChangeScreen,           KeySym_3270_ChangeScreen);
    add(XK_3270_DeleteWord,             KeySym_3270_DeleteWord);
    add(XK_3270_ExSelect,               KeySym_3270_ExSelect);
    add(XK_3270_CursorSelect,           KeySym_3270_CursorSelect);
    add(XK_3270_PrintScreen,            KeySym_3270_PrintScreen);
    add(XK_3270_Enter,                  KeySym_3270_Enter);
*/

    // Latin 1
    add(XK_space,                       KeySym_space);
    add(XK_exclam,                      KeySym_exclam);
    add(XK_quotedbl,                    KeySym_quotedbl);
    add(XK_numbersign,                  KeySym_numbersign);
    add(XK_dollar,                      KeySym_dollar);
    add(XK_percent,                     KeySym_percent);
    add(XK_ampersand,                   KeySym_ampersand);
    add(XK_apostrophe,                  KeySym_apostrophe);
    add(XK_parenleft,                   KeySym_parenleft);
    add(XK_parenright,                  KeySym_parenright);
    add(XK_asterisk,                    KeySym_asterisk);
    add(XK_plus,                        KeySym_plus);
    add(XK_comma,                       KeySym_comma);
    add(XK_minus,                       KeySym_minus);
    add(XK_period,                      KeySym_period);
    add(XK_slash,                       KeySym_slash);
    add(XK_0,                           KeySym_0);
    add(XK_1,                           KeySym_1);
    add(XK_2,                           KeySym_2);
    add(XK_3,                           KeySym_3);
    add(XK_4,                           KeySym_4);
    add(XK_5,                           KeySym_5);
    add(XK_6,                           KeySym_6);
    add(XK_7,                           KeySym_7);
    add(XK_8,                           KeySym_8);
    add(XK_9,                           KeySym_9);
    add(XK_colon,                       KeySym_colon);
    add(XK_semicolon,                   KeySym_semicolon);
    add(XK_less,                        KeySym_less);
    add(XK_equal,                       KeySym_equal);
    add(XK_greater,                     KeySym_greater);
    add(XK_question,                    KeySym_question);
    add(XK_at,                          KeySym_at);
    add(XK_A,                           KeySym_A);
    add(XK_B,                           KeySym_B);
    add(XK_C,                           KeySym_C);
    add(XK_D,                           KeySym_D);
    add(XK_E,                           KeySym_E);
    add(XK_F,                           KeySym_F);
    add(XK_G,                           KeySym_G);
    add(XK_H,                           KeySym_H);
    add(XK_I,                           KeySym_I);
    add(XK_J,                           KeySym_J);
    add(XK_K,                           KeySym_K);
    add(XK_L,                           KeySym_L);
    add(XK_M,                           KeySym_M);
    add(XK_N,                           KeySym_N);
    add(XK_O,                           KeySym_O);
    add(XK_P,                           KeySym_P);
    add(XK_Q,                           KeySym_Q);
    add(XK_R,                           KeySym_R);
    add(XK_S,                           KeySym_S);
    add(XK_T,                           KeySym_T);
    add(XK_U,                           KeySym_U);
    add(XK_V,                           KeySym_V);
    add(XK_W,                           KeySym_W);
    add(XK_X,                           KeySym_X);
    add(XK_Y,                           KeySym_Y);
    add(XK_Z,                           KeySym_Z);
    add(XK_bracketleft,                 KeySym_bracketleft);
    add(XK_backslash,                   KeySym_backslash);
    add(XK_bracketright,                KeySym_bracketright);
    add(XK_asciicircum,                 KeySym_asciicircum);
    add(XK_underscore,                  KeySym_underscore);
    add(XK_grave,                       KeySym_grave);
    add(XK_a,                           KeySym_a);
    add(XK_b,                           KeySym_b);
    add(XK_c,                           KeySym_c);
    add(XK_d,                           KeySym_d);
    add(XK_e,                           KeySym_e);
    add(XK_f,                           KeySym_f);
    add(XK_g,                           KeySym_g);
    add(XK_h,                           KeySym_h);
    add(XK_i,                           KeySym_i);
    add(XK_j,                           KeySym_j);
    add(XK_k,                           KeySym_k);
    add(XK_l,                           KeySym_l);
    add(XK_m,                           KeySym_m);
    add(XK_n,                           KeySym_n);
    add(XK_o,                           KeySym_o);
    add(XK_p,                           KeySym_p);
    add(XK_q,                           KeySym_q);
    add(XK_r,                           KeySym_r);
    add(XK_s,                           KeySym_s);
    add(XK_t,                           KeySym_t);
    add(XK_u,                           KeySym_u);
    add(XK_v,                           KeySym_v);
    add(XK_w,                           KeySym_w);
    add(XK_x,                           KeySym_x);
    add(XK_y,                           KeySym_y);
    add(XK_z,                           KeySym_z);
    add(XK_braceleft,                   KeySym_braceleft);
    add(XK_bar,                         KeySym_bar);
    add(XK_braceright,                  KeySym_braceright);
    add(XK_asciitilde,                  KeySym_asciitilde);

    add(XK_nobreakspace,                KeySym_nobreakspace);
    add(XK_exclamdown,                  KeySym_exclamdown);
    add(XK_cent,                        KeySym_cent);
    add(XK_sterling,                    KeySym_sterling);
    add(XK_currency,                    KeySym_currency);
    add(XK_yen,                         KeySym_yen);
    add(XK_brokenbar,                   KeySym_brokenbar);
    add(XK_section,                     KeySym_section);
    add(XK_diaeresis,                   KeySym_diaeresis);
    add(XK_copyright,                   KeySym_copyright);
    add(XK_ordfeminine,                 KeySym_ordfeminine);
    add(XK_guillemotleft,               KeySym_guillemotleft);
    add(XK_notsign,                     KeySym_notsign);
    add(XK_hyphen,                      KeySym_hyphen);
    add(XK_registered,                  KeySym_registered);
    add(XK_macron,                      KeySym_macron);
    add(XK_degree,                      KeySym_degree);
    add(XK_plusminus,                   KeySym_plusminus);
    add(XK_twosuperior,                 KeySym_twosuperior);
    add(XK_threesuperior,               KeySym_threesuperior);
    add(XK_acute,                       KeySym_acute);
    add(XK_mu,                          KeySym_mu);
    add(XK_paragraph,                   KeySym_paragraph);
    add(XK_periodcentered,              KeySym_periodcentered);
    add(XK_cedilla,                     KeySym_cedilla);
    add(XK_onesuperior,                 KeySym_onesuperior);
    add(XK_masculine,                   KeySym_masculine);
    add(XK_guillemotright,              KeySym_guillemotright);
    add(XK_onequarter,                  KeySym_onequarter);
    add(XK_onehalf,                     KeySym_onehalf);
    add(XK_threequarters,               KeySym_threequarters);
    add(XK_questiondown,                KeySym_questiondown);
    add(XK_Agrave,                      KeySym_Agrave);
    add(XK_Aacute,                      KeySym_Aacute);
    add(XK_Acircumflex,                 KeySym_Acircumflex);
    add(XK_Atilde,                      KeySym_Atilde);
    add(XK_Adiaeresis,                  KeySym_Adiaeresis);
    add(XK_Aring,                       KeySym_Aring);
    add(XK_AE,                          KeySym_AE);
    add(XK_Ccedilla,                    KeySym_Ccedilla);
    add(XK_Egrave,                      KeySym_Egrave);
    add(XK_Eacute,                      KeySym_Eacute);
    add(XK_Ecircumflex,                 KeySym_Ecircumflex);
    add(XK_Ediaeresis,                  KeySym_Ediaeresis);
    add(XK_Igrave,                      KeySym_Igrave);
    add(XK_Iacute,                      KeySym_Iacute);
    add(XK_Icircumflex,                 KeySym_Icircumflex);
    add(XK_Idiaeresis,                  KeySym_Idiaeresis);
    add(XK_ETH,                         KeySym_ETH);
    add(XK_Ntilde,                      KeySym_Ntilde);
    add(XK_Ograve,                      KeySym_Ograve);
    add(XK_Oacute,                      KeySym_Oacute);
    add(XK_Ocircumflex,                 KeySym_Ocircumflex);
    add(XK_Otilde,                      KeySym_Otilde);
    add(XK_Odiaeresis,                  KeySym_Odiaeresis);
    add(XK_multiply,                    KeySym_multiply);
    add(XK_Ooblique,                    KeySym_Ooblique);
    add(XK_Ugrave,                      KeySym_Ugrave);
    add(XK_Uacute,                      KeySym_Uacute);
    add(XK_Ucircumflex,                 KeySym_Ucircumflex);
    add(XK_Udiaeresis,                  KeySym_Udiaeresis);
    add(XK_Yacute,                      KeySym_Yacute);
    add(XK_THORN,                       KeySym_THORN);
    add(XK_ssharp,                      KeySym_ssharp);
    add(XK_agrave,                      KeySym_agrave);
    add(XK_aacute,                      KeySym_aacute);
    add(XK_acircumflex,                 KeySym_acircumflex);
    add(XK_atilde,                      KeySym_atilde);
    add(XK_adiaeresis,                  KeySym_adiaeresis);
    add(XK_aring,                       KeySym_aring);
    add(XK_ae,                          KeySym_ae);
    add(XK_ccedilla,                    KeySym_ccedilla);
    add(XK_egrave,                      KeySym_egrave);
    add(XK_eacute,                      KeySym_eacute);
    add(XK_ecircumflex,                 KeySym_ecircumflex);
    add(XK_ediaeresis,                  KeySym_ediaeresis);
    add(XK_igrave,                      KeySym_igrave);
    add(XK_iacute,                      KeySym_iacute);
    add(XK_icircumflex,                 KeySym_icircumflex);
    add(XK_idiaeresis,                  KeySym_idiaeresis);
    add(XK_eth,                         KeySym_eth);
    add(XK_ntilde,                      KeySym_ntilde);
    add(XK_ograve,                      KeySym_ograve);
    add(XK_oacute,                      KeySym_oacute);
    add(XK_ocircumflex,                 KeySym_ocircumflex);
    add(XK_otilde,                      KeySym_otilde);
    add(XK_odiaeresis,                  KeySym_odiaeresis);
    add(XK_division,                    KeySym_division);
    add(XK_ooblique,                    KeySym_ooblique);
    add(XK_ugrave,                      KeySym_ugrave);
    add(XK_uacute,                      KeySym_uacute);
    add(XK_ucircumflex,                 KeySym_ucircumflex);
    add(XK_udiaeresis,                  KeySym_udiaeresis);
    add(XK_yacute,                      KeySym_yacute);
    add(XK_thorn,                       KeySym_thorn);
    add(XK_ydiaeresis,                  KeySym_ydiaeresis);

    // Latin 2
    add(XK_Aogonek,                     KeySym_Aogonek);
    add(XK_breve,                       KeySym_breve);
    add(XK_Lstroke,                     KeySym_Lstroke);
    add(XK_Lcaron,                      KeySym_Lcaron);
    add(XK_Sacute,                      KeySym_Sacute);
    add(XK_Scaron,                      KeySym_Scaron);
    add(XK_Scedilla,                    KeySym_Scedilla);
    add(XK_Tcaron,                      KeySym_Tcaron);
    add(XK_Zacute,                      KeySym_Zacute);
    add(XK_Zcaron,                      KeySym_Zcaron);
    add(XK_Zabovedot,                   KeySym_Zabovedot);
    add(XK_aogonek,                     KeySym_aogonek);
    add(XK_ogonek,                      KeySym_ogonek);
    add(XK_lstroke,                     KeySym_lstroke);
    add(XK_lcaron,                      KeySym_lcaron);
    add(XK_sacute,                      KeySym_sacute);
    add(XK_caron,                       KeySym_caron);
    add(XK_scaron,                      KeySym_scaron);
    add(XK_scedilla,                    KeySym_scedilla);
    add(XK_tcaron,                      KeySym_tcaron);
    add(XK_zacute,                      KeySym_zacute);
    add(XK_doubleacute,                 KeySym_doubleacute);
    add(XK_zcaron,                      KeySym_zcaron);
    add(XK_zabovedot,                   KeySym_zabovedot);
    add(XK_Racute,                      KeySym_Racute);
    add(XK_Abreve,                      KeySym_Abreve);
    add(XK_Lacute,                      KeySym_Lacute);
    add(XK_Cacute,                      KeySym_Cacute);
    add(XK_Ccaron,                      KeySym_Ccaron);
    add(XK_Eogonek,                     KeySym_Eogonek);
    add(XK_Ecaron,                      KeySym_Ecaron);
    add(XK_Dcaron,                      KeySym_Dcaron);
    add(XK_Dstroke,                     KeySym_Dstroke);
    add(XK_Nacute,                      KeySym_Nacute);
    add(XK_Ncaron,                      KeySym_Ncaron);
    add(XK_Odoubleacute,                KeySym_Odoubleacute);
    add(XK_Rcaron,                      KeySym_Rcaron);
    add(XK_Uring,                       KeySym_Uring);
    add(XK_Udoubleacute,                KeySym_Udoubleacute);
    add(XK_Tcedilla,                    KeySym_Tcedilla);
    add(XK_racute,                      KeySym_racute);
    add(XK_abreve,                      KeySym_abreve);
    add(XK_lacute,                      KeySym_lacute);
    add(XK_cacute,                      KeySym_cacute);
    add(XK_ccaron,                      KeySym_ccaron);
    add(XK_eogonek,                     KeySym_eogonek);
    add(XK_ecaron,                      KeySym_ecaron);
    add(XK_dcaron,                      KeySym_dcaron);
    add(XK_dstroke,                     KeySym_dstroke);
    add(XK_nacute,                      KeySym_nacute);
    add(XK_ncaron,                      KeySym_ncaron);
    add(XK_odoubleacute,                KeySym_odoubleacute);
    add(XK_udoubleacute,                KeySym_udoubleacute);
    add(XK_rcaron,                      KeySym_rcaron);
    add(XK_uring,                       KeySym_uring);
    add(XK_tcedilla,                    KeySym_tcedilla);
    add(XK_abovedot,                    KeySym_abovedot);

    // Latin 3
    add(XK_Hstroke,                     KeySym_Hstroke);
    add(XK_Hcircumflex,                 KeySym_Hcircumflex);
    add(XK_Iabovedot,                   KeySym_Iabovedot);
    add(XK_Gbreve,                      KeySym_Gbreve);
    add(XK_Jcircumflex,                 KeySym_Jcircumflex);
    add(XK_hstroke,                     KeySym_hstroke);
    add(XK_hcircumflex,                 KeySym_hcircumflex);
    add(XK_idotless,                    KeySym_idotless);
    add(XK_gbreve,                      KeySym_gbreve);
    add(XK_jcircumflex,                 KeySym_jcircumflex);
    add(XK_Cabovedot,                   KeySym_Cabovedot);
    add(XK_Ccircumflex,                 KeySym_Ccircumflex);
    add(XK_Gabovedot,                   KeySym_Gabovedot);
    add(XK_Gcircumflex,                 KeySym_Gcircumflex);
    add(XK_Ubreve,                      KeySym_Ubreve);
    add(XK_Scircumflex,                 KeySym_Scircumflex);
    add(XK_cabovedot,                   KeySym_cabovedot);
    add(XK_ccircumflex,                 KeySym_ccircumflex);
    add(XK_gabovedot,                   KeySym_gabovedot);
    add(XK_gcircumflex,                 KeySym_gcircumflex);
    add(XK_ubreve,                      KeySym_ubreve);
    add(XK_scircumflex,                 KeySym_scircumflex);

    // Latin 4
    add(XK_kra,                         KeySym_kra);
    add(XK_Rcedilla,                    KeySym_Rcedilla);
    add(XK_Itilde,                      KeySym_Itilde);
    add(XK_Lcedilla,                    KeySym_Lcedilla);
    add(XK_Emacron,                     KeySym_Emacron);
    add(XK_Gcedilla,                    KeySym_Gcedilla);
    add(XK_Tslash,                      KeySym_Tslash);
    add(XK_rcedilla,                    KeySym_rcedilla);
    add(XK_itilde,                      KeySym_itilde);
    add(XK_lcedilla,                    KeySym_lcedilla);
    add(XK_emacron,                     KeySym_emacron);
    add(XK_gcedilla,                    KeySym_gcedilla);
    add(XK_tslash,                      KeySym_tslash);
    add(XK_ENG,                         KeySym_ENG);
    add(XK_eng,                         KeySym_eng);
    add(XK_Amacron,                     KeySym_Amacron);
    add(XK_Iogonek,                     KeySym_Iogonek);
    add(XK_Eabovedot,                   KeySym_Eabovedot);
    add(XK_Imacron,                     KeySym_Imacron);
    add(XK_Ncedilla,                    KeySym_Ncedilla);
    add(XK_Omacron,                     KeySym_Omacron);
    add(XK_Kcedilla,                    KeySym_Kcedilla);
    add(XK_Uogonek,                     KeySym_Uogonek);
    add(XK_Utilde,                      KeySym_Utilde);
    add(XK_Umacron,                     KeySym_Umacron);
    add(XK_amacron,                     KeySym_amacron);
    add(XK_iogonek,                     KeySym_iogonek);
    add(XK_eabovedot,                   KeySym_eabovedot);
    add(XK_imacron,                     KeySym_imacron);
    add(XK_ncedilla,                    KeySym_ncedilla);
    add(XK_omacron,                     KeySym_omacron);
    add(XK_kcedilla,                    KeySym_kcedilla);
    add(XK_uogonek,                     KeySym_uogonek);
    add(XK_utilde,                      KeySym_utilde);
    add(XK_umacron,                     KeySym_umacron);

    // Latin-8
    add(XK_Babovedot,                   KeySym_Babovedot);
    add(XK_babovedot,                   KeySym_babovedot);
    add(XK_Dabovedot,                   KeySym_Dabovedot);
    add(XK_Wgrave,                      KeySym_Wgrave);
    add(XK_Wacute,                      KeySym_Wacute);
    add(XK_dabovedot,                   KeySym_dabovedot);
    add(XK_Ygrave,                      KeySym_Ygrave);
    add(XK_Fabovedot,                   KeySym_Fabovedot);
    add(XK_fabovedot,                   KeySym_fabovedot);
    add(XK_Mabovedot,                   KeySym_Mabovedot);
    add(XK_mabovedot,                   KeySym_mabovedot);
    add(XK_Pabovedot,                   KeySym_Pabovedot);
    add(XK_wgrave,                      KeySym_wgrave);
    add(XK_pabovedot,                   KeySym_pabovedot);
    add(XK_wacute,                      KeySym_wacute);
    add(XK_Sabovedot,                   KeySym_Sabovedot);
    add(XK_ygrave,                      KeySym_ygrave);
    add(XK_Wdiaeresis,                  KeySym_Wdiaeresis);
    add(XK_wdiaeresis,                  KeySym_wdiaeresis);
    add(XK_sabovedot,                   KeySym_sabovedot);
    add(XK_Wcircumflex,                 KeySym_Wcircumflex);
    add(XK_Tabovedot,                   KeySym_Tabovedot);
    add(XK_Ycircumflex,                 KeySym_Ycircumflex);
    add(XK_wcircumflex,                 KeySym_wcircumflex);
    add(XK_tabovedot,                   KeySym_tabovedot);
    add(XK_ycircumflex,                 KeySym_ycircumflex);

    // Latin-9 (a.k.a. Latin-0)
    add(XK_OE,                          KeySym_OE);
    add(XK_oe,                          KeySym_oe);
    add(XK_Ydiaeresis,                  KeySym_Ydiaeresis);

    // Katakana
    add(XK_overline,                    KeySym_overline);
    add(XK_kana_fullstop,               KeySym_kana_fullstop);
    add(XK_kana_openingbracket,         KeySym_kana_openingbracket);
    add(XK_kana_closingbracket,         KeySym_kana_closingbracket);
    add(XK_kana_comma,                  KeySym_kana_comma);
    add(XK_kana_conjunctive,            KeySym_kana_conjunctive);
    add(XK_kana_WO,                     KeySym_kana_WO);
    add(XK_kana_a,                      KeySym_kana_a);
    add(XK_kana_i,                      KeySym_kana_i);
    add(XK_kana_u,                      KeySym_kana_u);
    add(XK_kana_e,                      KeySym_kana_e);
    add(XK_kana_o,                      KeySym_kana_o);
    add(XK_kana_ya,                     KeySym_kana_ya);
    add(XK_kana_yu,                     KeySym_kana_yu);
    add(XK_kana_yo,                     KeySym_kana_yo);
    add(XK_kana_tsu,                    KeySym_kana_tsu);
    add(XK_prolongedsound,              KeySym_prolongedsound);
    add(XK_kana_A,                      KeySym_kana_A);
    add(XK_kana_I,                      KeySym_kana_I);
    add(XK_kana_U,                      KeySym_kana_U);
    add(XK_kana_E,                      KeySym_kana_E);
    add(XK_kana_O,                      KeySym_kana_O);
    add(XK_kana_KA,                     KeySym_kana_KA);
    add(XK_kana_KI,                     KeySym_kana_KI);
    add(XK_kana_KU,                     KeySym_kana_KU);
    add(XK_kana_KE,                     KeySym_kana_KE);
    add(XK_kana_KO,                     KeySym_kana_KO);
    add(XK_kana_SA,                     KeySym_kana_SA);
    add(XK_kana_SHI,                    KeySym_kana_SHI);
    add(XK_kana_SU,                     KeySym_kana_SU);
    add(XK_kana_SE,                     KeySym_kana_SE);
    add(XK_kana_SO,                     KeySym_kana_SO);
    add(XK_kana_TA,                     KeySym_kana_TA);
    add(XK_kana_CHI,                    KeySym_kana_CHI);
    add(XK_kana_TSU,                    KeySym_kana_TSU);
    add(XK_kana_TE,                     KeySym_kana_TE);
    add(XK_kana_TO,                     KeySym_kana_TO);
    add(XK_kana_NA,                     KeySym_kana_NA);
    add(XK_kana_NI,                     KeySym_kana_NI);
    add(XK_kana_NU,                     KeySym_kana_NU);
    add(XK_kana_NE,                     KeySym_kana_NE);
    add(XK_kana_NO,                     KeySym_kana_NO);
    add(XK_kana_HA,                     KeySym_kana_HA);
    add(XK_kana_HI,                     KeySym_kana_HI);
    add(XK_kana_FU,                     KeySym_kana_FU);
    add(XK_kana_HE,                     KeySym_kana_HE);
    add(XK_kana_HO,                     KeySym_kana_HO);
    add(XK_kana_MA,                     KeySym_kana_MA);
    add(XK_kana_MI,                     KeySym_kana_MI);
    add(XK_kana_MU,                     KeySym_kana_MU);
    add(XK_kana_ME,                     KeySym_kana_ME);
    add(XK_kana_MO,                     KeySym_kana_MO);
    add(XK_kana_YA,                     KeySym_kana_YA);
    add(XK_kana_YU,                     KeySym_kana_YU);
    add(XK_kana_YO,                     KeySym_kana_YO);
    add(XK_kana_RA,                     KeySym_kana_RA);
    add(XK_kana_RI,                     KeySym_kana_RI);
    add(XK_kana_RU,                     KeySym_kana_RU);
    add(XK_kana_RE,                     KeySym_kana_RE);
    add(XK_kana_RO,                     KeySym_kana_RO);
    add(XK_kana_WA,                     KeySym_kana_WA);
    add(XK_kana_N,                      KeySym_kana_N);
    add(XK_voicedsound,                 KeySym_voicedsound);
    add(XK_semivoicedsound,             KeySym_semivoicedsound);

    // Arabic
    add(XK_Farsi_0,                     KeySym_Farsi_0);
    add(XK_Farsi_1,                     KeySym_Farsi_1);
    add(XK_Farsi_2,                     KeySym_Farsi_2);
    add(XK_Farsi_3,                     KeySym_Farsi_3);
    add(XK_Farsi_4,                     KeySym_Farsi_4);
    add(XK_Farsi_5,                     KeySym_Farsi_5);
    add(XK_Farsi_6,                     KeySym_Farsi_6);
    add(XK_Farsi_7,                     KeySym_Farsi_7);
    add(XK_Farsi_8,                     KeySym_Farsi_8);
    add(XK_Farsi_9,                     KeySym_Farsi_9);
    add(XK_Arabic_percent,              KeySym_Arabic_percent);
    add(XK_Arabic_superscript_alef,     KeySym_Arabic_superscript_alef);
    add(XK_Arabic_tteh,                 KeySym_Arabic_tteh);
    add(XK_Arabic_peh,                  KeySym_Arabic_peh);
    add(XK_Arabic_tcheh,                KeySym_Arabic_tcheh);
    add(XK_Arabic_ddal,                 KeySym_Arabic_ddal);
    add(XK_Arabic_rreh,                 KeySym_Arabic_rreh);
    add(XK_Arabic_comma,                KeySym_Arabic_comma);
    add(XK_Arabic_fullstop,             KeySym_Arabic_fullstop);
    add(XK_Arabic_0,                    KeySym_Arabic_0);
    add(XK_Arabic_1,                    KeySym_Arabic_1);
    add(XK_Arabic_2,                    KeySym_Arabic_2);
    add(XK_Arabic_3,                    KeySym_Arabic_3);
    add(XK_Arabic_4,                    KeySym_Arabic_4);
    add(XK_Arabic_5,                    KeySym_Arabic_5);
    add(XK_Arabic_6,                    KeySym_Arabic_6);
    add(XK_Arabic_7,                    KeySym_Arabic_7);
    add(XK_Arabic_8,                    KeySym_Arabic_8);
    add(XK_Arabic_9,                    KeySym_Arabic_9);
    add(XK_Arabic_semicolon,            KeySym_Arabic_semicolon);
    add(XK_Arabic_question_mark,        KeySym_Arabic_question_mark);
    add(XK_Arabic_hamza,                KeySym_Arabic_hamza);
    add(XK_Arabic_maddaonalef,          KeySym_Arabic_maddaonalef);
    add(XK_Arabic_hamzaonalef,          KeySym_Arabic_hamzaonalef);
    add(XK_Arabic_hamzaonwaw,           KeySym_Arabic_hamzaonwaw);
    add(XK_Arabic_hamzaunderalef,       KeySym_Arabic_hamzaunderalef);
    add(XK_Arabic_hamzaonyeh,           KeySym_Arabic_hamzaonyeh);
    add(XK_Arabic_alef,                 KeySym_Arabic_alef);
    add(XK_Arabic_beh,                  KeySym_Arabic_beh);
    add(XK_Arabic_tehmarbuta,           KeySym_Arabic_tehmarbuta);
    add(XK_Arabic_teh,                  KeySym_Arabic_teh);
    add(XK_Arabic_theh,                 KeySym_Arabic_theh);
    add(XK_Arabic_jeem,                 KeySym_Arabic_jeem);
    add(XK_Arabic_hah,                  KeySym_Arabic_hah);
    add(XK_Arabic_khah,                 KeySym_Arabic_khah);
    add(XK_Arabic_dal,                  KeySym_Arabic_dal);
    add(XK_Arabic_thal,                 KeySym_Arabic_thal);
    add(XK_Arabic_ra,                   KeySym_Arabic_ra);
    add(XK_Arabic_zain,                 KeySym_Arabic_zain);
    add(XK_Arabic_seen,                 KeySym_Arabic_seen);
    add(XK_Arabic_sheen,                KeySym_Arabic_sheen);
    add(XK_Arabic_sad,                  KeySym_Arabic_sad);
    add(XK_Arabic_dad,                  KeySym_Arabic_dad);
    add(XK_Arabic_tah,                  KeySym_Arabic_tah);
    add(XK_Arabic_zah,                  KeySym_Arabic_zah);
    add(XK_Arabic_ain,                  KeySym_Arabic_ain);
    add(XK_Arabic_ghain,                KeySym_Arabic_ghain);
    add(XK_Arabic_tatweel,              KeySym_Arabic_tatweel);
    add(XK_Arabic_feh,                  KeySym_Arabic_feh);
    add(XK_Arabic_qaf,                  KeySym_Arabic_qaf);
    add(XK_Arabic_kaf,                  KeySym_Arabic_kaf);
    add(XK_Arabic_lam,                  KeySym_Arabic_lam);
    add(XK_Arabic_meem,                 KeySym_Arabic_meem);
    add(XK_Arabic_noon,                 KeySym_Arabic_noon);
    add(XK_Arabic_ha,                   KeySym_Arabic_ha);
    add(XK_Arabic_waw,                  KeySym_Arabic_waw);
    add(XK_Arabic_alefmaksura,          KeySym_Arabic_alefmaksura);
    add(XK_Arabic_yeh,                  KeySym_Arabic_yeh);
    add(XK_Arabic_fathatan,             KeySym_Arabic_fathatan);
    add(XK_Arabic_dammatan,             KeySym_Arabic_dammatan);
    add(XK_Arabic_kasratan,             KeySym_Arabic_kasratan);
    add(XK_Arabic_fatha,                KeySym_Arabic_fatha);
    add(XK_Arabic_damma,                KeySym_Arabic_damma);
    add(XK_Arabic_kasra,                KeySym_Arabic_kasra);
    add(XK_Arabic_shadda,               KeySym_Arabic_shadda);
    add(XK_Arabic_sukun,                KeySym_Arabic_sukun);
    add(XK_Arabic_madda_above,          KeySym_Arabic_madda_above);
    add(XK_Arabic_hamza_above,          KeySym_Arabic_hamza_above);
    add(XK_Arabic_hamza_below,          KeySym_Arabic_hamza_below);
    add(XK_Arabic_jeh,                  KeySym_Arabic_jeh);
    add(XK_Arabic_veh,                  KeySym_Arabic_veh);
    add(XK_Arabic_keheh,                KeySym_Arabic_keheh);
    add(XK_Arabic_gaf,                  KeySym_Arabic_gaf);
    add(XK_Arabic_noon_ghunna,          KeySym_Arabic_noon_ghunna);
    add(XK_Arabic_heh_doachashmee,      KeySym_Arabic_heh_doachashmee);
    add(XK_Farsi_yeh,                   KeySym_Farsi_yeh);
    add(XK_Arabic_yeh_baree,            KeySym_Arabic_yeh_baree);
    add(XK_Arabic_heh_goal,             KeySym_Arabic_heh_goal);

    // Cyrillic
    add(XK_Cyrillic_GHE_bar,            KeySym_Cyrillic_GHE_bar);
    add(XK_Cyrillic_ghe_bar,            KeySym_Cyrillic_ghe_bar);
    add(XK_Cyrillic_ZHE_descender,      KeySym_Cyrillic_ZHE_descender);
    add(XK_Cyrillic_zhe_descender,      KeySym_Cyrillic_zhe_descender);
    add(XK_Cyrillic_KA_descender,       KeySym_Cyrillic_KA_descender);
    add(XK_Cyrillic_ka_descender,       KeySym_Cyrillic_ka_descender);
    add(XK_Cyrillic_KA_vertstroke,      KeySym_Cyrillic_KA_vertstroke);
    add(XK_Cyrillic_ka_vertstroke,      KeySym_Cyrillic_ka_vertstroke);
    add(XK_Cyrillic_EN_descender,       KeySym_Cyrillic_EN_descender);
    add(XK_Cyrillic_en_descender,       KeySym_Cyrillic_en_descender);
    add(XK_Cyrillic_U_straight,         KeySym_Cyrillic_U_straight);
    add(XK_Cyrillic_u_straight,         KeySym_Cyrillic_u_straight);
    add(XK_Cyrillic_U_straight_bar,     KeySym_Cyrillic_U_straight_bar);
    add(XK_Cyrillic_u_straight_bar,     KeySym_Cyrillic_u_straight_bar);
    add(XK_Cyrillic_HA_descender,       KeySym_Cyrillic_HA_descender);
    add(XK_Cyrillic_ha_descender,       KeySym_Cyrillic_ha_descender);
    add(XK_Cyrillic_CHE_descender,      KeySym_Cyrillic_CHE_descender);
    add(XK_Cyrillic_che_descender,      KeySym_Cyrillic_che_descender);
    add(XK_Cyrillic_CHE_vertstroke,     KeySym_Cyrillic_CHE_vertstroke);
    add(XK_Cyrillic_che_vertstroke,     KeySym_Cyrillic_che_vertstroke);
    add(XK_Cyrillic_SHHA,               KeySym_Cyrillic_SHHA);
    add(XK_Cyrillic_shha,               KeySym_Cyrillic_shha);

    add(XK_Cyrillic_SCHWA,              KeySym_Cyrillic_SCHWA);
    add(XK_Cyrillic_schwa,              KeySym_Cyrillic_schwa);
    add(XK_Cyrillic_I_macron,           KeySym_Cyrillic_I_macron);
    add(XK_Cyrillic_i_macron,           KeySym_Cyrillic_i_macron);
    add(XK_Cyrillic_O_bar,              KeySym_Cyrillic_O_bar);
    add(XK_Cyrillic_o_bar,              KeySym_Cyrillic_o_bar);
    add(XK_Cyrillic_U_macron,           KeySym_Cyrillic_U_macron);
    add(XK_Cyrillic_u_macron,           KeySym_Cyrillic_u_macron);

    add(XK_Serbian_dje,                 KeySym_Serbian_dje);
    add(XK_Macedonia_gje,               KeySym_Macedonia_gje);
    add(XK_Cyrillic_io,                 KeySym_Cyrillic_io);
    add(XK_Ukrainian_ie,                KeySym_Ukrainian_ie);
    add(XK_Macedonia_dse,               KeySym_Macedonia_dse);
    add(XK_Ukrainian_i,                 KeySym_Ukrainian_i);
    add(XK_Ukrainian_yi,                KeySym_Ukrainian_yi);
    add(XK_Cyrillic_je,                 KeySym_Cyrillic_je);
    add(XK_Cyrillic_lje,                KeySym_Cyrillic_lje);
    add(XK_Cyrillic_nje,                KeySym_Cyrillic_nje);
    add(XK_Serbian_tshe,                KeySym_Serbian_tshe);
    add(XK_Macedonia_kje,               KeySym_Macedonia_kje);
    add(XK_Ukrainian_ghe_with_upturn,   KeySym_Ukrainian_ghe_with_upturn);
    add(XK_Byelorussian_shortu,         KeySym_Byelorussian_shortu);
    add(XK_Cyrillic_dzhe,               KeySym_Cyrillic_dzhe);
    add(XK_numerosign,                  KeySym_numerosign);
    add(XK_Serbian_DJE,                 KeySym_Serbian_DJE);
    add(XK_Macedonia_GJE,               KeySym_Macedonia_GJE);
    add(XK_Cyrillic_IO,                 KeySym_Cyrillic_IO);
    add(XK_Ukrainian_IE,                KeySym_Ukrainian_IE);
    add(XK_Macedonia_DSE,               KeySym_Macedonia_DSE);
    add(XK_Ukrainian_I,                 KeySym_Ukrainian_I);
    add(XK_Ukrainian_YI,                KeySym_Ukrainian_YI);
    add(XK_Cyrillic_JE,                 KeySym_Cyrillic_JE);
    add(XK_Cyrillic_LJE,                KeySym_Cyrillic_LJE);
    add(XK_Cyrillic_NJE,                KeySym_Cyrillic_NJE);
    add(XK_Serbian_TSHE,                KeySym_Serbian_TSHE);
    add(XK_Macedonia_KJE,               KeySym_Macedonia_KJE);
    add(XK_Ukrainian_GHE_WITH_UPTURN,   KeySym_Ukrainian_GHE_WITH_UPTURN);
    add(XK_Byelorussian_SHORTU,         KeySym_Byelorussian_SHORTU);
    add(XK_Cyrillic_DZHE,               KeySym_Cyrillic_DZHE);
    add(XK_Cyrillic_yu,                 KeySym_Cyrillic_yu);
    add(XK_Cyrillic_a,                  KeySym_Cyrillic_a);
    add(XK_Cyrillic_be,                 KeySym_Cyrillic_be);
    add(XK_Cyrillic_tse,                KeySym_Cyrillic_tse);
    add(XK_Cyrillic_de,                 KeySym_Cyrillic_de);
    add(XK_Cyrillic_ie,                 KeySym_Cyrillic_ie);
    add(XK_Cyrillic_ef,                 KeySym_Cyrillic_ef);
    add(XK_Cyrillic_ghe,                KeySym_Cyrillic_ghe);
    add(XK_Cyrillic_ha,                 KeySym_Cyrillic_ha);
    add(XK_Cyrillic_i,                  KeySym_Cyrillic_i);
    add(XK_Cyrillic_shorti,             KeySym_Cyrillic_shorti);
    add(XK_Cyrillic_ka,                 KeySym_Cyrillic_ka);
    add(XK_Cyrillic_el,                 KeySym_Cyrillic_el);
    add(XK_Cyrillic_em,                 KeySym_Cyrillic_em);
    add(XK_Cyrillic_en,                 KeySym_Cyrillic_en);
    add(XK_Cyrillic_o,                  KeySym_Cyrillic_o);
    add(XK_Cyrillic_pe,                 KeySym_Cyrillic_pe);
    add(XK_Cyrillic_ya,                 KeySym_Cyrillic_ya);
    add(XK_Cyrillic_er,                 KeySym_Cyrillic_er);
    add(XK_Cyrillic_es,                 KeySym_Cyrillic_es);
    add(XK_Cyrillic_te,                 KeySym_Cyrillic_te);
    add(XK_Cyrillic_u,                  KeySym_Cyrillic_u);
    add(XK_Cyrillic_zhe,                KeySym_Cyrillic_zhe);
    add(XK_Cyrillic_ve,                 KeySym_Cyrillic_ve);
    add(XK_Cyrillic_softsign,           KeySym_Cyrillic_softsign);
    add(XK_Cyrillic_yeru,               KeySym_Cyrillic_yeru);
    add(XK_Cyrillic_ze,                 KeySym_Cyrillic_ze);
    add(XK_Cyrillic_sha,                KeySym_Cyrillic_sha);
    add(XK_Cyrillic_e,                  KeySym_Cyrillic_e);
    add(XK_Cyrillic_shcha,              KeySym_Cyrillic_shcha);
    add(XK_Cyrillic_che,                KeySym_Cyrillic_che);
    add(XK_Cyrillic_hardsign,           KeySym_Cyrillic_hardsign);
    add(XK_Cyrillic_YU,                 KeySym_Cyrillic_YU);
    add(XK_Cyrillic_A,                  KeySym_Cyrillic_A);
    add(XK_Cyrillic_BE,                 KeySym_Cyrillic_BE);
    add(XK_Cyrillic_TSE,                KeySym_Cyrillic_TSE);
    add(XK_Cyrillic_DE,                 KeySym_Cyrillic_DE);
    add(XK_Cyrillic_IE,                 KeySym_Cyrillic_IE);
    add(XK_Cyrillic_EF,                 KeySym_Cyrillic_EF);
    add(XK_Cyrillic_GHE,                KeySym_Cyrillic_GHE);
    add(XK_Cyrillic_HA,                 KeySym_Cyrillic_HA);
    add(XK_Cyrillic_I,                  KeySym_Cyrillic_I);
    add(XK_Cyrillic_SHORTI,             KeySym_Cyrillic_SHORTI);
    add(XK_Cyrillic_KA,                 KeySym_Cyrillic_KA);
    add(XK_Cyrillic_EL,                 KeySym_Cyrillic_EL);
    add(XK_Cyrillic_EM,                 KeySym_Cyrillic_EM);
    add(XK_Cyrillic_EN,                 KeySym_Cyrillic_EN);
    add(XK_Cyrillic_O,                  KeySym_Cyrillic_O);
    add(XK_Cyrillic_PE,                 KeySym_Cyrillic_PE);
    add(XK_Cyrillic_YA,                 KeySym_Cyrillic_YA);
    add(XK_Cyrillic_ER,                 KeySym_Cyrillic_ER);
    add(XK_Cyrillic_ES,                 KeySym_Cyrillic_ES);
    add(XK_Cyrillic_TE,                 KeySym_Cyrillic_TE);
    add(XK_Cyrillic_U,                  KeySym_Cyrillic_U);
    add(XK_Cyrillic_ZHE,                KeySym_Cyrillic_ZHE);
    add(XK_Cyrillic_VE,                 KeySym_Cyrillic_VE);
    add(XK_Cyrillic_SOFTSIGN,           KeySym_Cyrillic_SOFTSIGN);
    add(XK_Cyrillic_YERU,               KeySym_Cyrillic_YERU);
    add(XK_Cyrillic_ZE,                 KeySym_Cyrillic_ZE);
    add(XK_Cyrillic_SHA,                KeySym_Cyrillic_SHA);
    add(XK_Cyrillic_E,                  KeySym_Cyrillic_E);
    add(XK_Cyrillic_SHCHA,              KeySym_Cyrillic_SHCHA);
    add(XK_Cyrillic_CHE,                KeySym_Cyrillic_CHE);
    add(XK_Cyrillic_HARDSIGN,           KeySym_Cyrillic_HARDSIGN);

    // Greek
    add(XK_Greek_ALPHAaccent,           KeySym_Greek_ALPHAaccent);
    add(XK_Greek_EPSILONaccent,         KeySym_Greek_EPSILONaccent);
    add(XK_Greek_ETAaccent,             KeySym_Greek_ETAaccent);
    add(XK_Greek_IOTAaccent,            KeySym_Greek_IOTAaccent);
    add(XK_Greek_IOTAdieresis,          KeySym_Greek_IOTAdieresis);
    add(XK_Greek_OMICRONaccent,         KeySym_Greek_OMICRONaccent);
    add(XK_Greek_UPSILONaccent,         KeySym_Greek_UPSILONaccent);
    add(XK_Greek_UPSILONdieresis,       KeySym_Greek_UPSILONdieresis);
    add(XK_Greek_OMEGAaccent,           KeySym_Greek_OMEGAaccent);
    add(XK_Greek_accentdieresis,        KeySym_Greek_accentdieresis);
    add(XK_Greek_horizbar,              KeySym_Greek_horizbar);
    add(XK_Greek_alphaaccent,           KeySym_Greek_alphaaccent);
    add(XK_Greek_epsilonaccent,         KeySym_Greek_epsilonaccent);
    add(XK_Greek_etaaccent,             KeySym_Greek_etaaccent);
    add(XK_Greek_iotaaccent,            KeySym_Greek_iotaaccent);
    add(XK_Greek_iotadieresis,          KeySym_Greek_iotadieresis);
    add(XK_Greek_iotaaccentdieresis,    KeySym_Greek_iotaaccentdieresis);
    add(XK_Greek_omicronaccent,         KeySym_Greek_omicronaccent);
    add(XK_Greek_upsilonaccent,         KeySym_Greek_upsilonaccent);
    add(XK_Greek_upsilondieresis,       KeySym_Greek_upsilondieresis);
    add(XK_Greek_upsilonaccentdieresis, KeySym_Greek_upsilonaccentdieresis);
    add(XK_Greek_omegaaccent,           KeySym_Greek_omegaaccent);
    add(XK_Greek_ALPHA,                 KeySym_Greek_ALPHA);
    add(XK_Greek_BETA,                  KeySym_Greek_BETA);
    add(XK_Greek_GAMMA,                 KeySym_Greek_GAMMA);
    add(XK_Greek_DELTA,                 KeySym_Greek_DELTA);
    add(XK_Greek_EPSILON,               KeySym_Greek_EPSILON);
    add(XK_Greek_ZETA,                  KeySym_Greek_ZETA);
    add(XK_Greek_ETA,                   KeySym_Greek_ETA);
    add(XK_Greek_THETA,                 KeySym_Greek_THETA);
    add(XK_Greek_IOTA,                  KeySym_Greek_IOTA);
    add(XK_Greek_KAPPA,                 KeySym_Greek_KAPPA);
    add(XK_Greek_LAMBDA,                KeySym_Greek_LAMBDA);
    add(XK_Greek_MU,                    KeySym_Greek_MU);
    add(XK_Greek_NU,                    KeySym_Greek_NU);
    add(XK_Greek_XI,                    KeySym_Greek_XI);
    add(XK_Greek_OMICRON,               KeySym_Greek_OMICRON);
    add(XK_Greek_PI,                    KeySym_Greek_PI);
    add(XK_Greek_RHO,                   KeySym_Greek_RHO);
    add(XK_Greek_SIGMA,                 KeySym_Greek_SIGMA);
    add(XK_Greek_TAU,                   KeySym_Greek_TAU);
    add(XK_Greek_UPSILON,               KeySym_Greek_UPSILON);
    add(XK_Greek_PHI,                   KeySym_Greek_PHI);
    add(XK_Greek_CHI,                   KeySym_Greek_CHI);
    add(XK_Greek_PSI,                   KeySym_Greek_PSI);
    add(XK_Greek_OMEGA,                 KeySym_Greek_OMEGA);
    add(XK_Greek_alpha,                 KeySym_Greek_alpha);
    add(XK_Greek_beta,                  KeySym_Greek_beta);
    add(XK_Greek_gamma,                 KeySym_Greek_gamma);
    add(XK_Greek_delta,                 KeySym_Greek_delta);
    add(XK_Greek_epsilon,               KeySym_Greek_epsilon);
    add(XK_Greek_zeta,                  KeySym_Greek_zeta);
    add(XK_Greek_eta,                   KeySym_Greek_eta);
    add(XK_Greek_theta,                 KeySym_Greek_theta);
    add(XK_Greek_iota,                  KeySym_Greek_iota);
    add(XK_Greek_kappa,                 KeySym_Greek_kappa);
    add(XK_Greek_lambda,                KeySym_Greek_lambda);
    add(XK_Greek_mu,                    KeySym_Greek_mu);
    add(XK_Greek_nu,                    KeySym_Greek_nu);
    add(XK_Greek_xi,                    KeySym_Greek_xi);
    add(XK_Greek_omicron,               KeySym_Greek_omicron);
    add(XK_Greek_pi,                    KeySym_Greek_pi);
    add(XK_Greek_rho,                   KeySym_Greek_rho);
    add(XK_Greek_sigma,                 KeySym_Greek_sigma);
    add(XK_Greek_finalsmallsigma,       KeySym_Greek_finalsmallsigma);
    add(XK_Greek_tau,                   KeySym_Greek_tau);
    add(XK_Greek_upsilon,               KeySym_Greek_upsilon);
    add(XK_Greek_phi,                   KeySym_Greek_phi);
    add(XK_Greek_chi,                   KeySym_Greek_chi);
    add(XK_Greek_psi,                   KeySym_Greek_psi);
    add(XK_Greek_omega,                 KeySym_Greek_omega);

/*
    // Technical
    add(XK_leftradical,                 KeySym_leftradical);
    add(XK_topleftradical,              KeySym_topleftradical);
    add(XK_horizconnector,              KeySym_horizconnector);
    add(XK_topintegral,                 KeySym_topintegral);
    add(XK_botintegral,                 KeySym_botintegral);
    add(XK_vertconnector,               KeySym_vertconnector);
    add(XK_topleftsqbracket,            KeySym_topleftsqbracket);
    add(XK_botleftsqbracket,            KeySym_botleftsqbracket);
    add(XK_toprightsqbracket,           KeySym_toprightsqbracket);
    add(XK_botrightsqbracket,           KeySym_botrightsqbracket);
    add(XK_topleftparens,               KeySym_topleftparens);
    add(XK_botleftparens,               KeySym_botleftparens);
    add(XK_toprightparens,              KeySym_toprightparens);
    add(XK_botrightparens,              KeySym_botrightparens);
    add(XK_leftmiddlecurlybrace,        KeySym_leftmiddlecurlybrace);
    add(XK_rightmiddlecurlybrace,       KeySym_rightmiddlecurlybrace);
    add(XK_topleftsummation,            KeySym_topleftsummation);
    add(XK_botleftsummation,            KeySym_botleftsummation);
    add(XK_topvertsummationconnector,   KeySym_topvertsummationconnector);
    add(XK_botvertsummationconnector,   KeySym_botvertsummationconnector);
    add(XK_toprightsummation,           KeySym_toprightsummation);
    add(XK_botrightsummation,           KeySym_botrightsummation);
    add(XK_rightmiddlesummation,        KeySym_rightmiddlesummation);
    add(XK_lessthanequal,               KeySym_lessthanequal);
    add(XK_notequal,                    KeySym_notequal);
    add(XK_greaterthanequal,            KeySym_greaterthanequal);
    add(XK_integral,                    KeySym_integral);
    add(XK_therefore,                   KeySym_therefore);
    add(XK_variation,                   KeySym_variation);
    add(XK_infinity,                    KeySym_infinity);
    add(XK_nabla,                       KeySym_nabla);
    add(XK_approximate,                 KeySym_approximate);
    add(XK_similarequal,                KeySym_similarequal);
    add(XK_ifonlyif,                    KeySym_ifonlyif);
    add(XK_implies,                     KeySym_implies);
    add(XK_identical,                   KeySym_identical);
    add(XK_radical,                     KeySym_radical);
    add(XK_includedin,                  KeySym_includedin);
    add(XK_includes,                    KeySym_includes);
    add(XK_intersection,                KeySym_intersection);
    add(XK_union,                       KeySym_union);
    add(XK_logicaland,                  KeySym_logicaland);
    add(XK_logicalor,                   KeySym_logicalor);
    add(XK_partialderivative,           KeySym_partialderivative);
    add(XK_function,                    KeySym_function);
    add(XK_leftarrow,                   KeySym_leftarrow);
    add(XK_uparrow,                     KeySym_uparrow);
    add(XK_rightarrow,                  KeySym_rightarrow);
    add(XK_downarrow,                   KeySym_downarrow);

    // Special
    add(XK_blank,                       KeySym_blank);
    add(XK_soliddiamond,                KeySym_soliddiamond);
    add(XK_checkerboard,                KeySym_checkerboard);
    add(XK_ht,                          KeySym_ht);
    add(XK_ff,                          KeySym_ff);
    add(XK_cr,                          KeySym_cr);
    add(XK_lf,                          KeySym_lf);
    add(XK_nl,                          KeySym_nl);
    add(XK_vt,                          KeySym_vt);
    add(XK_lowrightcorner,              KeySym_lowrightcorner);
    add(XK_uprightcorner,               KeySym_uprightcorner);
    add(XK_upleftcorner,                KeySym_upleftcorner);
    add(XK_lowleftcorner,               KeySym_lowleftcorner);
    add(XK_crossinglines,               KeySym_crossinglines);
    add(XK_horizlinescan1,              KeySym_horizlinescan1);
    add(XK_horizlinescan3,              KeySym_horizlinescan3);
    add(XK_horizlinescan5,              KeySym_horizlinescan5);
    add(XK_horizlinescan7,              KeySym_horizlinescan7);
    add(XK_horizlinescan9,              KeySym_horizlinescan9);
    add(XK_leftt,                       KeySym_leftt);
    add(XK_rightt,                      KeySym_rightt);
    add(XK_bott,                        KeySym_bott);
    add(XK_topt,                        KeySym_topt);
    add(XK_vertbar,                     KeySym_vertbar);

    // Publishing
    add(XK_emspace,                     KeySym_emspace);
    add(XK_enspace,                     KeySym_enspace);
    add(XK_em3space,                    KeySym_em3space);
    add(XK_em4space,                    KeySym_em4space);
    add(XK_digitspace,                  KeySym_digitspace);
    add(XK_punctspace,                  KeySym_punctspace);
    add(XK_thinspace,                   KeySym_thinspace);
    add(XK_hairspace,                   KeySym_hairspace);
    add(XK_emdash,                      KeySym_emdash);
    add(XK_endash,                      KeySym_endash);
    add(XK_signifblank,                 KeySym_signifblank);
    add(XK_ellipsis,                    KeySym_ellipsis);
    add(XK_doubbaselinedot,             KeySym_doubbaselinedot);
    add(XK_onethird,                    KeySym_onethird);
    add(XK_twothirds,                   KeySym_twothirds);
    add(XK_onefifth,                    KeySym_onefifth);
    add(XK_twofifths,                   KeySym_twofifths);
    add(XK_threefifths,                 KeySym_threefifths);
    add(XK_fourfifths,                  KeySym_fourfifths);
    add(XK_onesixth,                    KeySym_onesixth);
    add(XK_fivesixths,                  KeySym_fivesixths);
    add(XK_careof,                      KeySym_careof);
    add(XK_figdash,                     KeySym_figdash);
    add(XK_leftanglebracket,            KeySym_leftanglebracket);
    add(XK_decimalpoint,                KeySym_decimalpoint);
    add(XK_rightanglebracket,           KeySym_rightanglebracket);
    add(XK_marker,                      KeySym_marker);
    add(XK_oneeighth,                   KeySym_oneeighth);
    add(XK_threeeighths,                KeySym_threeeighths);
    add(XK_fiveeighths,                 KeySym_fiveeighths);
    add(XK_seveneighths,                KeySym_seveneighths);
    add(XK_trademark,                   KeySym_trademark);
    add(XK_signaturemark,               KeySym_signaturemark);
    add(XK_trademarkincircle,           KeySym_trademarkincircle);
    add(XK_leftopentriangle,            KeySym_leftopentriangle);
    add(XK_rightopentriangle,           KeySym_rightopentriangle);
    add(XK_emopencircle,                KeySym_emopencircle);
    add(XK_emopenrectangle,             KeySym_emopenrectangle);
    add(XK_leftsinglequotemark,         KeySym_leftsinglequotemark);
    add(XK_rightsinglequotemark,        KeySym_rightsinglequotemark);
    add(XK_leftdoublequotemark,         KeySym_leftdoublequotemark);
    add(XK_rightdoublequotemark,        KeySym_rightdoublequotemark);
    add(XK_prescription,                KeySym_prescription);
    add(XK_minutes,                     KeySym_minutes);
    add(XK_seconds,                     KeySym_seconds);
    add(XK_latincross,                  KeySym_latincross);
    add(XK_hexagram,                    KeySym_hexagram);
    add(XK_filledrectbullet,            KeySym_filledrectbullet);
    add(XK_filledlefttribullet,         KeySym_filledlefttribullet);
    add(XK_filledrighttribullet,        KeySym_filledrighttribullet);
    add(XK_emfilledcircle,              KeySym_emfilledcircle);
    add(XK_emfilledrect,                KeySym_emfilledrect);
    add(XK_enopencircbullet,            KeySym_enopencircbullet);
    add(XK_enopensquarebullet,          KeySym_enopensquarebullet);
    add(XK_openrectbullet,              KeySym_openrectbullet);
    add(XK_opentribulletup,             KeySym_opentribulletup);
    add(XK_opentribulletdown,           KeySym_opentribulletdown);
    add(XK_openstar,                    KeySym_openstar);
    add(XK_enfilledcircbullet,          KeySym_enfilledcircbullet);
    add(XK_enfilledsqbullet,            KeySym_enfilledsqbullet);
    add(XK_filledtribulletup,           KeySym_filledtribulletup);
    add(XK_filledtribulletdown,         KeySym_filledtribulletdown);
    add(XK_leftpointer,                 KeySym_leftpointer);
    add(XK_rightpointer,                KeySym_rightpointer);
    add(XK_club,                        KeySym_club);
    add(XK_diamond,                     KeySym_diamond);
    add(XK_heart,                       KeySym_heart);
    add(XK_maltesecross,                KeySym_maltesecross);
    add(XK_dagger,                      KeySym_dagger);
    add(XK_doubledagger,                KeySym_doubledagger);
    add(XK_checkmark,                   KeySym_checkmark);
    add(XK_ballotcross,                 KeySym_ballotcross);
    add(XK_musicalsharp,                KeySym_musicalsharp);
    add(XK_musicalflat,                 KeySym_musicalflat);
    add(XK_malesymbol,                  KeySym_malesymbol);
    add(XK_femalesymbol,                KeySym_femalesymbol);
    add(XK_telephone,                   KeySym_telephone);
    add(XK_telephonerecorder,           KeySym_telephonerecorder);
    add(XK_phonographcopyright,         KeySym_phonographcopyright);
    add(XK_caret,                       KeySym_caret);
    add(XK_singlelowquotemark,          KeySym_singlelowquotemark);
    add(XK_doublelowquotemark,          KeySym_doublelowquotemark);
    add(XK_cursor,                      KeySym_cursor);

    // APL
    add(XK_leftcaret,                   KeySym_leftcaret);
    add(XK_rightcaret,                  KeySym_rightcaret);
    add(XK_downcaret,                   KeySym_downcaret);
    add(XK_upcaret,                     KeySym_upcaret);
    add(XK_overbar,                     KeySym_overbar);
    add(XK_downtack,                    KeySym_downtack);
    add(XK_upshoe,                      KeySym_upshoe);
    add(XK_downstile,                   KeySym_downstile);
    add(XK_underbar,                    KeySym_underbar);
    add(XK_jot,                         KeySym_jot);
    add(XK_quad,                        KeySym_quad);
    add(XK_uptack,                      KeySym_uptack);
    add(XK_circle,                      KeySym_circle);
    add(XK_upstile,                     KeySym_upstile);
    add(XK_downshoe,                    KeySym_downshoe);
    add(XK_rightshoe,                   KeySym_rightshoe);
    add(XK_leftshoe,                    KeySym_leftshoe);
    add(XK_lefttack,                    KeySym_lefttack);
    add(XK_righttack,                   KeySym_righttack);
*/

    // Hebrew
    add(XK_hebrew_doublelowline,        KeySym_hebrew_doublelowline);
    add(XK_hebrew_aleph,                KeySym_hebrew_aleph);
    add(XK_hebrew_bet,                  KeySym_hebrew_bet);
    add(XK_hebrew_gimel,                KeySym_hebrew_gimel);
    add(XK_hebrew_dalet,                KeySym_hebrew_dalet);
    add(XK_hebrew_he,                   KeySym_hebrew_he);
    add(XK_hebrew_waw,                  KeySym_hebrew_waw);
    add(XK_hebrew_zain,                 KeySym_hebrew_zain);
    add(XK_hebrew_chet,                 KeySym_hebrew_chet);
    add(XK_hebrew_tet,                  KeySym_hebrew_tet);
    add(XK_hebrew_yod,                  KeySym_hebrew_yod);
    add(XK_hebrew_finalkaph,            KeySym_hebrew_finalkaph);
    add(XK_hebrew_kaph,                 KeySym_hebrew_kaph);
    add(XK_hebrew_lamed,                KeySym_hebrew_lamed);
    add(XK_hebrew_finalmem,             KeySym_hebrew_finalmem);
    add(XK_hebrew_mem,                  KeySym_hebrew_mem);
    add(XK_hebrew_finalnun,             KeySym_hebrew_finalnun);
    add(XK_hebrew_nun,                  KeySym_hebrew_nun);
    add(XK_hebrew_samech,               KeySym_hebrew_samech);
    add(XK_hebrew_ayin,                 KeySym_hebrew_ayin);
    add(XK_hebrew_finalpe,              KeySym_hebrew_finalpe);
    add(XK_hebrew_pe,                   KeySym_hebrew_pe);
    add(XK_hebrew_finalzade,            KeySym_hebrew_finalzade);
    add(XK_hebrew_zade,                 KeySym_hebrew_zade);
    add(XK_hebrew_qoph,                 KeySym_hebrew_qoph);
    add(XK_hebrew_resh,                 KeySym_hebrew_resh);
    add(XK_hebrew_shin,                 KeySym_hebrew_shin);
    add(XK_hebrew_taw,                  KeySym_hebrew_taw);

    // Thai
    add(XK_Thai_kokai,                  KeySym_Thai_kokai);
    add(XK_Thai_khokhai,                KeySym_Thai_khokhai);
    add(XK_Thai_khokhuat,               KeySym_Thai_khokhuat);
    add(XK_Thai_khokhwai,               KeySym_Thai_khokhwai);
    add(XK_Thai_khokhon,                KeySym_Thai_khokhon);
    add(XK_Thai_khorakhang,             KeySym_Thai_khorakhang);
    add(XK_Thai_ngongu,                 KeySym_Thai_ngongu);
    add(XK_Thai_chochan,                KeySym_Thai_chochan);
    add(XK_Thai_choching,               KeySym_Thai_choching);
    add(XK_Thai_chochang,               KeySym_Thai_chochang);
    add(XK_Thai_soso,                   KeySym_Thai_soso);
    add(XK_Thai_chochoe,                KeySym_Thai_chochoe);
    add(XK_Thai_yoying,                 KeySym_Thai_yoying);
    add(XK_Thai_dochada,                KeySym_Thai_dochada);
    add(XK_Thai_topatak,                KeySym_Thai_topatak);
    add(XK_Thai_thothan,                KeySym_Thai_thothan);
    add(XK_Thai_thonangmontho,          KeySym_Thai_thonangmontho);
    add(XK_Thai_thophuthao,             KeySym_Thai_thophuthao);
    add(XK_Thai_nonen,                  KeySym_Thai_nonen);
    add(XK_Thai_dodek,                  KeySym_Thai_dodek);
    add(XK_Thai_totao,                  KeySym_Thai_totao);
    add(XK_Thai_thothung,               KeySym_Thai_thothung);
    add(XK_Thai_thothahan,              KeySym_Thai_thothahan);
    add(XK_Thai_thothong,               KeySym_Thai_thothong);
    add(XK_Thai_nonu,                   KeySym_Thai_nonu);
    add(XK_Thai_bobaimai,               KeySym_Thai_bobaimai);
    add(XK_Thai_popla,                  KeySym_Thai_popla);
    add(XK_Thai_phophung,               KeySym_Thai_phophung);
    add(XK_Thai_fofa,                   KeySym_Thai_fofa);
    add(XK_Thai_phophan,                KeySym_Thai_phophan);
    add(XK_Thai_fofan,                  KeySym_Thai_fofan);
    add(XK_Thai_phosamphao,             KeySym_Thai_phosamphao);
    add(XK_Thai_moma,                   KeySym_Thai_moma);
    add(XK_Thai_yoyak,                  KeySym_Thai_yoyak);
    add(XK_Thai_rorua,                  KeySym_Thai_rorua);
    add(XK_Thai_ru,                     KeySym_Thai_ru);
    add(XK_Thai_loling,                 KeySym_Thai_loling);
    add(XK_Thai_lu,                     KeySym_Thai_lu);
    add(XK_Thai_wowaen,                 KeySym_Thai_wowaen);
    add(XK_Thai_sosala,                 KeySym_Thai_sosala);
    add(XK_Thai_sorusi,                 KeySym_Thai_sorusi);
    add(XK_Thai_sosua,                  KeySym_Thai_sosua);
    add(XK_Thai_hohip,                  KeySym_Thai_hohip);
    add(XK_Thai_lochula,                KeySym_Thai_lochula);
    add(XK_Thai_oang,                   KeySym_Thai_oang);
    add(XK_Thai_honokhuk,               KeySym_Thai_honokhuk);
    add(XK_Thai_paiyannoi,              KeySym_Thai_paiyannoi);
    add(XK_Thai_saraa,                  KeySym_Thai_saraa);
    add(XK_Thai_maihanakat,             KeySym_Thai_maihanakat);
    add(XK_Thai_saraaa,                 KeySym_Thai_saraaa);
    add(XK_Thai_saraam,                 KeySym_Thai_saraam);
    add(XK_Thai_sarai,                  KeySym_Thai_sarai);
    add(XK_Thai_saraii,                 KeySym_Thai_saraii);
    add(XK_Thai_saraue,                 KeySym_Thai_saraue);
    add(XK_Thai_sarauee,                KeySym_Thai_sarauee);
    add(XK_Thai_sarau,                  KeySym_Thai_sarau);
    add(XK_Thai_sarauu,                 KeySym_Thai_sarauu);
    add(XK_Thai_phinthu,                KeySym_Thai_phinthu);
    add(XK_Thai_maihanakat_maitho,      KeySym_Thai_maihanakat_maitho);
    add(XK_Thai_baht,                   KeySym_Thai_baht);
    add(XK_Thai_sarae,                  KeySym_Thai_sarae);
    add(XK_Thai_saraae,                 KeySym_Thai_saraae);
    add(XK_Thai_sarao,                  KeySym_Thai_sarao);
    add(XK_Thai_saraaimaimuan,          KeySym_Thai_saraaimaimuan);
    add(XK_Thai_saraaimaimalai,         KeySym_Thai_saraaimaimalai);
    add(XK_Thai_lakkhangyao,            KeySym_Thai_lakkhangyao);
    add(XK_Thai_maiyamok,               KeySym_Thai_maiyamok);
    add(XK_Thai_maitaikhu,              KeySym_Thai_maitaikhu);
    add(XK_Thai_maiek,                  KeySym_Thai_maiek);
    add(XK_Thai_maitho,                 KeySym_Thai_maitho);
    add(XK_Thai_maitri,                 KeySym_Thai_maitri);
    add(XK_Thai_maichattawa,            KeySym_Thai_maichattawa);
    add(XK_Thai_thanthakhat,            KeySym_Thai_thanthakhat);
    add(XK_Thai_nikhahit,               KeySym_Thai_nikhahit);
    add(XK_Thai_leksun,                 KeySym_Thai_leksun);
    add(XK_Thai_leknung,                KeySym_Thai_leknung);
    add(XK_Thai_leksong,                KeySym_Thai_leksong);
    add(XK_Thai_leksam,                 KeySym_Thai_leksam);
    add(XK_Thai_leksi,                  KeySym_Thai_leksi);
    add(XK_Thai_lekha,                  KeySym_Thai_lekha);
    add(XK_Thai_lekhok,                 KeySym_Thai_lekhok);
    add(XK_Thai_lekchet,                KeySym_Thai_lekchet);
    add(XK_Thai_lekpaet,                KeySym_Thai_lekpaet);
    add(XK_Thai_lekkao,                 KeySym_Thai_lekkao);

    // Korean
    add(XK_Hangul,                      KeySym_Hangul);
    add(XK_Hangul_Start,                KeySym_Hangul_Start);
    add(XK_Hangul_End,                  KeySym_Hangul_End);
    add(XK_Hangul_Hanja,                KeySym_Hangul_Hanja);
    add(XK_Hangul_Jamo,                 KeySym_Hangul_Jamo);
    add(XK_Hangul_Romaja,               KeySym_Hangul_Romaja);
    add(XK_Hangul_Jeonja,               KeySym_Hangul_Jeonja);
    add(XK_Hangul_Banja,                KeySym_Hangul_Banja);
    add(XK_Hangul_PreHanja,             KeySym_Hangul_PreHanja);
    add(XK_Hangul_PostHanja,            KeySym_Hangul_PostHanja);
    add(XK_Hangul_Special,              KeySym_Hangul_Special);
    // Hangul Consonant Characters
    add(XK_Hangul_Kiyeog,               KeySym_Hangul_Kiyeog);
    add(XK_Hangul_SsangKiyeog,          KeySym_Hangul_SsangKiyeog);
    add(XK_Hangul_KiyeogSios,           KeySym_Hangul_KiyeogSios);
    add(XK_Hangul_Nieun,                KeySym_Hangul_Nieun);
    add(XK_Hangul_NieunJieuj,           KeySym_Hangul_NieunJieuj);
    add(XK_Hangul_NieunHieuh,           KeySym_Hangul_NieunHieuh);
    add(XK_Hangul_Dikeud,               KeySym_Hangul_Dikeud);
    add(XK_Hangul_SsangDikeud,          KeySym_Hangul_SsangDikeud);
    add(XK_Hangul_Rieul,                KeySym_Hangul_Rieul);
    add(XK_Hangul_RieulKiyeog,          KeySym_Hangul_RieulKiyeog);
    add(XK_Hangul_RieulMieum,           KeySym_Hangul_RieulMieum);
    add(XK_Hangul_RieulPieub,           KeySym_Hangul_RieulPieub);
    add(XK_Hangul_RieulSios,            KeySym_Hangul_RieulSios);
    add(XK_Hangul_RieulTieut,           KeySym_Hangul_RieulTieut);
    add(XK_Hangul_RieulPhieuf,          KeySym_Hangul_RieulPhieuf);
    add(XK_Hangul_RieulHieuh,           KeySym_Hangul_RieulHieuh);
    add(XK_Hangul_Mieum,                KeySym_Hangul_Mieum);
    add(XK_Hangul_Pieub,                KeySym_Hangul_Pieub);
    add(XK_Hangul_SsangPieub,           KeySym_Hangul_SsangPieub);
    add(XK_Hangul_PieubSios,            KeySym_Hangul_PieubSios);
    add(XK_Hangul_Sios,                 KeySym_Hangul_Sios);
    add(XK_Hangul_SsangSios,            KeySym_Hangul_SsangSios);
    add(XK_Hangul_Ieung,                KeySym_Hangul_Ieung);
    add(XK_Hangul_Jieuj,                KeySym_Hangul_Jieuj);
    add(XK_Hangul_SsangJieuj,           KeySym_Hangul_SsangJieuj);
    add(XK_Hangul_Cieuc,                KeySym_Hangul_Cieuc);
    add(XK_Hangul_Khieuq,               KeySym_Hangul_Khieuq);
    add(XK_Hangul_Tieut,                KeySym_Hangul_Tieut);
    add(XK_Hangul_Phieuf,               KeySym_Hangul_Phieuf);
    add(XK_Hangul_Hieuh,                KeySym_Hangul_Hieuh);
    // Hangul Vowel Characters
    add(XK_Hangul_A,                    KeySym_Hangul_A);
    add(XK_Hangul_AE,                   KeySym_Hangul_AE);
    add(XK_Hangul_YA,                   KeySym_Hangul_YA);
    add(XK_Hangul_YAE,                  KeySym_Hangul_YAE);
    add(XK_Hangul_EO,                   KeySym_Hangul_EO);
    add(XK_Hangul_E,                    KeySym_Hangul_E);
    add(XK_Hangul_YEO,                  KeySym_Hangul_YEO);
    add(XK_Hangul_YE,                   KeySym_Hangul_YE);
    add(XK_Hangul_O,                    KeySym_Hangul_O);
    add(XK_Hangul_WA,                   KeySym_Hangul_WA);
    add(XK_Hangul_WAE,                  KeySym_Hangul_WAE);
    add(XK_Hangul_OE,                   KeySym_Hangul_OE);
    add(XK_Hangul_YO,                   KeySym_Hangul_YO);
    add(XK_Hangul_U,                    KeySym_Hangul_U);
    add(XK_Hangul_WEO,                  KeySym_Hangul_WEO);
    add(XK_Hangul_WE,                   KeySym_Hangul_WE);
    add(XK_Hangul_WI,                   KeySym_Hangul_WI);
    add(XK_Hangul_YU,                   KeySym_Hangul_YU);
    add(XK_Hangul_EU,                   KeySym_Hangul_EU);
    add(XK_Hangul_YI,                   KeySym_Hangul_YI);
    add(XK_Hangul_I,                    KeySym_Hangul_I);
    // Hangul syllable-final (JongSeong) Characters
    add(XK_Hangul_J_Kiyeog,             KeySym_Hangul_J_Kiyeog);
    add(XK_Hangul_J_SsangKiyeog,        KeySym_Hangul_J_SsangKiyeog);
    add(XK_Hangul_J_KiyeogSios,         KeySym_Hangul_J_KiyeogSios);
    add(XK_Hangul_J_Nieun,              KeySym_Hangul_J_Nieun);
    add(XK_Hangul_J_NieunJieuj,         KeySym_Hangul_J_NieunJieuj);
    add(XK_Hangul_J_NieunHieuh,         KeySym_Hangul_J_NieunHieuh);
    add(XK_Hangul_J_Dikeud,             KeySym_Hangul_J_Dikeud);
    add(XK_Hangul_J_Rieul,              KeySym_Hangul_J_Rieul);
    add(XK_Hangul_J_RieulKiyeog,        KeySym_Hangul_J_RieulKiyeog);
    add(XK_Hangul_J_RieulMieum,         KeySym_Hangul_J_RieulMieum);
    add(XK_Hangul_J_RieulPieub,         KeySym_Hangul_J_RieulPieub);
    add(XK_Hangul_J_RieulSios,          KeySym_Hangul_J_RieulSios);
    add(XK_Hangul_J_RieulTieut,         KeySym_Hangul_J_RieulTieut);
    add(XK_Hangul_J_RieulPhieuf,        KeySym_Hangul_J_RieulPhieuf);
    add(XK_Hangul_J_RieulHieuh,         KeySym_Hangul_J_RieulHieuh);
    add(XK_Hangul_J_Mieum,              KeySym_Hangul_J_Mieum);
    add(XK_Hangul_J_Pieub,              KeySym_Hangul_J_Pieub);
    add(XK_Hangul_J_PieubSios,          KeySym_Hangul_J_PieubSios);
    add(XK_Hangul_J_Sios,               KeySym_Hangul_J_Sios);
    add(XK_Hangul_J_SsangSios,          KeySym_Hangul_J_SsangSios);
    add(XK_Hangul_J_Ieung,              KeySym_Hangul_J_Ieung);
    add(XK_Hangul_J_Jieuj,              KeySym_Hangul_J_Jieuj);
    add(XK_Hangul_J_Cieuc,              KeySym_Hangul_J_Cieuc);
    add(XK_Hangul_J_Khieuq,             KeySym_Hangul_J_Khieuq);
    add(XK_Hangul_J_Tieut,              KeySym_Hangul_J_Tieut);
    add(XK_Hangul_J_Phieuf,             KeySym_Hangul_J_Phieuf);
    add(XK_Hangul_J_Hieuh,              KeySym_Hangul_J_Hieuh);
    // Ancient Hangul Consonant Characters
    add(XK_Hangul_RieulYeorinHieuh,     KeySym_Hangul_RieulYeorinHieuh);
    add(XK_Hangul_SunkyeongeumMieum,    KeySym_Hangul_SunkyeongeumMieum);
    add(XK_Hangul_SunkyeongeumPieub,    KeySym_Hangul_SunkyeongeumPieub);
    add(XK_Hangul_PanSios,              KeySym_Hangul_PanSios);
    add(XK_Hangul_KkogjiDalrinIeung,    KeySym_Hangul_KkogjiDalrinIeung);
    add(XK_Hangul_SunkyeongeumPhieuf,   KeySym_Hangul_SunkyeongeumPhieuf);
    add(XK_Hangul_YeorinHieuh,          KeySym_Hangul_YeorinHieuh);
    // Ancient Hangul Vowel Characters
    add(XK_Hangul_AraeA,                KeySym_Hangul_AraeA);
    add(XK_Hangul_AraeAE,               KeySym_Hangul_AraeAE);
    // Ancient Hangul syllable-final (JongSeong) Characters
    add(XK_Hangul_J_PanSios,            KeySym_Hangul_J_PanSios);
    add(XK_Hangul_J_KkogjiDalrinIeung,  KeySym_Hangul_J_KkogjiDalrinIeung);
    add(XK_Hangul_J_YeorinHieuh,        KeySym_Hangul_J_YeorinHieuh);
    // Korean currency symbol
    add(XK_Korean_Won,                  KeySym_Korean_Won);

/*
    // Armenian
    add(XK_Armenian_eternity,           KeySym_Armenian_eternity);
    add(XK_Armenian_ligature_ew,        KeySym_Armenian_ligature_ew);
    add(XK_Armenian_full_stop,          KeySym_Armenian_full_stop);
    add(XK_Armenian_parenright,         KeySym_Armenian_parenright);
    add(XK_Armenian_parenleft,          KeySym_Armenian_parenleft);
    add(XK_Armenian_guillemotright,     KeySym_Armenian_guillemotright);
    add(XK_Armenian_guillemotleft,      KeySym_Armenian_guillemotleft);
    add(XK_Armenian_em_dash,            KeySym_Armenian_em_dash);
    add(XK_Armenian_dot,                KeySym_Armenian_dot);
    add(XK_Armenian_separation_mark,    KeySym_Armenian_separation_mark);
    add(XK_Armenian_comma,              KeySym_Armenian_comma);
    add(XK_Armenian_en_dash,            KeySym_Armenian_en_dash);
    add(XK_Armenian_hyphen,             KeySym_Armenian_hyphen);
    add(XK_Armenian_ellipsis,           KeySym_Armenian_ellipsis);
    add(XK_Armenian_exclam,             KeySym_Armenian_exclam);
    add(XK_Armenian_accent,             KeySym_Armenian_accent);
    add(XK_Armenian_question,           KeySym_Armenian_question);
    add(XK_Armenian_AYB,                KeySym_Armenian_AYB);
    add(XK_Armenian_ayb,                KeySym_Armenian_ayb);
    add(XK_Armenian_BEN,                KeySym_Armenian_BEN);
    add(XK_Armenian_ben,                KeySym_Armenian_ben);
    add(XK_Armenian_GIM,                KeySym_Armenian_GIM);
    add(XK_Armenian_gim,                KeySym_Armenian_gim);
    add(XK_Armenian_DA,                 KeySym_Armenian_DA);
    add(XK_Armenian_da,                 KeySym_Armenian_da);
    add(XK_Armenian_YECH,               KeySym_Armenian_YECH);
    add(XK_Armenian_yech,               KeySym_Armenian_yech);
    add(XK_Armenian_ZA,                 KeySym_Armenian_ZA);
    add(XK_Armenian_za,                 KeySym_Armenian_za);
    add(XK_Armenian_E,                  KeySym_Armenian_E);
    add(XK_Armenian_e,                  KeySym_Armenian_e);
    add(XK_Armenian_AT,                 KeySym_Armenian_AT);
    add(XK_Armenian_at,                 KeySym_Armenian_at);
    add(XK_Armenian_TO,                 KeySym_Armenian_TO);
    add(XK_Armenian_to,                 KeySym_Armenian_to);
    add(XK_Armenian_ZHE,                KeySym_Armenian_ZHE);
    add(XK_Armenian_zhe,                KeySym_Armenian_zhe);
    add(XK_Armenian_INI,                KeySym_Armenian_INI);
    add(XK_Armenian_ini,                KeySym_Armenian_ini);
    add(XK_Armenian_LYUN,               KeySym_Armenian_LYUN);
    add(XK_Armenian_lyun,               KeySym_Armenian_lyun);
    add(XK_Armenian_KHE,                KeySym_Armenian_KHE);
    add(XK_Armenian_khe,                KeySym_Armenian_khe);
    add(XK_Armenian_TSA,                KeySym_Armenian_TSA);
    add(XK_Armenian_tsa,                KeySym_Armenian_tsa);
    add(XK_Armenian_KEN,                KeySym_Armenian_KEN);
    add(XK_Armenian_ken,                KeySym_Armenian_ken);
    add(XK_Armenian_HO,                 KeySym_Armenian_HO);
    add(XK_Armenian_ho,                 KeySym_Armenian_ho);
    add(XK_Armenian_DZA,                KeySym_Armenian_DZA);
    add(XK_Armenian_dza,                KeySym_Armenian_dza);
    add(XK_Armenian_GHAT,               KeySym_Armenian_GHAT);
    add(XK_Armenian_ghat,               KeySym_Armenian_ghat);
    add(XK_Armenian_TCHE,               KeySym_Armenian_TCHE);
    add(XK_Armenian_tche,               KeySym_Armenian_tche);
    add(XK_Armenian_MEN,                KeySym_Armenian_MEN);
    add(XK_Armenian_men,                KeySym_Armenian_men);
    add(XK_Armenian_HI,                 KeySym_Armenian_HI);
    add(XK_Armenian_hi,                 KeySym_Armenian_hi);
    add(XK_Armenian_NU,                 KeySym_Armenian_NU);
    add(XK_Armenian_nu,                 KeySym_Armenian_nu);
    add(XK_Armenian_SHA,                KeySym_Armenian_SHA);
    add(XK_Armenian_sha,                KeySym_Armenian_sha);
    add(XK_Armenian_VO,                 KeySym_Armenian_VO);
    add(XK_Armenian_vo,                 KeySym_Armenian_vo);
    add(XK_Armenian_CHA,                KeySym_Armenian_CHA);
    add(XK_Armenian_cha,                KeySym_Armenian_cha);
    add(XK_Armenian_PE,                 KeySym_Armenian_PE);
    add(XK_Armenian_pe,                 KeySym_Armenian_pe);
    add(XK_Armenian_JE,                 KeySym_Armenian_JE);
    add(XK_Armenian_je,                 KeySym_Armenian_je);
    add(XK_Armenian_RA,                 KeySym_Armenian_RA);
    add(XK_Armenian_ra,                 KeySym_Armenian_ra);
    add(XK_Armenian_SE,                 KeySym_Armenian_SE);
    add(XK_Armenian_se,                 KeySym_Armenian_se);
    add(XK_Armenian_VEV,                KeySym_Armenian_VEV);
    add(XK_Armenian_vev,                KeySym_Armenian_vev);
    add(XK_Armenian_TYUN,               KeySym_Armenian_TYUN);
    add(XK_Armenian_tyun,               KeySym_Armenian_tyun);
    add(XK_Armenian_RE,                 KeySym_Armenian_RE);
    add(XK_Armenian_re,                 KeySym_Armenian_re);
    add(XK_Armenian_TSO,                KeySym_Armenian_TSO);
    add(XK_Armenian_tso,                KeySym_Armenian_tso);
    add(XK_Armenian_VYUN,               KeySym_Armenian_VYUN);
    add(XK_Armenian_vyun,               KeySym_Armenian_vyun);
    add(XK_Armenian_PYUR,               KeySym_Armenian_PYUR);
    add(XK_Armenian_pyur,               KeySym_Armenian_pyur);
    add(XK_Armenian_KE,                 KeySym_Armenian_KE);
    add(XK_Armenian_ke,                 KeySym_Armenian_ke);
    add(XK_Armenian_O,                  KeySym_Armenian_O);
    add(XK_Armenian_o,                  KeySym_Armenian_o);
    add(XK_Armenian_FE,                 KeySym_Armenian_FE);
    add(XK_Armenian_fe,                 KeySym_Armenian_fe);
    add(XK_Armenian_apostrophe,         KeySym_Armenian_apostrophe);
    add(XK_Armenian_section_sign,       KeySym_Armenian_section_sign);
*/

    // Georgian
    add(XK_Georgian_an,                 KeySym_Georgian_an);
    add(XK_Georgian_ban,                KeySym_Georgian_ban);
    add(XK_Georgian_gan,                KeySym_Georgian_gan);
    add(XK_Georgian_don,                KeySym_Georgian_don);
    add(XK_Georgian_en,                 KeySym_Georgian_en);
    add(XK_Georgian_vin,                KeySym_Georgian_vin);
    add(XK_Georgian_zen,                KeySym_Georgian_zen);
    add(XK_Georgian_tan,                KeySym_Georgian_tan);
    add(XK_Georgian_in,                 KeySym_Georgian_in);
    add(XK_Georgian_kan,                KeySym_Georgian_kan);
    add(XK_Georgian_las,                KeySym_Georgian_las);
    add(XK_Georgian_man,                KeySym_Georgian_man);
    add(XK_Georgian_nar,                KeySym_Georgian_nar);
    add(XK_Georgian_on,                 KeySym_Georgian_on);
    add(XK_Georgian_par,                KeySym_Georgian_par);
    add(XK_Georgian_zhar,               KeySym_Georgian_zhar);
    add(XK_Georgian_rae,                KeySym_Georgian_rae);
    add(XK_Georgian_san,                KeySym_Georgian_san);
    add(XK_Georgian_tar,                KeySym_Georgian_tar);
    add(XK_Georgian_un,                 KeySym_Georgian_un);
    add(XK_Georgian_phar,               KeySym_Georgian_phar);
    add(XK_Georgian_khar,               KeySym_Georgian_khar);
    add(XK_Georgian_ghan,               KeySym_Georgian_ghan);
    add(XK_Georgian_qar,                KeySym_Georgian_qar);
    add(XK_Georgian_shin,               KeySym_Georgian_shin);
    add(XK_Georgian_chin,               KeySym_Georgian_chin);
    add(XK_Georgian_can,                KeySym_Georgian_can);
    add(XK_Georgian_jil,                KeySym_Georgian_jil);
    add(XK_Georgian_cil,                KeySym_Georgian_cil);
    add(XK_Georgian_char,               KeySym_Georgian_char);
    add(XK_Georgian_xan,                KeySym_Georgian_xan);
    add(XK_Georgian_jhan,               KeySym_Georgian_jhan);
    add(XK_Georgian_hae,                KeySym_Georgian_hae);
    add(XK_Georgian_he,                 KeySym_Georgian_he);
    add(XK_Georgian_hie,                KeySym_Georgian_hie);
    add(XK_Georgian_we,                 KeySym_Georgian_we);
    add(XK_Georgian_har,                KeySym_Georgian_har);
    add(XK_Georgian_hoe,                KeySym_Georgian_hoe);
    add(XK_Georgian_fi,                 KeySym_Georgian_fi);

/*
    // Azeri (and other Turkic or Caucasian languages of ex-USSR)
    //
    // latin
    add(XK_Ccedillaabovedot,            KeySym_Ccedillaabovedot);
    add(XK_Xabovedot,                   KeySym_Xabovedot);
    add(XK_Qabovedot,                   KeySym_Qabovedot);
    add(XK_Ibreve,                      KeySym_Ibreve);
    add(XK_IE,                          KeySym_IE);
    add(XK_UO,                          KeySym_UO);
    add(XK_Zstroke,                     KeySym_Zstroke);
    add(XK_Gcaron,                      KeySym_Gcaron);
    add(XK_Obarred,                     KeySym_Obarred);
    add(XK_ccedillaabovedot,            KeySym_ccedillaabovedot);
    add(XK_xabovedot,                   KeySym_xabovedot);
    add(XK_Ocaron,                      KeySym_Ocaron);
    add(XK_qabovedot,                   KeySym_qabovedot);
    add(XK_ibreve,                      KeySym_ibreve);
    add(XK_ie,                          KeySym_ie);
    add(XK_uo,                          KeySym_uo);
    add(XK_zstroke,                     KeySym_zstroke);
    add(XK_gcaron,                      KeySym_gcaron);
    add(XK_ocaron,                      KeySym_ocaron);
    add(XK_obarred,                     KeySym_obarred);
    add(XK_SCHWA,                       KeySym_SCHWA);
    add(XK_schwa,                       KeySym_schwa);
    // For Inupiak
    add(XK_Lbelowdot,                   KeySym_Lbelowdot);
    add(XK_Lstrokebelowdot,             KeySym_Lstrokebelowdot);
    add(XK_lbelowdot,                   KeySym_lbelowdot);
    add(XK_lstrokebelowdot,             KeySym_lstrokebelowdot);
    // For Guarani
    add(XK_Gtilde,                      KeySym_Gtilde);
    add(XK_gtilde,                      KeySym_gtilde);
*/

    // Vietnamese
    add(XK_Abelowdot,                   KeySym_Abelowdot);
    add(XK_abelowdot,                   KeySym_abelowdot);
    add(XK_Ahook,                       KeySym_Ahook);
    add(XK_ahook,                       KeySym_ahook);
    add(XK_Acircumflexacute,            KeySym_Acircumflexacute);
    add(XK_acircumflexacute,            KeySym_acircumflexacute);
    add(XK_Acircumflexgrave,            KeySym_Acircumflexgrave);
    add(XK_acircumflexgrave,            KeySym_acircumflexgrave);
    add(XK_Acircumflexhook,             KeySym_Acircumflexhook);
    add(XK_acircumflexhook,             KeySym_acircumflexhook);
    add(XK_Acircumflextilde,            KeySym_Acircumflextilde);
    add(XK_acircumflextilde,            KeySym_acircumflextilde);
    add(XK_Acircumflexbelowdot,         KeySym_Acircumflexbelowdot);
    add(XK_acircumflexbelowdot,         KeySym_acircumflexbelowdot);
    add(XK_Abreveacute,                 KeySym_Abreveacute);
    add(XK_abreveacute,                 KeySym_abreveacute);
    add(XK_Abrevegrave,                 KeySym_Abrevegrave);
    add(XK_abrevegrave,                 KeySym_abrevegrave);
    add(XK_Abrevehook,                  KeySym_Abrevehook);
    add(XK_abrevehook,                  KeySym_abrevehook);
    add(XK_Abrevetilde,                 KeySym_Abrevetilde);
    add(XK_abrevetilde,                 KeySym_abrevetilde);
    add(XK_Abrevebelowdot,              KeySym_Abrevebelowdot);
    add(XK_abrevebelowdot,              KeySym_abrevebelowdot);
    add(XK_Ebelowdot,                   KeySym_Ebelowdot);
    add(XK_ebelowdot,                   KeySym_ebelowdot);
    add(XK_Ehook,                       KeySym_Ehook);
    add(XK_ehook,                       KeySym_ehook);
    add(XK_Etilde,                      KeySym_Etilde);
    add(XK_etilde,                      KeySym_etilde);
    add(XK_Ecircumflexacute,            KeySym_Ecircumflexacute);
    add(XK_ecircumflexacute,            KeySym_ecircumflexacute);
    add(XK_Ecircumflexgrave,            KeySym_Ecircumflexgrave);
    add(XK_ecircumflexgrave,            KeySym_ecircumflexgrave);
    add(XK_Ecircumflexhook,             KeySym_Ecircumflexhook);
    add(XK_ecircumflexhook,             KeySym_ecircumflexhook);
    add(XK_Ecircumflextilde,            KeySym_Ecircumflextilde);
    add(XK_ecircumflextilde,            KeySym_ecircumflextilde);
    add(XK_Ecircumflexbelowdot,         KeySym_Ecircumflexbelowdot);
    add(XK_ecircumflexbelowdot,         KeySym_ecircumflexbelowdot);
    add(XK_Ihook,                       KeySym_Ihook);
    add(XK_ihook,                       KeySym_ihook);
    add(XK_Ibelowdot,                   KeySym_Ibelowdot);
    add(XK_ibelowdot,                   KeySym_ibelowdot);
    add(XK_Obelowdot,                   KeySym_Obelowdot);
    add(XK_obelowdot,                   KeySym_obelowdot);
    add(XK_Ohook,                       KeySym_Ohook);
    add(XK_ohook,                       KeySym_ohook);
    add(XK_Ocircumflexacute,            KeySym_Ocircumflexacute);
    add(XK_ocircumflexacute,            KeySym_ocircumflexacute);
    add(XK_Ocircumflexgrave,            KeySym_Ocircumflexgrave);
    add(XK_ocircumflexgrave,            KeySym_ocircumflexgrave);
    add(XK_Ocircumflexhook,             KeySym_Ocircumflexhook);
    add(XK_ocircumflexhook,             KeySym_ocircumflexhook);
    add(XK_Ocircumflextilde,            KeySym_Ocircumflextilde);
    add(XK_ocircumflextilde,            KeySym_ocircumflextilde);
    add(XK_Ocircumflexbelowdot,         KeySym_Ocircumflexbelowdot);
    add(XK_ocircumflexbelowdot,         KeySym_ocircumflexbelowdot);
    add(XK_Ohornacute,                  KeySym_Ohornacute);
    add(XK_ohornacute,                  KeySym_ohornacute);
    add(XK_Ohorngrave,                  KeySym_Ohorngrave);
    add(XK_ohorngrave,                  KeySym_ohorngrave);
    add(XK_Ohornhook,                   KeySym_Ohornhook);
    add(XK_ohornhook,                   KeySym_ohornhook);
    add(XK_Ohorntilde,                  KeySym_Ohorntilde);
    add(XK_ohorntilde,                  KeySym_ohorntilde);
    add(XK_Ohornbelowdot,               KeySym_Ohornbelowdot);
    add(XK_ohornbelowdot,               KeySym_ohornbelowdot);
    add(XK_Ubelowdot,                   KeySym_Ubelowdot);
    add(XK_ubelowdot,                   KeySym_ubelowdot);
    add(XK_Uhook,                       KeySym_Uhook);
    add(XK_uhook,                       KeySym_uhook);
    add(XK_Uhornacute,                  KeySym_Uhornacute);
    add(XK_uhornacute,                  KeySym_uhornacute);
    add(XK_Uhorngrave,                  KeySym_Uhorngrave);
    add(XK_uhorngrave,                  KeySym_uhorngrave);
    add(XK_Uhornhook,                   KeySym_Uhornhook);
    add(XK_uhornhook,                   KeySym_uhornhook);
    add(XK_Uhorntilde,                  KeySym_Uhorntilde);
    add(XK_uhorntilde,                  KeySym_uhorntilde);
    add(XK_Uhornbelowdot,               KeySym_Uhornbelowdot);
    add(XK_uhornbelowdot,               KeySym_uhornbelowdot);
    add(XK_Ybelowdot,                   KeySym_Ybelowdot);
    add(XK_ybelowdot,                   KeySym_ybelowdot);
    add(XK_Yhook,                       KeySym_Yhook);
    add(XK_yhook,                       KeySym_yhook);
    add(XK_Ytilde,                      KeySym_Ytilde);
    add(XK_ytilde,                      KeySym_ytilde);
    add(XK_Ohorn,                       KeySym_Ohorn);
    add(XK_ohorn,                       KeySym_ohorn);
    add(XK_Uhorn,                       KeySym_Uhorn);
    add(XK_uhorn,                       KeySym_uhorn);

/*
    add(XK_combining_tilde,             KeySym_combining_tilde);
    add(XK_combining_grave,             KeySym_combining_grave);
    add(XK_combining_acute,             KeySym_combining_acute);
    add(XK_combining_hook,              KeySym_combining_hook);
    add(XK_combining_belowdot,          KeySym_combining_belowdot);
*/

    // Currency
    add(XK_EcuSign,                     KeySym_EcuSign);
    add(XK_ColonSign,                   KeySym_ColonSign);
    add(XK_CruzeiroSign,                KeySym_CruzeiroSign);
    add(XK_FFrancSign,                  KeySym_FFrancSign);
    add(XK_LiraSign,                    KeySym_LiraSign);
    add(XK_MillSign,                    KeySym_MillSign);
    add(XK_NairaSign,                   KeySym_NairaSign);
    add(XK_PesetaSign,                  KeySym_PesetaSign);
    add(XK_RupeeSign,                   KeySym_RupeeSign);
    add(XK_WonSign,                     KeySym_WonSign);
    add(XK_NewSheqelSign,               KeySym_NewSheqelSign);
    add(XK_DongSign,                    KeySym_DongSign);
    add(XK_EuroSign,                    KeySym_EuroSign);
}

} // unnamed namespace


namespace archon {
namespace display {

Implementation::Ptr get_implementation_x11()
{
    static Implementation::Ptr impl(new ImplementationImpl());
    return impl;
}

} // namespace display
} // namespace archon
