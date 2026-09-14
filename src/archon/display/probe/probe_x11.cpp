// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <memory>
#include <algorithm>
#include <optional>
#include <tuple>
#include <vector>
#include <string_view>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>
#include <ostream>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>
#include <archon/display/mouse_button.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/noinst/impl_util.hpp>
#include <archon/display/noinst/x11/support.hpp>


using namespace archon;
namespace impl = display::impl;
namespace x11 = impl::x11;


#if ARCHON_DISPLAY_HAVE_GOOD_X11


namespace {


auto get_grab_result_name(int ret) noexcept -> const char*
{
    switch (ret) {
        case GrabSuccess:
            return "GrabSuccess";
        case GrabNotViewable:
            return "GrabNotViewable";
        case AlreadyGrabbed:
            return "AlreadyGrabbed";
        case GrabFrozen:
            return "GrabFrozen";
        case GrabInvalidTime:
            return "GrabInvalidTime";
    }
    return "?";
}


bool try_grab_pointer(Display* dpy, Window win, bool motion, log::Logger& logger)
{
    Window grab_window = win;
    Bool owner_events = False;
    unsigned int event_mask = (ButtonPressMask | ButtonReleaseMask |
                               EnterWindowMask | LeaveWindowMask);
    if (motion)
        event_mask |= PointerMotionMask;
    int pointer_mode = GrabModeAsync;
    int keyboard_mode = GrabModeAsync;
    Window confine_to = None; // No confinement
    Cursor cursor = None; // Leave cursor as is
    Time time = CurrentTime;
    int ret = XGrabPointer(dpy, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor, time);
    if (ARCHON_LIKELY(ret == GrabSuccess))
        return true;
    logger.error("Grab failed: %s", get_grab_result_name(ret));
    return false;
}


bool try_grab_pointer_xi(Display* dpy, int deviceid, Window win, bool motion, log::Logger& logger)
{
    unsigned char mask_bytes[XIMaskLen(XI_LASTEVENT)] = {};
    if (motion)
        XISetMask(mask_bytes, XI_Motion);
    XISetMask(mask_bytes, XI_ButtonPress);
    XISetMask(mask_bytes, XI_ButtonRelease);
    XISetMask(mask_bytes, XI_Enter);
    XISetMask(mask_bytes, XI_Leave);

    XIEventMask mask = {};
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = sizeof mask_bytes;
    mask.mask = mask_bytes;

    Window grab_window = win;
    Time time = CurrentTime;
    Cursor cursor = None; // Leave cursor as is
    int grab_mode = GrabModeAsync;
    int paired_device_mode = GrabModeAsync;
    Bool owner_events = False;
    Status status = XIGrabDevice(dpy, deviceid, grab_window, time, cursor, grab_mode, paired_device_mode, owner_events, &mask);
    int ret = status; // Meaning of returned value does not match declared return type (API design error).
    if (ARCHON_LIKELY(ret == GrabSuccess))
        return true;
    logger.error("Grab failed: %s", get_grab_result_name(ret));
    return false;
}


void ungrab_pointer(Display* dpy) noexcept
{
    Time time = CurrentTime;
    XUngrabPointer(dpy, time);
}


void ungrab_pointer_xi(Display* dpy, int deviceid) noexcept
{
    Time time = CurrentTime;
    XIUngrabDevice(dpy, deviceid, time);
}


auto get_notify_mode_name(int mode) noexcept -> const char*
{
    switch (mode) {
        case NotifyNormal:
            return "NotifyNormal";
        case NotifyGrab:
            return "NotifyGrab";
        case NotifyUngrab:
            return "NotifyUngrab";
        case NotifyWhileGrabbed:
            return "NotifyWhileGrabbed";
    }
    return "?";
}


auto get_notify_mode_name_xi(int mode) noexcept -> const char*
{
    switch (mode) {
        case XINotifyNormal:
            return "NotifyNormal";
        case XINotifyGrab:
            return "NotifyGrab";
        case XINotifyUngrab:
            return "NotifyUngrab";
        case XINotifyWhileGrabbed:
            return "NotifyWhileGrabbed";
        case XINotifyPassiveGrab:
            return "NotifyPassiveGrab";
        case XINotifyPassiveUngrab:
            return "NotifyPassiveUngrab";
    }
    return "?";
}


auto get_notify_detail_name(int detail) noexcept -> const char*
{
    switch (detail) {
        case NotifyAncestor:
            return "NotifyAncestor";
        case NotifyVirtual:
            return "NotifyVirtual";
        case NotifyInferior:
            return "NotifyInferior";
        case NotifyNonlinear:
            return "NotifyNonlinear";
        case NotifyNonlinearVirtual:
            return "NotifyNonlinearVirtual";
        case NotifyPointer:
            return "NotifyPointer";
        case NotifyPointerRoot:
            return "NotifyPointerRoot";
        case NotifyDetailNone:
            return "NotifyDetailNone";
    }
    return "?";
}


auto get_notify_detail_name_xi(int detail) noexcept -> const char*
{
    switch (detail) {
        case XINotifyAncestor:
            return "NotifyAncestor";
        case XINotifyVirtual:
            return "NotifyVirtual";
        case XINotifyInferior:
            return "NotifyInferior";
        case XINotifyNonlinear:
            return "NotifyNonlinear";
        case XINotifyNonlinearVirtual:
            return "NotifyNonlinearVirtual";
        case XINotifyPointer:
            return "NotifyPointer";
        case XINotifyPointerRoot:
            return "NotifyPointerRoot";
        case XINotifyDetailNone:
            return "NotifyDetailNone";
    }
    return "?";
}


auto get_device_change_reason_name(int reason) noexcept -> const char*
{
    switch (reason) {
        case XISlaveSwitch:
            return "SlaveSwitch";
        case XIDeviceChange:
            return "DeviceChange";
    }
    return "?";
}


auto get_hierarchy_change_flag_name(int flag) noexcept -> const char*
{
    switch (flag) {
        case XIMasterAdded:
            return "MasterAdded";
        case XIMasterRemoved:
            return "MasterRemoved";
        case XISlaveAttached:
            return "SlaveAttached";
        case XISlaveDetached:
            return "SlaveDetached";
        case XISlaveAdded:
            return "SlaveAdded";
        case XISlaveRemoved:
            return "SlaveRemoved";
        case XIDeviceEnabled:
            return "DeviceEnabled";
        case XIDeviceDisabled:
            return "DeviceDisabled";
    }
    return "?";
}


// This function compares the two specified Xlib request serial numbers and returns true if,
// and only if `a` precedes `b`. It does this in a circular manner, because the serial
// number will wrap around after two to the power of N, where N is the number of bits in
// `unsigned long`. For this reason, this function works correctly only when the actual
// distance between `a` and `b`, in number of X requests, is definitely less than half of
// two to the power of N. Note than N is at least 32 on all platforms.
//
bool x11_request_serial_number_precedes(unsigned long a, unsigned long b) noexcept
{
    return (a - b > core::int_max<unsigned long>() / 2);
}


auto x11_intern_string(Display* dpy, const char* string) noexcept -> Atom
{
    Atom atom = XInternAtom(dpy, string, False);
    ARCHON_STEADY_ASSERT(atom != None);
    return atom;
}


struct x11_input_atoms {
    Atom rel_x, rel_y;

    Atom button_left, button_middle, button_right;
    Atom button_wheel_up, button_wheel_down;
    Atom button_horiz_wheel_left, button_horiz_wheel_right;
    Atom button_side, button_extra;
};


void x11_init_input_atoms(Display* dpy, x11_input_atoms& atoms) noexcept
{
    atoms.button_left              = x11_intern_string(dpy, "Button Left");
    atoms.button_middle            = x11_intern_string(dpy, "Button Middle");
    atoms.button_right             = x11_intern_string(dpy, "Button Right");
    atoms.button_wheel_up          = x11_intern_string(dpy, "Button Wheel Up");
    atoms.button_wheel_down        = x11_intern_string(dpy, "Button Wheel Down");
    atoms.button_horiz_wheel_left  = x11_intern_string(dpy, "Button Horiz Wheel Left");
    atoms.button_horiz_wheel_right = x11_intern_string(dpy, "Button Horiz Wheel Right");
    atoms.button_side              = x11_intern_string(dpy, "Button Side");
    atoms.button_extra             = x11_intern_string(dpy, "Button Extra");

    atoms.rel_x = x11_intern_string(dpy, "Rel X");
    atoms.rel_y = x11_intern_string(dpy, "Rel Y");
}


// Each entry specifies the button identifier used by XInput2 for the button function
// associated with that entry, or zero if the pointer device is reported by XInput2 as not
// having that button function. The associations between slot index and the 9 standard
// button functions, as represented by the corresponding XInput2 button labels, is shown
// below.
//
//          | Normalized |
//    Slot  | X11 button |
//    index | identifier | XInput2 label
//   -------|------------|----------------------------
//    0     | 1          | `Button Left`
//    1     | 2          | `Button Middle`
//    2     | 3          | `Button Right`
//    3     | 4          | `Button Wheel Up`
//    4     | 5          | `Button Wheel Down`
//    5     | 6          | `Button Horiz Wheel Left`
//    6     | 7          | `Button Horiz Wheel Right`
//    7     | 8          | `Button Side`
//    8     | 9          | `Button Extra`
//
// The button map (`x11_pointer_button_map`) should be considered unaffected by system-level
// or device-level button re-mapping (such as through use of the `xinput set-button-map`
// command). This means that system-level and device-level button re-mapping is respected by
// the Archon display library. For example, if a left-handed user has swapped the left and
// right mouse buttons and clicks the right button, then, because of the button remapping by
// the user, XInput2 reports the button identifier that, by default, is associated with the
// left button, which is the button identifier that will be found in first slot of the
// button map (`x11_pointer_button_map`). Therefore, the user's "right click" will be
// correctly interpreted as a "left click". A similar story holds for inversion of scroll
// direction through swapping of `Button Wheel Up` and `Button Wheel Down`.
//
using x11_pointer_button_map = std::array<int, 9>;


// Map specified XInput2 button identifier to normalized X11 button identifier
bool x11_try_normalize_pointer_button(int xi_button, const x11_pointer_button_map& button_map,
                                      int& x11_button) noexcept
{
    auto i = std::find(std::begin(button_map), std::end(button_map), xi_button);
    if (ARCHON_LIKELY(i != std::end(button_map))) {
        auto index = i - std::begin(button_map);
        x11_button = int(1 + index);
        return true;
    }
    return false;
}


// Map normalized X11 button identifier to well defined button or scrolling amount
bool x11_try_map_pointer_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                                math::Vector2F& amount) noexcept
{
    switch (x11_button) {
        case 1:
            is_scroll = false;
            button = display::MouseButton::left;
            return true;
        case 2:
            is_scroll = false;
            button = display::MouseButton::middle;
            return true;
        case 3:
            is_scroll = false;
            button = display::MouseButton::right;
            return true;
        case 4:
            is_scroll = true;
            amount = { 0, +1 }; // Scroll up
            return true;
        case 5:
            is_scroll = true;
            amount = { 0, -1 }; // Scroll down
            return true;
        case 6:
            is_scroll = true;
            amount = { -1, 0 }; // Scroll left
            return true;
        case 7:
            is_scroll = true;
            amount = { +1, 0 }; // Scroll right
            return true;
        case 8:
            is_scroll = false;
            button = display::MouseButton::x1;
            return true;
        case 9:
            is_scroll = false;
            button = display::MouseButton::x2;
            return true;
    }
    return false;
}


