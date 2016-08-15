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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <GL/gl.h>

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>
#include <archon/thread/thread.hpp>
#include <archon/render/conductor.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::thread;
using namespace archon::display;
using namespace archon::render;


namespace
{
  struct ViewImpl: View
  {
    Viewport::Ref newViewport() const;
    Screen::Ref   newScreen()   const;
    Eye::Ref      newEye()      const;
    Clip::Ref     newClip()     const;

    ViewImpl(Renderer::RefArg r): renderer(r) {}

    Renderer::Ref const renderer;
    Mutex viewMutex;
  };



  struct ViewportImpl: Viewport
  {
    void set(double left, double bottom,
             double width, double height)
    {
      Mutex::Lock l(view->viewMutex);
      this->left   = left;
      this->bottom = bottom;
      this->width  = width;
      this->height = height;
    }

    ViewportImpl(ViewImpl const *view):
      view(view), left(0), bottom(0), width(1), height(1) {}

    CntRef<ViewImpl const> const view;
    double left, bottom;
    double width, height;
  };



  struct ScreenImpl: Screen
  {
    void set(Vec3 const &center, Vec3 const &x,
             Vec3 const &y, double halfWidth, double halfHeight)
    {
      Mutex::Lock l(view->viewMutex);
      this->center     = center;
      this->x          = x;
      this->y          = y;
      this->halfWidth  = halfWidth;
      this->halfHeight = halfHeight;
    }

    void set(double aspectRatio,
             double fieldOfView,
             double distance)
    {
      double w = distance * tan(fieldOfView/2);
      double h = w;
      if(aspectRatio > 1) w *= aspectRatio;
      else h /= aspectRatio;

      set(Vec3(0, 0, -distance),
          Vec3(1, 0, 0), Vec3(0, 1, 0), w, h);
    }

    ScreenImpl(ViewImpl const *view): view(view)
    {
      set(1, M_PI/4, 1);
    }

    CntRef<ViewImpl const> const view;
    Vec3 center;
    Vec3 x, y;
    double halfWidth, halfHeight;
  };



  struct EyeImpl: Eye
  {
    void set(Vec3 position)
    {
      Mutex::Lock l(view->viewMutex);
      this->position = position;
    }

    EyeImpl(ViewImpl const *view):
      view(view), position(Vec3(0, 0, 0))
    {
    }

    CntRef<ViewImpl const> const view;
    Vec3 position;
  };



  struct ClipImpl: Clip
  {
    void set(double near, double far)
    {
      Mutex::Lock l(view->viewMutex);
      this->near = near;
      this->far  = far;
    }

    ClipImpl(ViewImpl const *view):
      view(view), near(0.2), far(200)
    {
    }

    CntRef<ViewImpl const> const view;
    double near, far;
  };



  Viewport::Ref ViewImpl::newViewport() const
  {
    return Viewport::Ref(new ViewportImpl(this));
  }

  Screen::Ref ViewImpl::newScreen() const
  {
    return Screen::Ref(new ScreenImpl(this));
  }

  Eye::Ref ViewImpl::newEye() const
  {
    return Eye::Ref(new EyeImpl(this));
  }

  Clip::Ref ViewImpl::newClip() const
  {
    return Clip::Ref(new ClipImpl(this));
  }



  struct Channel
  {
    void render() const
    {
      double l, b, w, h;
      Vec3 c, x, y;
      double s, t;
      Vec3 e;
      double n, f;

      // Fetch the view information
      {
        Mutex::Lock lock(view->viewMutex);
        l = viewport->left;
        b = viewport->bottom;
        w = viewport->width;
        h = viewport->height;
        c = screen->center;
        x = screen->x;
        y = screen->y;
        s = screen->halfWidth;
        t = screen->halfHeight;
        e = eye->position;
        n = clip->near;
        f = clip->far;
      }

      // Determine the canonical coordinate system of the eye
      Vec3 z = x * y;

      // (x, y, z) is now an orthonormal basis for the coordinate system of the screen with respect to the coordinate system of the physical viewer (the reference frame)

      // Determine the center of the screen in the canonical coordinate
      // system of the eye
      c -= e; // Get vector from eye to center of screen in reference frame
      c.set(dot(c,x), dot(c,y), dot(c,z));

      // Setup the projection matrix
      double a = n / -c[2];
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum((c[0] - s) * a, (c[0] + s) * a,
                (c[1] - t) * a, (c[1] + t) * a, n, f);

/*
      cerr << "w:\t"
        << ((c[0] - s) * a) << ",\t"
        << ((c[0] + s) * a) << ",\t"
        << ((c[1] - t) * a) << ",\t"
        << ((c[1] + t) * a) << ",\t"
        << n << ",\t" << f << "\n";
*/

      // Setup the modelview matrix
      GLdouble m[16] =
        {
          x[0],       y[0],       z[0],       0,
          x[1],       y[1],       z[1],       0,
          x[2],       y[2],       z[2],       0,
          -dot(x, e), -dot(y, e), -dot(z, e), 1
        };
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixd(m);

/*
      cerr
        << m[0] << "\t" << m[4] << "\t" << m[ 8] << "\t" << m[12] << "\n"
        << m[1] << "\t" << m[5] << "\t" << m[ 9] << "\t" << m[13] << "\n"
        << m[2] << "\t" << m[6] << "\t" << m[10] << "\t" << m[14] << "\n"
        << m[3] << "\t" << m[7] << "\t" << m[11] << "\t" << m[15] << "\n";
*/

      // Setup the viewport (with strict round-off controll)
      s = window->getWidth();
      t = window->getHeight();
      unsigned il = static_cast<unsigned>(l * s + 0.5);
      unsigned ib = static_cast<unsigned>(b * t + 0.5);
      unsigned iw = static_cast<unsigned>((l+w) * s + 0.5) - il;
      unsigned ih = static_cast<unsigned>((b+h) * t + 0.5) - ib;
      glViewport(il, ib, iw, ih);

      view->renderer->render();
    }

    Channel(ViewImpl const *view, Window::RefArg window,
            ViewportImpl const *viewport, ScreenImpl const *screen,
            EyeImpl const *eye, ClipImpl const *clip):
      view(view), window(window), viewport(viewport),
      screen(screen), eye(eye), clip(clip)
    {
    }

    CntRef<ViewImpl     const> view;
    Window::Ref                window;
    CntRef<ViewportImpl const> viewport;
    CntRef<ScreenImpl   const> screen;
    CntRef<EyeImpl      const> eye;
    CntRef<ClipImpl     const> clip;
  };



  struct Context: CntRefObjectBase, CntRefDefs<Context>
  {
    void addChannel(Channel const &channel)
    {
      Mutex::Lock l(channelMutex);
      channels.push_back(channel);
    }

    void render() const
    {
      Context::ConstRef context(this);
      Mutex::Lock l(channelMutex);
      for(vector<Channel>::const_iterator i = channels.begin(),
            j = channels.end(); i != j; ++i)
      {
        Bind bind(context->context, i->window);
        if(!initialized)
        {
          i->view->renderer->initOpenGlContext();
          initialized = true;
        }
	i->render();
      }
    }

    Context(Visual::ConstRefArg visual, bool direct, Context::RefArg master):
      context(master ? visual->newContext(direct, master->context) :
              visual->newContext(direct)), initialized(false)
    {
    }

    archon::display::Context::Ref context;

    Mutex channelMutex;
    vector<Channel> channels;

    /**
     * Has the user's initialization of this context been called.
     */
    mutable bool initialized;
  };



  struct ConductorImpl: Conductor
  {
    void add_pipeline(Visual::ConstRefArg visual, bool direct)
    {
      UniquePtr<Pipeline> pipe(new Pipeline(visual, direct, master_pipe));
      Thread::Ref thread;
      {
        Mutex::Lock l(syncMutex);
        pipelines.append(pipe);
        if(master_pipeline)
        {
          thread = Thread::run<ConductorImpl, size_t>(this, &ConductorImpl::slave_thread,
                                                      slaves.size(), false);
          slaves.push_back(Slave(pipelines[0], thread));
        }
        else master_pipeline = pipelines[0];
      }
      if(thread) Thread::start(thread);
    }

    void render()
    {
      // Clear all windows
      for(Windows::const_iterator i = windows.begin(), j = windows.end(); i != j; ++i)
      {
	Bind b(i->second->context, i->first);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }

      // Proceed slave threads
      {
	Mutex::Lock l(sync_mutex);
        for(vector<Slave>::iterator i = slaves.begin(),
              j = slaves.end(); i != j; ++i) i->hold = false;
	unfinished_slaves = slaves.size();
      }
      proceed.notify_all();

      // Render
      master_context->render();

      // Sync with slave threads
      {
	Mutex::Lock l(sync_mutex);
	while(unfinished_slaves) slave_finished.wait();
      }

      // Swap all buffers
      for(Windows::const_iterator i = windows.begin(),
            j = windows.end(); i != j; ++i) i->first->swap_buffers();
    }