struct x11_pointer_device_properties {
    x11_pointer_button_map button_map;

    // Valuator indexes
    int x_valuator;
    int y_valuator;
};


// Check if device has both "Rel X" and "Rel Y" relative-mode valuators.
bool x11_vet_pointer_device(const XIAnyClassInfo* const* classes, int n, const x11_input_atoms& atoms,
                            x11_pointer_device_properties& properties) noexcept
{
    x11_pointer_button_map button_map = {};
    std::optional<int> x_index, y_index;
    for (int i = 0; i < n; ++i) {
        const XIAnyClassInfo& info = *classes[i];
        switch (info.type) {
            case XIButtonClass: {
                const XIButtonClassInfo& button_info = reinterpret_cast<const XIButtonClassInfo&>(info);
                Atom labels[] = {
                    atoms.button_left,
                    atoms.button_middle,
                    atoms.button_right,
                    atoms.button_wheel_up,
                    atoms.button_wheel_down,
                    atoms.button_horiz_wheel_left,
                    atoms.button_horiz_wheel_right,
                    atoms.button_side,
                    atoms.button_extra,
                };
                static_assert(std::size(labels) == std::size(button_map));
                button_map = {};
                for (int i = 0; i < button_info.num_buttons; ++i) {
                    Atom label = button_info.labels[i];
                    auto j = std::find(std::begin(labels), std::end(labels), label);
                    if (ARCHON_LIKELY(j != std::end(labels))) {
                        auto index = j - std::begin(labels);
                        button_map[index] = 1 + i;
                    }
                }
                break;
            }
            case XIValuatorClass: {
                const XIValuatorClassInfo& valuator_info = reinterpret_cast<const XIValuatorClassInfo&>(info);
                if (valuator_info.mode == XIModeRelative) {
                    if (valuator_info.label == atoms.rel_x) {
                        x_index = valuator_info.number;
                    }
                    else if (valuator_info.label == atoms.rel_y) {
                        y_index = valuator_info.number;
                    }
                }
                break;
            }
        }
    }
    if (x_index.has_value() && y_index.has_value()) {
        properties = {
            button_map,
            x_index.value(),
            y_index.value(),
        };
        return true;
    }
    return false;
}


bool x11_is_genuine_motion_event(const XIDeviceEvent& ev, const x11_pointer_device_properties& properties) noexcept
{
    bool x_axis_change = bool(XIMaskIsSet(ev.valuators.mask, properties.x_valuator));
    bool y_axis_change = bool(XIMaskIsSet(ev.valuators.mask, properties.y_valuator));
    return (x_axis_change || y_axis_change);
}


struct x11_input_seat {
    bool removed;
    bool is_relative; // Pointing device has both "Rel X" and "Rel Y" relative-mode valuators

    // Device identifiers
    int pointer_device;
    int keyboard_device;

    x11_pointer_device_properties pointer_properties;
};


void x11_handle_xinput_device_change(const XIDeviceChangedEvent& ev, const x11_input_atoms& atoms,
                                     x11_input_seat& seat) noexcept
{
    if (seat.removed || ev.deviceid != seat.pointer_device)
        return;

    // Ignoring `ev.reason` because both reason codes (SlaveSwitch, DeviceChange) may entail
    // class changes
    seat.is_relative = x11_vet_pointer_device(ev.classes, ev.num_classes, atoms, seat.pointer_properties);
}


void x11_handle_xinput_hierarchy_change(const XIHierarchyEvent& ev, x11_input_seat& seat) noexcept
{
    if (seat.removed || (ev.flags & XIMasterRemoved) == 0)
        return;

    // We only care about removal of a selected device. We need to know if one of the
    // selected devices is removed, so that we do not mistake a subsequently added device,
    // that happens to be assigned the same device identifier, for the removed one.
    //
    // If one of the two master devices of a seat is removed (pointer or keyboard), both are
    // removed, so we only need to check on one of them (the pointer device).
    //
    for (int i = 0; i < ev.num_info; ++i) {
        const XIHierarchyInfo& info = ev.info[i];
        if (info.deviceid == seat.pointer_device) {
            if ((info.flags & XIMasterRemoved) != 0) {
                seat.removed = true;
                break;
            }
        }
    }
}


// Find an appropriate relative-mode master pointer device and its associated master
// keyboard device.
//
// The association between the two master devices can be considered fixed for the lifetime
// of the devices. Note that the destruction of one of the two devices causes immediate
// destruction of the other. The XInput API does not offer any means for changing the
// association, other than by destroying the device pair. Additionally, the events that
// report hierarchy change generally has flags to inform about anything that can change, but
// has no flag to inform about a change in association. This lends further support to the
// assumption. Finally, the server, in its current form, never itself breaks the association
// in a way that is visible to clients.
//
// On success, the event barrier is set to the request serial number corresponding to the
// device hierarchy snapshot, as returned by XIQueryDevice(), on the basis of which the
// selection was made. XI_DeviceChanged and XI_HierarchyChanged events with earlier serial
// numbers must be ignored in order to properly track changes to the selected devices.
//
bool x11_select_input_seat(Display* dpy, const x11_input_atoms& atoms, x11_input_seat& seat,
                           unsigned long& event_barrier)
{
    int ndevices = {};
    XIDeviceInfo* devices = XIQueryDevice(dpy, XIAllMasterDevices, &ndevices);
    ARCHON_SCOPE_EXIT {
        if (ARCHON_LIKELY(devices))
            XIFreeDeviceInfo(devices);
    };
    for (int i = 0; i < ndevices; ++i) {
        const XIDeviceInfo& device = devices[i];
        if (device.use != XIMasterPointer)
            continue;
        x11_pointer_device_properties pointer_properties = {};
        bool is_relative = x11_vet_pointer_device(device.classes, device.num_classes, atoms, pointer_properties);
        if (is_relative) {
            bool removed = false;
            int pointer_device = device.deviceid;
            int keyboard_device = device.attachment;
            seat = {
                removed,
                is_relative,
                pointer_device,
                keyboard_device,
                pointer_properties,
            };
            event_barrier = XLastKnownRequestProcessed(dpy);
            return true;
        }
    }
    return false;
}


// This function discards queued events that carries a request serial number that precedes
// the specified barrier. It only considers events that are already are already read from
// the display connection and queued on the client side. It discards events of all types.
//
// Whether a particular serial number precedes the specified barrier is judged as if by
// x11_request_serial_number_precedes(), which means that x11_discard_prebarrier_events()
// operates correctly only when the maximum actual distance between a serial number of a
// queued event and the specified barrier is less than half of two to the power of N, where
// N is the number of value bits in `unsigned long`. N is at least 32.
//
void x11_discard_prebarrier_events(Display* dpy, unsigned long event_barrier) noexcept
{
    XEvent ev = {};
    int n = XEventsQueued(dpy, QueuedAlready);
    for (;;) {
        if (ARCHON_LIKELY(n == 0))
            break;
        XPeekEvent(dpy, &ev);
        if (ARCHON_LIKELY(!x11_request_serial_number_precedes(ev.xany.serial, event_barrier)))
            break;
        XNextEvent(dpy, &ev); // Discard
        n -= 1;
    }
}


bool x11_are_any_xinput_buttons_pressed(const XIButtonState& state)
{
    for (int i = 0; i < state.mask_len; ++i) {
        if ((state.mask[i] & 0xFF) == 0)
            continue;
        return true;
    }
    return false;
}


// Compatible with XKeymapEvent::key_vector
class X11KeyCodeSet {
public:
    void assign(const char* bytes) noexcept
    {
        std::copy_n(bytes, 32, m_bytes);
    }

    bool contains(KeyCode keycode) const noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        return ((byte(i) & bit(i)) != 0);
    }

    void add(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) |= bit(i);
    }

    void remove(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) &= ~bit(i);
    }

private:
    char m_bytes[32] = {};

    auto byte(int i) const noexcept -> const unsigned char&
    {
        return reinterpret_cast<const unsigned char*>(m_bytes)[i / 8];
    }

    auto byte(int i) noexcept -> unsigned char&
    {
        return reinterpret_cast<unsigned char*>(m_bytes)[i / 8];
    }

    static int bit(int i) noexcept
    {
        return 1 << (i % 8);
    }
};


class ColormapFinderImpl final
    : public x11::ColormapFinder {
public:
    using standard_colormaps_type = core::FlatMap<VisualID, XStandardColormap>;
    ColormapFinderImpl(VisualID m_default_visual, Colormap m_default_colormap,
                       const standard_colormaps_type& m_standard_colormaps) noexcept;

    bool find_default_colormap(VisualID, Colormap&) const noexcept override;
    bool find_standard_colormap(VisualID, XStandardColormap&) const override;

private:
    VisualID m_default_visual;
    Colormap m_default_colormap;
    const core::FlatMap<VisualID, XStandardColormap>& m_standard_colormaps;
};


inline ColormapFinderImpl::ColormapFinderImpl(VisualID default_visual, Colormap default_colormap,
                                              const standard_colormaps_type& standard_colormaps) noexcept
    : m_default_visual(default_visual)
    , m_default_colormap(default_colormap)
    , m_standard_colormaps(standard_colormaps)
{
}


bool ColormapFinderImpl::find_default_colormap(VisualID visual, Colormap& colormap) const noexcept
{
    if (visual == m_default_visual) {
        colormap = m_default_colormap;
        return true;
    }
    return false;
}


bool ColormapFinderImpl::find_standard_colormap(VisualID visual, XStandardColormap& colormap_params) const
{
    auto i = m_standard_colormaps.find(visual);
    if (i != m_standard_colormaps.end()) {
        colormap_params = i->second;
        return true;
    }
    return false;
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale();

    namespace fs = std::filesystem;
    std::vector<fs::path> paths;
    bool list_visuals = false;
    bool list_pixmap_formats = false;
    std::optional<std::size_t> optional_num_windows;
    std::optional<std::string> optional_window_title;
    util::Color background_color = util::colors::black;
    bool fullscreen = false;
    std::optional<std::string> optional_display;
    std::optional<int> optional_screen;
    std::optional<display::x11_fullscreen_monitors> optional_fullscreen_monitors;
    std::optional<int> optional_visual_depth;
    std::optional<display::x11_connection_config::VisualClass> optional_visual_class;
    std::optional<VisualID> optional_visual_type;
    bool fullscreen_bypass_compositor = false;
    bool prefer_default_nondecomposed_colormap = false;
    bool disable_double_buffering = false;
    bool disable_detectable_autorepeat = false;
    std::optional<display::Pos> optional_pos;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool report_mouse_motion = false;
    bool use_xinput = false;
    bool override_redirect = false;
    bool set_input_focus = false;
    bool synchronous_mode = false;
    bool install_colormap = false;
    bool colormap_weirdness = false;

    cli::Spec spec;
    pat("[<path>...]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(paths)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-L, --list-visuals", "", cli::no_attributes, spec,
        "List the supported X11 visuals.",
        cli::raise_flag(list_visuals)); // Throws

    opt("-M, --list-pixmap-formats", "", cli::no_attributes, spec,
        "List the supported ZPixmap formats.",
        cli::raise_flag(list_pixmap_formats)); // Throws

    opt("-n, --num-windows", "<num>", cli::no_attributes, spec,
        "The number of windows to be opened. It can be zero. By default, one window will be opened for each of the "
        "specified images, or if no images are specified, the default number of windows is 1.",
        std::tie(optional_num_windows)); // Throws

    opt("-T, --window-title", "<string>", cli::no_attributes, spec,
        "Set an alternate text to be used as window title.",
        cli::assign(optional_window_title)); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set the background color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as "
        "well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default "
        "color is @Q.",
        cli::assign(util::as_css_color(background_color))); // Throws

    opt("-f, --fullscreen", "", cli::no_attributes, spec,
        "Open first window in fullscreen mode.",
        cli::raise_flag(fullscreen)); // Throws

    opt("-D, --display", "<string>", cli::no_attributes, spec,
        "Target the specified X11 display (@A). If this option is not specified, the value of the DISPLAY environment "
        "variable will be used.",
        cli::assign(optional_display)); // Throws

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A) of the targeted display. If this option is not specified, the default "
        "screen will be used.",
        cli::assign(optional_screen)); // Throws

    opt("-F, --fullscreen-monitors", "<monitors>", cli::no_attributes, spec,
        "Use the specified Xinerama screens (monitors) to define the fullscreen area. \"@A\" can be specified as one, "
        "two, or four comma-separated Xinerama screen indexes (`xrandr --listactivemonitors`). When four values are "
        "specified they will be interpreted as the Xinerama screens that determine the top, bottom, left, and right "
        "edges of the fullscreen area. When two values are specified, the first one determines both top and left "
        "edges and the second one determines bottom and right edges. When one value is specified, it determines all "
        "edges.",
        cli::assign(optional_fullscreen_monitors)); // Throws

    opt("-d, --visual-depth", "<num>", cli::no_attributes, spec,
        "Pick a visual of the specified depth (@A).",
        cli::assign(optional_visual_depth)); // Throws

    opt("-c, --visual-class", "<name>", cli::no_attributes, spec,
        "Pick a visual of the specified class (@A). The class can be @F.",
        cli::assign(optional_visual_class)); // Throws

    opt("-V, --visual-type", "<num>", cli::no_attributes, spec,
        "Pick a visual of the specified type (@A). The type, also known as the visual ID, is a 32-bit unsigned "
        "integer that can be expressed in decimal, hexadecimal (with prefix '0x'), or octal (with prefix '0') form.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            std::uint_fast32_t type = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_flex_int(type)))) {
                if (ARCHON_LIKELY(type <= core::int_mask<std::uint_fast32_t>(32))) {
                    optional_visual_type.emplace(VisualID(type));
                    return true;
                }
            }
            return false;
        })); // Throws

    opt("-P, --fullscreen-bypass-compositor", "", cli::no_attributes, spec,
        "Send hint to bypass X11 compositor while in fullscreen mode.",
        cli::raise_flag(fullscreen_bypass_compositor)); // Throws

    opt("-C, --prefer-default-nondecomposed-colormap", "", cli::no_attributes, spec,
        "Prefer the use of the default colormap when the default visual is used and is a PseudoColor or GrayScale "
        "visual. This succeeds if enough colors can be allocated. Otherwise a new colormap is created.",
        cli::raise_flag(prefer_default_nondecomposed_colormap)); // Throws

    opt("-B, --disable-double-buffering", "", cli::no_attributes, spec,
        "Disable use of double buffering, even when the selected visual supports double buffering.",
        cli::raise_flag(disable_double_buffering)); // Throws

    opt("-A, --disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "Do not enable detectable key auto-repeat mode even when it is supported.",
        cli::raise_flag(disable_detectable_autorepeat)); // Throws

    opt("-p, --pos", "<position>", cli::no_attributes, spec,
        "Specify the desired position of the windows. A window manager, if present, may override this.",
        std::tie(optional_pos)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        std::tie(log_level_limit)); // Throws

    opt("-m, --report-mouse-motion", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(report_mouse_motion)); // Throws

    opt("-u, --use-xinput", "", cli::no_attributes, spec,
        "Use XInput2 events and pointer grabs.",
        cli::raise_flag(use_xinput)); // Throws

    opt("-o, --override-redirect", "", cli::no_attributes, spec,
        "Turn on \"override redirect\" mode for created windows.",
        cli::raise_flag(override_redirect)); // Throws

    opt("-i, --set-input-focus", "", cli::no_attributes, spec,
        "Set input focus to \"self\" when creating windows.",
        cli::raise_flag(set_input_focus)); // Throws

    opt("-y, --synchronous-mode", "", cli::no_attributes, spec,
        "Turn on X11's synchronous mode. In this mode, buffering of X protocol requests is turned off, and the Xlib "
        "functions, that generate X requests, wait for a response from the server before they return. This is "
        "sometimes useful when debugging.",
        cli::raise_flag(synchronous_mode)); // Throws

    opt("-I, --install-colormap", "", cli::no_attributes, spec,
        "Install the colormap, i.e., make it current. This should only be done when there is no window manager.",
        cli::raise_flag(install_colormap)); // Throws

    opt("-W, --colormap-weirdness", "", cli::no_attributes, spec,
        "Use a weird (non-standard) palette when using a visual that allows for palette mutation (`PseudoColor`, "
        "`GrayScale`, and `DirectColor`).",
        cli::raise_flag(colormap_weirdness)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_stdout(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // `src_root` is the relative path to the root of the source tree from the root of the
    // project.
    //
    // `src_path` is the relative path to this source file from the root of source tree.
    //
    // `bin_path` is the relative path to the executable from the root of the source root as
    // it is reflected into the build directory.
    //
    core::BuildEnvironment::Params build_env_params;
    build_env_params.file_path = __FILE__;
    build_env_params.bin_path  = "archon/display/probe/archon-probe-x11";
    build_env_params.src_path  = "archon/display/probe/probe_x11.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/display/probe", locale)); // Throws

    // Load images
    if (paths.empty()) {
        fs::path path = resource_path / core::make_fs_path_generic("image.png", locale); // Throws
        paths.push_back(path); // Throws
    }
    core::Slab<std::unique_ptr<image::Image>> images(paths.size()); // Throws
    {
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        image::LoadConfig load_config;
        load_config.logger = &load_logger;
        for (const fs::path& path : paths) {
            std::unique_ptr<image::WritableImage> img;
            std::error_code ec;
            if (!image::try_load(path, img, locale, load_config, ec)) { // Throws
                logger.error("%s: Failed to load image: %s", core::as_native_path(path), ec.message()); // Throws
                return EXIT_FAILURE;
            }
            images.add(std::move(img));
        }
    }

    // Connect to display
    std::string_view display = x11::get_display_string(optional_display);
    x11::DisplayWrapper dpy_owner;
    if (ARCHON_UNLIKELY(!x11::try_connect(display, dpy_owner))) { // Throws
        logger.error("Failed to open X11 display connection to %s", core::quoted(display)); // Throws
        return EXIT_FAILURE;
    }
    Display* dpy = dpy_owner;

    if (ARCHON_UNLIKELY(synchronous_mode))
        XSynchronize(dpy, True);

    int screen = x11::get_screen_index(dpy, optional_screen);
    if (ARCHON_UNLIKELY(!x11::valid_screen_index(dpy, screen))) {
        logger.error("Invalid screen index (%s)", core::as_int(screen)); // Throws
        return EXIT_FAILURE;
    }

    x11::ExtensionInfo extension_info = x11::init_extensions(dpy); // Throws
    x11::ScreenInfo screen_info = x11::get_screen_info(dpy, extension_info, screen); // Throws
    Window root = screen_info.root;

    bool detectable_autorepeat_enabled = false;
    if (extension_info.have_xkb && !disable_detectable_autorepeat && !use_xinput) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            detectable_autorepeat_enabled = true;
    }

    x11_input_atoms input_atoms = {};
    x11_init_input_atoms(dpy, input_atoms);

    x11_input_seat input_seat = {};
    if (use_xinput) {
        // It is necessary to register interest in XI_DeviceChanged and XI_HierarchyChanged
        // events before selecting a particular input seat (master input device and master
        // keyboard device). This ensures that there are no changes to the selected devices that
        // go unnoticed.
        //
        // XI_HierarchyChanged requires XIAllDevices (not just XIAllMasterDevices), hence the
        // two separate masks.
        //
        unsigned char mask_bytes_1[XIMaskLen(XI_LASTEVENT)] = {};
        XISetMask(mask_bytes_1, XI_DeviceChanged);

        unsigned char mask_bytes_2[XIMaskLen(XI_LASTEVENT)] = {};
        XISetMask(mask_bytes_2, XI_HierarchyChanged);

        XIEventMask mask_1 = {};
        mask_1.deviceid = XIAllMasterDevices;
        mask_1.mask_len = sizeof mask_bytes_1;
        mask_1.mask = mask_bytes_1;

        XIEventMask mask_2 = {};
        mask_2.deviceid = XIAllDevices;
        mask_2.mask_len = sizeof mask_bytes_2;
        mask_2.mask = mask_bytes_2;

        XIEventMask masks[] = {
            mask_1,
            mask_2,
        };

        XISelectEvents(dpy, root, masks, std::size(masks));

        unsigned long event_barrier = {};
        if (ARCHON_UNLIKELY(!x11_select_input_seat(dpy, input_atoms, input_seat, event_barrier))) { // Throws
            logger.error("Failed to find appropriate input seat (associated pointer and keyboard device pair)"); // Throws
            return EXIT_FAILURE;
        }

        x11_discard_prebarrier_events(dpy, event_barrier);
    }