    ConductorImpl():
      unfinished_slaves(0), proceed(sync_mutex), slave_finished(sync_mutex) {}


    ~ConductorImpl()
    {
      terminate_threads();
    }

    // We cannot allow destructor to throw, since the threads might
    // then attempt to access destroyed data.
    void terminate_threads() throw()
    {
      Mutex::Lock l(sync_mutex);
      for(vector<Slave>::const_iterator i = slaves.begin(),
            j = slaves.end(); i != j; ++i) i->thread->terminate();
      for(vector<Slave>::const_iterator i = slaves.begin(),
            j = slaves.end(); i != j; ++i) i->thread->wait(true);
    }


    /**
     * Protects windows, masterPipe, slaves, Slave::hold,
     * unfinishedSlaves, proceed and slaveFinished.
     */
    Mutex syncMutex;

    void addWindow(Window::RefArg d, Context::RefArg c)
    {
      Mutex::Lock l(syncMutex);
      for(Windows::iterator i=windows.begin(), j=windows.end(); i!=j; ++i)
	if(i->first == d) return;
      windows.push_back(make_pair(d, c));
    }

    void slave_thread(size_t index)
    {
      Context::Ref context;
      {
	Mutex::Lock l(syncMutex);
	context = slaves[index].context;
      }

      for(;;)
      {
	{
	  Mutex::Lock l(syncMutex);
	  while(slaves[index].hold) proceed.wait();
	  slaves[index].hold = true;
	}

	context->render();

	{
	  Mutex::Lock l(syncMutex);
	  --unfinishedSlaves;
	}
	slaveFinished.notifyAll();
      }
    }

    struct Slave
    {
      Pipeline *pipe;
      Thread::Ref thread;
      bool hold;

      Slave(Pipeline *pipe, Thread::RefArg thread):
        pipe(pipe), thread(thread), hold(true) {}
    };

    typedef vector<pair<Window *, Pipeline *> > Windows;
    Windows windows;

    vector<Slave> slaves;
    unsigned unfinishedSlaves;
    Condition proceed, slaveFinished;
    DeletingContainer<Pipeline> pipelines;
  };



  struct PipeImpl: Pipe
  {
    /**
     * \todo Also check that the visual of the window equals the
     * visual of the context.
     */
    void add_channel(Window *window,
                     Viewport::ConstRefArg viewport, Screen::ConstRefArg screen,
                     Eye::ConstRefArg eye, Clip::ConstRefArg clip)
    {
      ViewportImpl const *_viewport =
        dynamic_cast<ViewportImpl const *>(viewport.get());
      ScreenImpl const *_screen =
        dynamic_cast<ScreenImpl const *>(screen.get());
      EyeImpl const *_eye =
        dynamic_cast<EyeImpl const *>(eye.get());
      ClipImpl const *_clip =
        dynamic_cast<ClipImpl const *>(clip.get());

      if(!_viewport || !_screen || !_eye || !_clip)
	throw invalid_argument("Found channel component of foreign implementation");

      ViewImpl const *_view = _viewport->view.get();
      if(_view !=   _screen->view.get() ||
	 _view !=      _eye->view.get() ||
	 _view !=     _clip->view.get())
	throw invalid_argument("All channel objects are not associated with the same view");

      conductor->addWindow(window, context);
      context->addChannel(Channel(_view, window, _viewport, _screen, _eye, _clip));
    }



    PipeImpl(ConductorImpl *conductor, Context::Ref context):
      conductor(conductor), context(context)
    {
    }

    CntRef<ConductorImpl> const conductor;
    Context::Ref const context;
  };



  struct ConductorHandle: Conductor
  {
    void render() { cond->render(); }

    ConductorHandle()

    ~ConductorHandle() { cond->terminate_treads(); }

    ConductorImpl const cond;
  };
}


namespace archon
{
  namespace render
  {
    View::Ref View::newView(Renderer::RefArg r)
    {
      return View::Ref(new ViewImpl(r));
    }

    Conductor::Ref Conductor::create()
    {
      return Conductor::Ref(new ConductorImpl);
    }
  }
}