#if ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR
    if (ARCHON_LIKELY(extension_info.have_xrandr)) {
        int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
        XRRSelectInput(dpy, root, mask);
    }
#endif // ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR

    // Key is visual depth
    core::FlatMap<int, XPixmapFormatValues> pixmap_formats = x11::fetch_pixmap_formats(dpy); // Throws

    core::FlatMap<VisualID, XStandardColormap> standard_colormaps =
        x11::fetch_standard_colormaps(dpy, root); // Throws

    // Fetch depths
    std::vector<int> depths;
    {
        int n = 0;
        int* entries = XListDepths(dpy, screen, &n);
        ARCHON_STEADY_ASSERT(entries);
        for (int i = 0; i < n; ++i)
            depths.push_back(entries[i]);
        XFree(entries);
    }

    core::Slab<x11::VisualSpec> visual_specs = x11::load_visuals(dpy, extension_info, screen_info); // Throws

    // List supported visuals
    if (list_visuals) {
        std::size_t n = visual_specs.size();
        for (std::size_t i = 0; i < n; ++i) {
            const x11::VisualSpec& spec = visual_specs[i];
            const XVisualInfo& info = spec.info;
            auto format_double_buffered = [&](std::ostream& out) {
                if (spec.double_buffered) {
                    if (spec.double_buffered_perflevel != 0) {
                        out << core::formatted("yes (%s)", core::as_int(spec.double_buffered_perflevel)); // Throws
                    }
                    else {
                        out << "yes"; // Throws
                    }
                }
                else {
                    out << "no"; // Throws
                }
            };
            logger.info("Visual %s: visualid = %s, screen = %s, depth = %s, class = %s, "
                        "red_mask = %s, green_mask = %s, blue_mask = %s, colormap_size = %s, bits_per_rgb = %s, "
                        "double_buffered = %s, supports_opengl = %s, opengl_level = %s, opengl_double_buffered = %s, "
                        "opengl_stereo = %s, opengl_num_aux_buffers = %s, opengl_depth_buffer_bits = %s, "
                        "opengl_stencil_buffer_bits = %s, opengl_accum_buffer_bits = %s", i + 1,
                        core::as_flex_int_h(info.visualid), core::as_int(info.screen), core::as_int(info.depth),
                        x11::get_visual_class_name(info.c_class), core::as_flex_int_h(info.red_mask),
                        core::as_flex_int_h(info.green_mask), core::as_flex_int_h(info.blue_mask),
                        core::as_int(info.colormap_size), core::as_int(info.bits_per_rgb),
                        core::as_format_func(format_double_buffered), (spec.opengl_supported ? "yes" : "no"),
                        core::as_int(spec.opengl_level), (spec.opengl_double_buffered ? "yes" : "no"),
                        (spec.opengl_stereo ? "yes" : "no"), core::as_int(spec.opengl_num_aux_buffers),
                        core::as_int(spec.opengl_depth_buffer_bits), core::as_int(spec.opengl_stencil_buffer_bits),
                        core::as_int(spec.opengl_accum_buffer_bits)); // Throws
        }
    }

    // List supported ZPixmap formats
    if (list_pixmap_formats) {
        std::size_t i = 0;
        for (const auto& entry : pixmap_formats) {
            const XPixmapFormatValues& format = entry.second;
            logger.info("Format %s: depth = %s, bits_per_pixel = %s, scanline_pad = %s", i + 1, format.depth,
                        format.bits_per_pixel, format.scanline_pad); // Throws
            ++i;
        }
    }

    // Choose visual (depth and type)
    x11::FindVisualParams params;
    params.visual_depth = optional_visual_depth;
    params.visual_class = x11::map_opt_visual_class(optional_visual_class);
    params.visual_type = optional_visual_type;
    params.prefer_double_buffered = !disable_double_buffering;
    std::size_t index = {};
    if (ARCHON_UNLIKELY(!x11::find_visual(dpy, screen, visual_specs, params, index))) { // Throws
        logger.error("No suitable X11 visual found");
        return EXIT_FAILURE;
    }
    const x11::VisualSpec& visual_spec = visual_specs[index];
    const XVisualInfo& visual_info = visual_spec.info;
    int depth = visual_info.depth;
    VisualID visualid = visual_info.visualid;
    bool use_double_buffering = visual_spec.double_buffered;
    const XPixmapFormatValues& pixmap_format = pixmap_formats.at(depth); // Throws

    auto format_have_xkb = [&](std::ostream& out) {
        if (extension_info.have_xkb) {
            out << core::formatted("yes (%s.%s)", extension_info.xkb_major, extension_info.xkb_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xinput = [&](std::ostream& out) {
        if (extension_info.have_xinput) {
            out << core::formatted("yes (%s.%s)", extension_info.xinput_major, extension_info.xinput_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xfixes = [&](std::ostream& out) {
        if (extension_info.have_xfixes) {
            out << core::formatted("yes (%s.%s)", extension_info.xfixes_major, extension_info.xfixes_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xdbe = [&](std::ostream& out) {
        if (extension_info.have_xdbe) {
            out << core::formatted("yes (%s.%s)", extension_info.xdbe_major, extension_info.xdbe_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xrandr = [&](std::ostream& out) {
        if (extension_info.have_xrandr) {
            out << core::formatted("yes (%s.%s)", extension_info.xrandr_major, extension_info.xrandr_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xrender = [&](std::ostream& out) {
        if (extension_info.have_xrender) {
            out << core::formatted("yes (%s.%s)", extension_info.xrender_major,
                                   extension_info.xrender_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_glx = [&](std::ostream& out) {
        if (screen_info.have_glx) {
            out << core::formatted("yes (%s.%s)", extension_info.glx_major, extension_info.glx_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    logger.info("Display string:                     %s", DisplayString(dpy)); // Throws
    logger.info("Server vendor:                      %s", ServerVendor(dpy)); // Throws
    logger.info("Vendor release:                     %s", core::as_int(VendorRelease(dpy))); // Throws
    logger.info("Have Xkb:                           %s", core::as_format_func(format_have_xkb)); // Throws
    logger.info("Have XInput:                        %s", core::as_format_func(format_have_xinput)); // Throws
    logger.info("Have Xfixes:                        %s", core::as_format_func(format_have_xfixes)); // Throws
    logger.info("Have Xdbe:                          %s", core::as_format_func(format_have_xdbe)); // Throws
    logger.info("Have XRandR:                        %s", core::as_format_func(format_have_xrandr)); // Throws
    logger.info("Have Xrender:                       %s", core::as_format_func(format_have_xrender)); // Throws
    logger.info("Have GLX:                           %s", core::as_format_func(format_have_glx)); // Throws
    logger.info("Image byte order:                   %s",
                (ImageByteOrder(dpy) == LSBFirst ? "little-endian" : "big-endian")); // Throws
    logger.info("Bitmap bit order:                   %s",
                (BitmapBitOrder(dpy) == LSBFirst ? "least significant bit first" :
                 "most significant bit first")); // Throws
    logger.info("Bitmap scanline pad:                %s", core::as_int(BitmapPad(dpy))); // Throws
    logger.info("Bitmap scanline unit:               %s", core::as_int(BitmapUnit(dpy))); // Throws
    logger.info("Number of screens:                  %s", core::as_int(ScreenCount(dpy))); // Throws
    logger.info("Selected screen:                    %s", core::as_int(screen + 1)); // Throws
    logger.info("Size of screen:                     %spx x %spx (%smm x %smm)",
                core::as_int(DisplayWidth(dpy, screen)), core::as_int(DisplayHeight(dpy, screen)),
                core::as_int(DisplayWidthMM(dpy, screen)), core::as_int(DisplayHeightMM(dpy, screen))); // Throws
    logger.info("Resolution of screen (dpcm):        %s x %s",
                10 * (DisplayWidth(dpy, screen) / double(DisplayWidthMM(dpy, screen))),
                10 * (DisplayHeight(dpy, screen) / double(DisplayHeightMM(dpy, screen)))); // Throws
    logger.info("Concurrent colormaps of screen:     %s -> %s",
                MinCmapsOfScreen(ScreenOfDisplay(dpy, screen)),
                MaxCmapsOfScreen(ScreenOfDisplay(dpy, screen))); // Throws
    logger.info("Size of default colormap of screen: %s", core::as_int(DisplayCells(dpy, screen))); // Throws
    logger.info("Supported depths on screen:         %s", core::as_list(depths)); // Throws
    logger.info("Default depth of screen:            %s", core::as_int(DefaultDepth(dpy, screen))); // Throws
    logger.info("Selected depth:                     %s", core::as_int(depth)); // Throws
    logger.info("Default visual of screen:           %s", core::as_flex_int_h(screen_info.default_visual)); // Throws
    logger.info("Selected visual:                    %s", core::as_flex_int_h(visualid)); // Throws
    logger.info("Class of selected visual:           %s", x11::get_visual_class_name(visual_info.c_class)); // Throws
    if (!use_xinput) {
        logger.info("Detectable auto-repeat enabled:     %s", (detectable_autorepeat_enabled ?
                                                               "yes" : "no")); // Throws
    }
    logger.info("Use double buffering:               %s", (use_double_buffering ? "yes" : "no")); // Throws
    if (use_xinput) {
        logger.info("Pointer device:                     %s", input_seat.pointer_device); // Throws
        logger.info("Keyboard device:                    %s", input_seat.keyboard_device); // Throws
    }

    if (ARCHON_UNLIKELY(!extension_info.have_xkb)) {
        logger.error("Required X Keyboard Extension is not available");
        return EXIT_FAILURE;
    }

    if (ARCHON_UNLIKELY(use_xinput && !extension_info.have_xinput)) {
        logger.error("Required XInput2 extension is not available");
        return EXIT_FAILURE;
    }

    ColormapFinderImpl colormap_finder(screen_info.default_visual, screen_info.default_colormap, standard_colormaps);
    std::unique_ptr<x11::PixelFormat> pixel_format =
        x11::create_pixel_format(dpy, root, visual_info, pixmap_format, colormap_finder, locale, logger,
                                 prefer_default_nondecomposed_colormap, colormap_weirdness); // Throws
    unsigned long interned_background_color = pixel_format->intern_color(background_color); // Throws
    Colormap colormap = pixel_format->get_colormap();

#if ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR
    x11::ScreenConf screen_conf;
    Atom atom_edid = x11::intern_string(dpy, RR_PROPERTY_RANDR_EDID);
    impl::EdidParser edid_parser(locale);
    auto update_screen_conf = [&] {
        return x11::update_screen_conf(dpy, root, atom_edid, edid_parser, locale, screen_conf); // Throws
    };
    auto dump_screen_conf = [&] {
        const char* strings_base = screen_conf.string_buffer.data();
        std::size_t n = screen_conf.viewports.size();
        for (std::size_t i = 0; i < n; ++i) {
            const x11::ProtoViewport& viewport = screen_conf.viewports[i];
            auto format_monitor_name = [&](std::ostream& out) {
                if (ARCHON_LIKELY(viewport.monitor_name.has_value())) {
                    out << core::quoted(viewport.monitor_name.value().resolve_string(strings_base)); // Throws
                }
                else {
                    out << "unknown"; // Throws
                }
            };
            logger.info("Viewport %s/%s: output_name=%s, bounds=%s, monitor_name=%s, resolution=%s, refresh_rate=%s",
                        i + 1, n, core::quoted(viewport.output_name.resolve_string(strings_base)), viewport.bounds,
                        core::as_format_func(format_monitor_name), core::as_optional(viewport.resolution, "unknown"),
                        core::as_optional(viewport.refresh_rate, "unknown")); // Throws
        }
    };
    if (ARCHON_LIKELY(extension_info.have_xrandr)) {
        update_screen_conf(); // Throws
        dump_screen_conf(); // Throws
    }
#endif // ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR

    // Create graphics context
    XGCValues gc_values = {};
    gc_values.graphics_exposures = False;
    GC gc = XCreateGC(dpy, root, GCGraphicsExposures, &gc_values);
    ARCHON_SCOPE_EXIT {
        XFreeGC(dpy, gc);
    };
    XSetForeground(dpy, gc, interned_background_color);

    // Upload images
    struct PixmapSlot {
        image::Size size;
        Pixmap pixmap;
    };
    core::Slab<PixmapSlot> pixmaps(images.size()); // Throws
    ARCHON_SCOPE_EXIT {
        for (const auto& slot : pixmaps)
            XFreePixmap(dpy, slot.pixmap);
    };
    {
        std::unique_ptr<x11::ImageBridge> bridge =
            pixel_format->create_image_bridge(impl::subdivide_max_subbox_size); // Throws
        image::Writer writer(bridge->img_1); // Throws
        for (const std::unique_ptr<image::Image>& img : images) {
            image::Size size = img->get_size();
            Pixmap pixmap = XCreatePixmap(dpy, root, unsigned(size.width), unsigned(size.height), depth);
            pixmaps.add(PixmapSlot { size, pixmap });
            image::Reader reader(*img); // Throws
            impl::subdivide(size, [&](const display::Box& subbox) {
                image::Pos pos = { 0, 0 };
                writer.put_image_a(pos, reader, subbox); // Throws
                int src_x = pos.x, src_y = pos.y;
                int dest_x = subbox.pos.x, dest_y = subbox.pos.y;
                unsigned width = unsigned(subbox.size.width);
                unsigned height = unsigned(subbox.size.height);
                XPutImage(dpy, pixmap, gc, &bridge->img_2, src_x, src_y, dest_x, dest_y, width, height);
            }); // Throws
        }
    }

    struct WindowSlot {
        bool is_first;
        int no;
        Window window;
        Drawable drawable;
        image::Size win_size;
        image::Size img_size;
        Pixmap pixmap;
        bool input_focus_set = false;
        bool wanted_fullscreen_state = false;
        bool confirmed_fullscreen_state = false;
        bool compositor_bypassed = false;
        bool grabbed = false;
        bool redraw = false;
        bool suppress_redraw = false;
        bool has_input_focus = false;

        WindowSlot(bool is_first_2, int no_2, Window window_2, Drawable drawable_2, image::Size size,
                   Pixmap pixmap_2) noexcept
        {
            is_first = is_first_2;
            no       = no_2;
            window   = window_2;
            drawable = drawable_2;
            win_size = size;
            img_size = size;
            pixmap   = pixmap_2;
        }
    };

    core::FlatMap<Window, WindowSlot> window_slots;

    auto try_get_window_slot = [&](Window win, WindowSlot*& slot) {
        auto i = window_slots.find(win);
        if (ARCHON_LIKELY(i != window_slots.end())) {
            slot = &i->second;
            return true;
        }
        return false;
    };

    // FIXME: Does an X11 request exist for interning a batch of strings at once?
    Atom atom_wm_protocols               = x11::intern_string(dpy, "WM_PROTOCOLS");
    Atom atom_wm_delete_window           = x11::intern_string(dpy, "WM_DELETE_WINDOW");
    Atom atom_net_wm_fullscreen_monitors = x11::intern_string(dpy, "_NET_WM_FULLSCREEN_MONITORS");
    Atom atom_net_wm_state               = x11::intern_string(dpy, "_NET_WM_STATE");
    Atom atom_net_wm_state_fullscreen    = x11::intern_string(dpy, "_NET_WM_STATE_FULLSCREEN");
    Atom atom_net_wm_bypass_compositor   = x11::intern_string(dpy, "_NET_WM_BYPASS_COMPOSITOR");

#if ARCHON_DISPLAY_HAVE_GOOD_X11_XDBE
    XdbeSwapAction swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
#endif

    std::size_t next_window_index = 0;
    std::size_t max_seen_window_slots = 0;
    auto open_window = [&] {
        // Link to an image (round-robin)
        int window_index = next_window_index++;
        ARCHON_ASSERT(pixmaps.size() > 0);
        std::size_t pixmap_index = window_index % pixmaps.size();
        const PixmapSlot& pixmap_slot = pixmaps[pixmap_index];
        image::Size size = pixmap_slot.size;

        // Create window
        long event_mask = (ExposureMask |
                           StructureNotifyMask |
                           VisibilityChangeMask |
                           PropertyChangeMask);
        if (!use_xinput) {
            event_mask |= (KeyPressMask | KeyReleaseMask |
                           ButtonPressMask | ButtonReleaseMask |
                           EnterWindowMask | LeaveWindowMask |
                           FocusChangeMask |
                           KeymapStateMask);
            if (report_mouse_motion)
                event_mask |= ButtonMotionMask;
        }
        display::Pos pos;
        if (optional_pos.has_value())
            pos = optional_pos.value();
        unsigned long valuemask = (CWBackPixel | CWEventMask | CWOverrideRedirect | CWColormap);
        XSetWindowAttributes attributes;
        attributes.background_pixel = interned_background_color;
        attributes.event_mask = event_mask;
        attributes.override_redirect = (override_redirect ? True : False);
        attributes.colormap = colormap;
        Window window = XCreateWindow(dpy, root, pos.x, pos.y, unsigned(size.width), unsigned(size.height), 0, depth,
                                      InputOutput, visual_info.visual, valuemask, &attributes);

        // Unfortunately, with XInput2, it is very complicated to set the event mask for a
        // particular device in a robust way (free of race conditions involving the
        // disappearance of the specified device). Therefore, it is set for all master
        // devices.
        //
        // Unfortunately, with XInput2, it is impossible to request motion events only while
        // a button is pressed, so it is necessary to request them for the entire lifetime
        // of the window.
        //
        if (use_xinput) {
            unsigned char mask_bytes[XIMaskLen(XI_LASTEVENT)] = {};
            XISetMask(mask_bytes, XI_KeyPress);
            XISetMask(mask_bytes, XI_KeyRelease);
            XISetMask(mask_bytes, XI_ButtonPress);
            XISetMask(mask_bytes, XI_ButtonRelease);
            if (report_mouse_motion)
                XISetMask(mask_bytes, XI_Motion);
            XISetMask(mask_bytes, XI_Enter);
            XISetMask(mask_bytes, XI_Leave);
            XISetMask(mask_bytes, XI_FocusIn);
            XISetMask(mask_bytes, XI_FocusOut);
            XIEventMask mask = {};
            mask.deviceid = XIAllMasterDevices;
            mask.mask_len = sizeof mask_bytes;
            mask.mask = mask_bytes;
            XIEventMask masks[] = {
                mask,
            };
            XISelectEvents(dpy, window, masks, std::size(masks));
        }

        // Set window name
        int no = int(window_index + 1);
        std::string name_1;
        std::string_view name_2;
        if (optional_window_title.has_value()) {
            name_2 = optional_window_title.value();
        }
        else {
            name_1 = core::format(locale, "X11 Probe %s", core::as_int(no)); // Throws
            name_2 = name_1;
        }
        x11::TextPropertyWrapper name_3(dpy, name_2, locale); // Throws
        XSetWMName(dpy, window, &name_3.prop);

        // Tell window manager to assign input focus to this window (the "passive focus
        // model" as defined by ICCCM)
        XWMHints hints = {};
        hints.flags = InputHint;
        hints.input = True;
        XSetWMHints(dpy, window, &hints);

        // Set minimum window size
        XSizeHints size_hints;
        size_hints.flags = PMinSize;
        size_hints.min_width  = 128;
        size_hints.min_height = 128;
        if (optional_pos.has_value()) {
            size_hints.flags |= USPosition;
            size_hints.x = pos.x; // Mostly ignored!?
            size_hints.y = pos.y; // Mostly ignored!?
        }
        XSetWMNormalHints(dpy, window, &size_hints);

        // Ask X to notify rather than close connection when window is closed
        x11::set_property_a(dpy, window, atom_wm_protocols, atom_wm_delete_window);

        // Allocate back buffer when using double buffering
        Drawable drawable = window;
#if ARCHON_DISPLAY_HAVE_GOOD_X11_XDBE
        if (use_double_buffering) {
            XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(dpy, window, swap_action);
            drawable = back_buffer;
        }
#endif // ARCHON_DISPLAY_HAVE_GOOD_X11_XDBE

        bool is_first = (window_index == 0);
        WindowSlot slot = { is_first, no, window, drawable, size, pixmap_slot.pixmap };
        window_slots.emplace(window, slot); // Throws

        if (window_slots.size() > max_seen_window_slots)
            max_seen_window_slots = window_slots.size();

        return window;
    };

    auto close_window = [&](Window win) noexcept {
        XDestroyWindow(dpy, win);
        window_slots.erase(win);
    };

    auto set_fullscreen_monitors = [&](Window win) {
        if (ARCHON_LIKELY(!optional_fullscreen_monitors.has_value()))
            return;
        x11::set_fullscreen_monitors(dpy, win, optional_fullscreen_monitors.value(), root,
                                     atom_net_wm_fullscreen_monitors); // Throws
    };

    auto set_compositor_bypass_mode = [&](::Window win, bool on) noexcept {
        // Send EWMH hint to compositor (if present) to "unredirect" the window. If honored,
        // this will bypass the compositing stage and improve performance. Some compositors
        // have this behavior by default (when value is zero). Others do not (require a
        // value of 1).
        unsigned long value = (on ? 1 : 0);
        x11::set_property_32(dpy, win, atom_net_wm_bypass_compositor, value);
    };

    auto ensure_turn_on_off_compositor_bypass_mode = [&](WindowSlot& slot) noexcept {
        // When the "fullscreen compositor bypass" feature is enabled
        // (`fullscreen_bypass_compositor`), compositor bypass mode is automatically turned
        // on and off for windows as they enter and leave fullscreen mode.
        //
        // In this context, the *compositor bypass mode* is to be understood as *turned on*
        // when the window property _NET_WM_BYPASS_COMPOSITOR is set to 1. Note, however,
        // that this window property is merely a hint to the compositor, if there even is
        // one. Compositors may or may not honor the hint. Some will, by default,
        // automatically manage bypass mode based on their own heuristics.
        //
        // To control exactly when to turn compositor bypass mode on or off, two flags are
        // maintained per window:
        //
        // 1. `wanted_fullscreen_state`: Reflects the application's currently requested
        //    state.
        //
        // 2. `confirmed_fullscreen_state`: Reflects the actual state, judged by observing
        //    the necessarily delayed stream of "property change" events from the X server.
        //
        // The turning on and off of compositor bypass mode is then driven by the
        // conjunction (logical AND) of these two flags:
        //
        // - OFF -> ON: When both flags become true, compositor bypass mode is turned on
        //   (ensures wait for X server to actually enter fullscreen mode).
        //
        // - ON -> OFF: When the conjunction becomes false, compositor bypass mode is turned
        //   off (ensures immediate turn off of bypass mode when leaving fullscreen mode).
        //
        // This scheme generally ensures that compositor bypass mode is in the turned-on
        // state *only* while the window is actually in fullscreen mode. Under normal timing
        // conditions, this is guaranteed. However, in extreme cases (e.g., when fullscreen
        // mode is rapidly and repeatedly toggled), transients can occur where compositor
        // bypass mode is in the turned-on state while the window is not in fullscreen
        // mode. Unfortunately, no reasonable solution exists that completely avoids this.
        //
        ARCHON_ASSERT(fullscreen_bypass_compositor);
        bool want_compositor_bypass = (slot.wanted_fullscreen_state && slot.confirmed_fullscreen_state);
        if (want_compositor_bypass != slot.compositor_bypassed) {
            set_compositor_bypass_mode(slot.window, want_compositor_bypass);
            slot.compositor_bypassed = want_compositor_bypass;
        }
    };

    auto set_fullscreen_mode = [&](WindowSlot& slot, bool on) {
        if (on != slot.wanted_fullscreen_state) {
            slot.wanted_fullscreen_state = on;
            x11::set_fullscreen_mode(dpy, slot.window, on, root, atom_net_wm_state,
                                     atom_net_wm_state_fullscreen); // Throws
            if (fullscreen_bypass_compositor)
                ensure_turn_on_off_compositor_bypass_mode(slot);
        }
    };

    // Map key code to a keyboard independent symbol identifier for the main function of the
    // key (in general the symbol in the upper left corner).
    auto try_get_keysym = [&](KeyCode keycode, KeySym& keysym) noexcept -> bool {
        // Both the shift level and the group selector needs to be zero in order to get the
        // KeySym value for the main function of the key
        int group = 0;
        int level = 0;
        // FIXME: XkbKeycodeToKeysym() does automatically track changes to the keyboard
        // layout, but this tracking is not 100% reliable when multiple layout changes
        // happen in quick succession, or when input seat re-assignment happens (MPX).
        KeySym keysym_2 = XkbKeycodeToKeysym(dpy, keycode, group, level);
        if (ARCHON_LIKELY(keysym_2 != NoSymbol)) {
            keysym = keysym_2;
            return true;
        }
        return false;
    };

    auto get_key_name = [&](KeySym keysym) -> std::string_view {
        // XKeysymToString() returns a string consisting entirely of characters from the X
        // Portable Character Set. Since all locales, that are compatible with Xlib, agree
        // on the encoding of characters in this character set, and since we assume that the
        // selected locale is compatible with Xlib, we can assume that the returned string
        // is valid in the selected locale.
        return XKeysymToString(keysym);
    };

    auto log = [&](int window_no, std::string_view message, const auto&... args) {
        if (max_seen_window_slots < 2) {
            logger.info(message, args...); // Throws
        }
        else {
            logger.info("WINDOW %s: %s", window_no, core::formatted(message, args...)); // Throws
        }
    };

    std::size_t num_windows = 1;
    if (!paths.empty())
        num_windows = paths.size();
    if (optional_num_windows.has_value())
        num_windows = optional_num_windows.value();
    for (std::size_t i = 0; i < num_windows; ++i)
        open_window(); // Throws

    for (auto& entry : window_slots) {
        WindowSlot& slot = entry.second;
        XMapWindow(dpy, slot.window);
        set_fullscreen_monitors(slot.window); // Throws
        if (slot.is_first && fullscreen) {
            bool on = true;
            set_fullscreen_mode(slot, on); // Throws
        }
    }

    if (install_colormap)
        XInstallColormap(dpy, colormap);

    auto on_keydown = [&](KeySym keysym, bool is_repetition, WindowSlot& slot) {
        switch (keysym) {
            case XK_Escape:
            case XK_q: {
                if (!is_repetition)
                    close_window(slot.window);
                break;
            }
        }
    };

    auto on_keyup = [&](KeySym keysym, WindowSlot& slot) {
        switch (keysym) {
            case XK_n: {
                Window window = open_window(); // Throws
                XMapWindow(dpy, window);
                set_fullscreen_monitors(window); // Throws
                break;
            }
            case XK_f: {
                set_fullscreen_mode(slot, !slot.wanted_fullscreen_state); // Throws
                break;
            }
            case XK_g: {
                if (!slot.grabbed) {
                    bool success;
                    if (use_xinput) {
                        success = try_grab_pointer_xi(dpy, input_seat.pointer_device,
                                                      slot.window, report_mouse_motion,
                                                      logger); // Throws
                    }
                    else {
                        success = try_grab_pointer(dpy, slot.window, report_mouse_motion,
                                                   logger); // Throws
                    }
                    if (ARCHON_LIKELY(success)) {
                        slot.grabbed = true;
                        log(slot.no, "GRAB");
                    }
                    else {
                        log(slot.no, "GRAB FAILED");
                    }
                }
                else {
                    slot.grabbed = false;
                    if (use_xinput) {
                        ungrab_pointer_xi(dpy, input_seat.pointer_device);
                    }
                    else {
                        ungrab_pointer(dpy);
                    }
                    log(slot.no, "UNGRAB");
                }
                break;
            }
            case XK_r: {
                slot.suppress_redraw = !slot.suppress_redraw;
                break;
            }
        }
    };

    // X11 timestamps are 32-bit unsigned integers and `Time` refers to the unsigned integer
    // type that X11 uses to store these timestamps.
    using timestamp_unwrapper_type = impl::TimestampUnwrapper<Time, 32>;
    timestamp_unwrapper_type timestamp_unwrapper;

    // Event loop
    bool expect_keymap_notify = false;
    X11KeyCodeSet pressed_keys;
    while (!window_slots.empty()) {
        XEvent ev = {};
        XPeekEvent(dpy, &ev); // Block until at least one event is available
        timestamp_unwrapper_type::Session unwrap_session(timestamp_unwrapper);
        for (;;) {
            int n = XEventsQueued(dpy, QueuedAfterReading); // Non-blocking
            if (n == 0)
                break;
            // If generation of X11 events happens fast enough to saturate processing, `n`
            // could grow without bounds over time. A ceiling is put on `n` in order to
            // avoid this.
            int num_events = std::min(n, 256);
            while (num_events > 0) {
                XNextEvent(dpy, &ev);
                num_events -= 1;
                bool expect_keymap_notify_2 = expect_keymap_notify;
                expect_keymap_notify = false;
                ARCHON_ASSERT(!expect_keymap_notify_2 || ev.type == KeymapNotify);
                WindowSlot* slot = {};
                switch (ev.type) {
                    case GenericEvent:
                        if (use_xinput && ev.xcookie.extension == extension_info.xinput_opcode) {
                            Bool success = XGetEventData(dpy, &ev.xcookie);
                            ARCHON_STEADY_ASSERT(success);
                            ARCHON_SCOPE_EXIT {
                                XFreeEventData(dpy, &ev.xcookie);
                            };
                            switch (ev.xcookie.evtype) {
                                case XI_Motion: {
                                    const XIDeviceEvent& ev_2 = *static_cast<XIDeviceEvent*>(ev.xcookie.data);
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.pointer_device &&
                                                 try_get_window_slot(ev_2.event, slot) &&
                                                 x11_is_genuine_motion_event(ev_2, input_seat.pointer_properties));
                                    if (ARCHON_LIKELY(good)) {
                                        if (slot->grabbed || x11_are_any_xinput_buttons_pressed(ev_2.buttons)) {
                                            math::Vector2F pos = { float(ev_2.event_x), float(ev_2.event_y) };
                                            auto timestamp = unwrap_session.unwrap_next_timestamp(ev_2.time); // Throws
                                            log(slot->no, "XI MOUSE MOVE: %s, %s", pos,
                                                core::as_int(timestamp.count())); // Throws
                                        }
                                    }
                                    break;
                                }
                                case XI_ButtonPress:
                                case XI_ButtonRelease: {
                                    const XIDeviceEvent& ev_2 = *static_cast<XIDeviceEvent*>(ev.xcookie.data);
                                    const auto& button_map = input_seat.pointer_properties.button_map;
                                    int x11_button = {};
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.pointer_device &&
                                                 try_get_window_slot(ev_2.event, slot) &&
                                                 x11_try_normalize_pointer_button(ev_2.detail, button_map,
                                                                                  x11_button));
                                    if (ARCHON_LIKELY(good)) {
                                        bool is_scroll = {};
                                        display::MouseButton button = {};
                                        math::Vector2F amount = {};
                                        bool good_2 = x11_try_map_pointer_button(x11_button, is_scroll,
                                                                                 button, amount);
                                        ARCHON_ASSERT(good_2);
                                        auto timestamp = unwrap_session.unwrap_next_timestamp(ev_2.time); // Throws
                                        if (is_scroll) {
                                            if (ev_2.evtype == XI_ButtonPress)
                                                log(slot->no, "XI SCROLL: %s, %s", amount,
                                                    core::as_int(timestamp.count())); // Throws
                                        }
                                        else {
                                            std::string_view label = (ev_2.evtype == XI_ButtonPress ?
                                                                      "XI MOUSE DOWN" : "XI MOUSE UP");
                                            math::Vector2F pos = { float(ev_2.event_x), float(ev_2.event_y) };
                                            log(slot->no, "%s: %s, %s, %s", label, button, pos,
                                                core::as_int(timestamp.count())); // Throws
                                        }
                                    }
                                    break;
                                }
                                case XI_KeyPress:
                                case XI_KeyRelease: {
                                    const XIDeviceEvent& ev_2 = *static_cast<XIDeviceEvent*>(ev.xcookie.data);
                                    // FIXME: Using XkbKeycodeToKeysym() is fundamentally
                                    // incompatible with XInput2, at least when multiple
                                    // input seats are involved.                                          
                                    KeyCode keycode = KeyCode(ev_2.detail);
                                    KeySym keysym = {};
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.keyboard_device &&
                                                 try_get_window_slot(ev_2.event, slot) &&
                                                 try_get_keysym(keycode, keysym));
                                    if (ARCHON_LIKELY(good)) {
                                        bool is_repetition = ((ev_2.flags & XIKeyRepeat) != 0);
                                        std::string_view label = (ev_2.evtype == XI_KeyPress ?
                                                                  (is_repetition ? "XI KEY REPEAT" :
                                                                   "XI KEY DOWN") : "XI KEY UP");
                                        std::string_view key_name = get_key_name(keysym); // Throws
                                        auto timestamp = unwrap_session.unwrap_next_timestamp(ev_2.time); // Throws
                                        log(slot->no, "%s: %s, %s -> %s, %s", label, key_name, core::as_int(keycode),
                                            core::as_int(keysym), core::as_int(timestamp.count())); // Throws
                                        if (ev_2.evtype == XI_KeyPress) {
                                            on_keydown(keysym, is_repetition, *slot); // Throws
                                        }
                                        else {
                                            on_keyup(keysym, *slot); // Throws
                                        }
                                    }
                                    break;
                                }
                                case XI_Enter: {
                                    const XIEnterEvent& ev_2 = *static_cast<XIEnterEvent*>(ev.xcookie.data);
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.pointer_device &&
                                                 try_get_window_slot(ev_2.event, slot));
                                    if (ARCHON_LIKELY(good)) {
                                        log(slot->no, "XI MOUSE OVER: %s, %s", get_notify_mode_name_xi(ev_2.mode),
                                            get_notify_detail_name_xi(ev_2.detail)); // Throws
                                    }
                                    break;
                                }
                                case XI_Leave: {
                                    const XILeaveEvent& ev_2 = *static_cast<XILeaveEvent*>(ev.xcookie.data);
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.pointer_device &&
                                                 try_get_window_slot(ev_2.event, slot));
                                    if (ARCHON_LIKELY(good)) {
                                        log(slot->no, "XI MOUSE OUT: %s, %s", get_notify_mode_name_xi(ev_2.mode),
                                            get_notify_detail_name_xi(ev_2.detail)); // Throws
                                    }
                                    break;
                                }
                                case XI_FocusIn: {
                                    const XIFocusInEvent& ev_2 = *static_cast<XIFocusInEvent*>(ev.xcookie.data);
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.keyboard_device &&
                                                 try_get_window_slot(ev_2.event, slot));
                                    if (ARCHON_LIKELY(good)) {
                                        log(slot->no, "XI FOCUS: %s, %s", get_notify_mode_name_xi(ev_2.mode),
                                            get_notify_detail_name_xi(ev_2.detail)); // Throws
                                    }
                                    break;
                                }
                                case XI_FocusOut: {
                                    const XIFocusOutEvent& ev_2 = *static_cast<XIFocusOutEvent*>(ev.xcookie.data);
                                    bool good = (!input_seat.removed && ev_2.deviceid == input_seat.keyboard_device &&
                                                 try_get_window_slot(ev_2.event, slot));
                                    if (ARCHON_LIKELY(good)) {
                                        log(slot->no, "XI BLUR: %s, %s", get_notify_mode_name_xi(ev_2.mode),
                                            get_notify_detail_name_xi(ev_2.detail)); // Throws
                                    }
                                    break;
                                }
                                case XI_DeviceChanged: {
                                    const XIDeviceChangedEvent& ev_2 =
                                        *static_cast<XIDeviceChangedEvent*>(ev.xcookie.data);
                                    logger.info("XI DEVICE CHANGE: %s, %s", ev_2.deviceid,
                                                get_device_change_reason_name(ev_2.reason)); // Throws
                                    x11_handle_xinput_device_change(ev_2, input_atoms, input_seat);
                                    break;
                                }
                                case XI_HierarchyChanged: {
                                    const XIHierarchyEvent& ev_2 = *static_cast<XIHierarchyEvent*>(ev.xcookie.data);
                                    auto format_info = [](const XIHierarchyInfo& info) {
                                        return core::as_format_func([&](std::ostream& out) {
                                            auto format_flags = [&](std::ostream& out) {
                                                int flags[] = {
                                                    XIMasterAdded,
                                                    XIMasterRemoved,
                                                    XISlaveAttached,
                                                    XISlaveDetached,
                                                    XISlaveAdded,
                                                    XISlaveRemoved,
                                                    XIDeviceEnabled,
                                                    XIDeviceDisabled,
                                                };
                                                constexpr int n = std::size(flags);
                                                int flags_2[n] = {};
                                                std::size_t i = 0;
                                                for (int flag : flags) {
                                                    if ((info.flags & flag) != 0)
                                                        flags_2[i++] = flag;
                                                }
                                                out << core::as_list(core::Span(flags_2, i), [](int flag) noexcept {
                                                    return get_hierarchy_change_flag_name(flag);
                                                }); // Throws
                                            };
                                            out << core::formatted("(%s: %s)", core::as_int(info.deviceid),
                                                                   core::as_format_func(format_flags)); // Throws
                                        });
                                    };
                                    core::Span infos = { ev_2.info, ev_2.info + ev_2.num_info };
                                    logger.info("XI HIERARCHY CHANGE: %s",
                                                core::as_list(infos, format_info)); // Throws
                                    x11_handle_xinput_hierarchy_change(ev_2, input_seat);
                                    break;
                                }
                            }
                        }
                        break;
                    case MotionNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xmotion.window, slot))) {
                            math::Vector2F pos = { float(ev.xmotion.x), float(ev.xmotion.y) };
                            auto timestamp = unwrap_session.unwrap_next_timestamp(ev.xmotion.time); // Throws
                            log(slot->no, "MOUSE MOVE: %s, %s", pos, core::as_int(timestamp.count())); // Throws
                        }
                        break;
                    case ConfigureNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xconfigure.window, slot))) {
                            // When there is a window manager, the window manager will
                            // generally re-parent the client's window. This generally means
                            // that the client's window will remain at a fixed position
                            // relative to it's parent, so there will be no configure
                            // notifications when the window is moved through user
                            // interaction. Also, if the user's window is moved relative to
                            // its parent, the reported position will be unreliable, as it
                            // will be relative to its parent, which is not the root window
                            // of the screen. Fortunately, in all those cases, the window
                            // manager is obligated to generate synthetic configure
                            // notifications in which the positions are absolute (relative
                            // to the root window of the screen).
                            if (ev.xconfigure.send_event) {
                                log(slot->no, "POS: %s", display::Pos(ev.xconfigure.x, ev.xconfigure.y));
                            }
                            else {
                                log(slot->no, "SIZE: %s", display::Size(ev.xconfigure.width, ev.xconfigure.height));
                            }
                            display::Size size = { ev.xconfigure.width, ev.xconfigure.height };
                            if (size != slot->win_size) {
                                slot->win_size = size;
                                slot->redraw = true;
                            }
                        }
                        break;
                    case Expose:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xexpose.window, slot)))
                            slot->redraw = true;
                        break;
                    case ButtonPress:
                    case ButtonRelease:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xbutton.window, slot))) {
                            bool is_scroll = {};
                            display::MouseButton button = {};
                            math::Vector2F amount = {};
                            if (ARCHON_LIKELY(x11_try_map_pointer_button(ev.xbutton.button, is_scroll, button,
                                                                         amount))) {
                                auto timestamp = unwrap_session.unwrap_next_timestamp(ev.xbutton.time); // Throws
                                if (is_scroll) {
                                    if (ev.type == ButtonPress) {
                                        log(slot->no, "SCROLL: %s, %s", amount,
                                            core::as_int(timestamp.count())); // Throws
                                    }
                                }
                                else {
                                    std::string_view label = (ev.type == ButtonPress ? "MOUSE DOWN" : "MOUSE UP");
                                    math::Vector2F pos = { float(ev.xbutton.x), float(ev.xbutton.y) };
                                    log(slot->no, "%s: %s, %s, %s", label, button, pos,
                                        core::as_int(timestamp.count())); // Throws
                                }
                            }
                        }
                        break;
                    case KeyPress:
                    case KeyRelease:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xkey.window, slot))) {
                            KeyCode keycode = KeyCode(ev.xkey.keycode);
                            auto timestamp = unwrap_session.unwrap_next_timestamp(ev.xkey.time); // Throws
                            bool is_repetition = false;
                            if (ARCHON_LIKELY(detectable_autorepeat_enabled)) {
                                if (ev.type == KeyPress) {
                                    if (!pressed_keys.contains(keycode)) {
                                        pressed_keys.add(keycode);
                                    }
                                    else {
                                        is_repetition = true;
                                    }
                                }
                                else {
                                    ARCHON_ASSERT(pressed_keys.contains(keycode));
                                    pressed_keys.remove(keycode);
                                }
                            }
                            else {
                                // When "detectable auto-repeat" mode was not enabled, we
                                // need to use a fall-back detection mechanism, which works
                                // as follows: On "key up", if the next event is "key down"
                                // for the same key and at almost the same time, consider
                                // the pair to be caused by key repetition. This scheme
                                // assumes that the second "key down" event is immediately
                                // available, i.e., without having to block. This assumption
                                // appears to hold in practice, but it could conceivably
                                // fail, in which case the pair will be treated as genuine
                                // "key up" and "key down" events.
                                if (ev.type == KeyPress) {
                                    ARCHON_ASSERT(!pressed_keys.contains(keycode));
                                    pressed_keys.add(keycode);
                                }
                                else {
                                    ARCHON_ASSERT(pressed_keys.contains(keycode));
                                    if (num_events == 0) {
                                        int n = XEventsQueued(dpy, QueuedAfterReading); // Non-blocking
                                        if (n > 0)
                                            num_events = 1;
                                    }
                                    if (num_events > 0) {
                                        XEvent ev_2 = {};
                                        XPeekEvent(dpy, &ev_2);
                                        if (ev_2.type == KeyPress && KeyCode(ev_2.xkey.keycode) == keycode) {
                                            ARCHON_ASSERT(ev_2.xkey.window == ev.xkey.window);
                                            auto timestamp_2 =
                                                unwrap_session.unwrap_next_timestamp(ev_2.xkey.time); // Throws
                                            ARCHON_ASSERT(timestamp_2 >= timestamp);
                                            if ((timestamp_2 - timestamp).count() <= 1) {
                                                XNextEvent(dpy, &ev);
                                                timestamp = timestamp_2;
                                                --num_events;
                                                is_repetition = true;
                                            }
                                        }
                                    }
                                    if (!is_repetition)
                                        pressed_keys.remove(keycode);
                                }
                            }
                            KeySym keysym = {};
                            if (ARCHON_LIKELY(slot->has_input_focus && try_get_keysym(keycode, keysym))) {
                                std::string_view label = (ev.type == KeyPress ? (is_repetition ? "KEY REPEAT" :
                                                                                 "KEY DOWN") : "KEY UP");
                                std::string_view key_name = get_key_name(keysym); // Throws
                                log(slot->no, "%s: %s, %s -> %s, %s", label, key_name, core::as_int(ev.xkey.keycode),
                                    core::as_int(keysym), core::as_int(timestamp.count())); // Throws
                                if (ev.type == KeyPress) {
                                    on_keydown(keysym, is_repetition, *slot); // Throws
                                }
                                else {
                                    on_keyup(keysym, *slot); // Throws
                                }
                            }
                        }
                        break;
                    case KeymapNotify:
                        // Note: For some unclear reason, `ev.xkeymap.window` does not
                        // specify the target window like it does for other types of
                        // events. Instead, one can rely on `KeymapNotify` to be generated
                        // immediately after every `FocusIn` event, so this provides an
                        // implicit target window.
                        if (expect_keymap_notify_2)
                            pressed_keys.assign(ev.xkeymap.key_vector);
                        break;
                    case EnterNotify:
                    case LeaveNotify:
                        // Not clear why X server sends EnterNotify during mouse drags when
                        // EnterNotify has not been asked for in the event mask. Seems to be
                        // a side effect of also enabling certain XInput2 events. Possibly a
                        // bug.
                        if (!use_xinput) {
                            if (ARCHON_LIKELY(try_get_window_slot(ev.xcrossing.window, slot))) {
                                log(slot->no, "%s: %s, %s", (ev.type == EnterNotify ? "MOUSE OVER" : "MOUSE OUT"),
                                    get_notify_mode_name(ev.xcrossing.mode),
                                    get_notify_detail_name(ev.xcrossing.detail)); // Throws
                            }
                        }
                        break;
                    case FocusIn:
                    case FocusOut:
                        if (ev.type == FocusIn)
                            expect_keymap_notify = true;
                        //
                        // When regular input focus is gained or lost, it is reported to the
                        // window using a focus event with mode=NotifyNormal or
                        // mode=NotifyWhileGrabbed. The latter is used if the change occurs
                        // while the keyboard is grabbed.
                        //
                        // Events with mode=NotifyGrab or mode=NotifyUngrab are filtered
                        // out. These report the initiation or termination of a keyboard
                        // grab, which does not indicate a gain or loss of regular input
                        // focus for the window.
                        //
                        // In this context, a window is understood as having *regular input
                        // focus* if and only if the X server's global input focus is
                        // explicitly set to that window. The X server's global input focus
                        // is set by XSetInputFocus(), and then only changes if
                        // XSetInputFocus() is called again or the focus window becomes
                        // unviewable.
                        //
                        // Note that the regular input focus is not identical to the
                        // *effective input focus*. The latter determines where key events
                        // are actually sent at any specific time. It depends on the regular
                        // input focus, but also on keyboard grabs and, when the server is
                        // in PointerRoot mode, the position of the pointer.
                        //
                        // Events with detail=NotifyPointer are also filtered out. These are
                        // generated for a window, W, when an ancestor window gains or loses
                        // input focus (including switches to or from PointerRoot), and the
                        // pointer happens to be located within W. In none of these cases
                        // does it indicate a gain or loss of regular input focus for W.
                        //
                        if (ARCHON_LIKELY((ev.xfocus.mode == NotifyNormal || ev.xfocus.mode == NotifyWhileGrabbed) &&
                                          ev.xfocus.detail != NotifyPointer)) {
                            if (ARCHON_LIKELY(try_get_window_slot(ev.xfocus.window, slot))) {
                                slot->has_input_focus = (ev.type == FocusIn);
                                log(slot->no, "%s", (ev.type == FocusIn ? "FOCUS" : "BLUR")); // Throws
                            }
                        }
                        break;
                    case ClientMessage:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xclient.window, slot))) {
                            bool is_close = (ev.xclient.format == 32 &&
                                             Atom(ev.xclient.data.l[0]) == atom_wm_delete_window);
                            if (is_close)
                                close_window(slot->window);
                        }
                        break;
                    case VisibilityNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xvisibility.window, slot))) {
                            if (set_input_focus && !slot->input_focus_set) {
                                XSetInputFocus(dpy, slot->window, RevertToPointerRoot, CurrentTime);
                                slot->input_focus_set = true;
                            }
                        }
                        break;
                    case PropertyNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xproperty.window, slot))) {
                            if (fullscreen_bypass_compositor) {
                                bool found = {};
                                bool good = (ev.xproperty.atom == atom_net_wm_state &&
                                             ev.xproperty.state == PropertyNewValue &&
                                             x11::try_property_find_a(dpy, slot->window, atom_net_wm_state,
                                                                      atom_net_wm_state_fullscreen, found)); // Throws
                                if (good) {
                                    slot->confirmed_fullscreen_state = found;
                                    ensure_turn_on_off_compositor_bypass_mode(*slot);
                                }
                            }
                        }
                        break;
                }
#if ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR
                if (extension_info.have_xrandr && ev.type == extension_info.xrandr_event_base + RRNotify) {
                    const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
                    switch (ev_2.subtype) {
                        case RRNotify_CrtcChange:
                        case RRNotify_OutputChange:
                            if (update_screen_conf()) // Throws
                                dump_screen_conf(); // Throws
                    }
                }
#endif // ARCHON_DISPLAY_HAVE_GOOD_X11_XRANDR
            }
        }

        for (const auto& entry : window_slots) {
            const WindowSlot& slot = entry.second;
            if (slot.redraw && !slot.suppress_redraw) {
                int win_width  = slot.win_size.width;
                int win_height = slot.win_size.height;
                int left = 0, right = win_width;
                int top = 0, bottom = win_height;
                int x = 0, y = 0;
                int w = slot.img_size.width, h = slot.img_size.height;
                {
                    int width_diff  = win_width  - slot.img_size.width;
                    int height_diff = win_height - slot.img_size.height;
                    if (width_diff >= 0) {
                        left  = width_diff / 2;
                        right = left + slot.img_size.width;
                    }
                    else {
                        x = (-width_diff + 1) / 2;
                        w = win_width;
                    }
                    if (height_diff >= 0) {
                        top    = height_diff / 2;
                        bottom = top + slot.img_size.height;
                    }
                    else {
                        y = (-height_diff + 1) / 2;
                        h = win_height;
                    }
                }
                Drawable drawable = slot.drawable;
                // Clear top area
                if (top > 0)
                    XFillRectangle(dpy, drawable, gc, 0, 0, unsigned(win_width), unsigned(top));
                // Clear left area
                if (left > 0)
                    XFillRectangle(dpy, drawable, gc, 0, top, unsigned(left), unsigned(h));
                // Copy image
                XCopyArea(dpy, slot.pixmap, drawable, gc, x, y, w, h, left, top);
                // Clear right area
                if (right < win_width)
                    XFillRectangle(dpy, drawable, gc, right, top, unsigned(win_width - right), unsigned(h));
                // Clear bottom area
                if (bottom < win_height)
                    XFillRectangle(dpy, drawable, gc, 0, bottom, unsigned(win_width), unsigned(win_height - bottom));

#if ARCHON_DISPLAY_HAVE_GOOD_X11_XDBE
                if (use_double_buffering) {
                    XdbeSwapInfo info;
                    info.swap_window = slot.window;
                    info.swap_action = swap_action;
                    Status status = XdbeSwapBuffers(dpy, &info, 1);
                    ARCHON_STEADY_ASSERT(status != 0);
                }
#endif // ARCHON_DISPLAY_HAVE_GOOD_X11_XDBE
            }
        }
    }
}


#else // !ARCHON_DISPLAY_HAVE_GOOD_X11


int main()
{
    throw std::runtime_error("No Xlib support");
}


#endif // !ARCHON_DISPLAY_HAVE_GOOD_X11
