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
 *
 * Testing the rendering framework.
 */

#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>
#include <string>
#include <iostream>

#include <archon/core/options.hpp>
#include <archon/core/text.hpp>
#include <archon/core/time.hpp>
#include <archon/thread/thread.hpp>
#include <archon/util/ticker.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/integer_buffer_format.hpp>
#include <archon/display/implementation.hpp>
#include <archon/render/conductor.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::thread;
using namespace archon::math;
using namespace archon::image;
using namespace archon::Render;
namespace Display = archon::Display;


namespace
{
  bool   opt_help            = false;
  int    opt_viewMode        = 2;
  double opt_frameRate       = 30;
  double opt_windowSize      = 1;
  double opt_detailLevel     = 1;
  bool   opt_directRendering = true;

  unsigned adjust(unsigned val, unsigned min, double f)
  {
    return std::max(static_cast<unsigned>(f*val), min);
  }

  unsigned adjustDetail(unsigned val, unsigned min)
  {
    return adjust(val, min, opt_detailLevel);
  }

  Conductor::Ref oneThreadMono(Display::Connection::Ptr conn, Renderer::Ref renderer)
  {
    Display::Window::Ptr window = conn->new_window(adjust(1000, 10, opt_win_size),
                                                   adjust(1000, 10, opt_win_size));
    win->set_title("One thread monoscopic view");
    win->show();
    win->set_position(100, 100);

    Conductor::Ref cond = Conductor::create();
    View::Ref view = View::newView(renderer);

    Viewport::Ref viewport = view->newViewport();
    Screen::Ref     screen = view->newScreen();
    Eye::Ref           eye = view->newEye();
    Clip::Ref         clip = view->newClip();

    screen->set(Vec3(0, 0, -2),
                Vec3(1, 0, 0), Vec3(0, 1, 0), 1, 1);

    Pipe::Ref pipe = conductor->addPipe(visual);
    pipe->addChannel(window, viewport, screen, eye, clip);
    return conductor;
  }


  Conductor::Ref twoThreadMono(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref window = visual->newWindow(100, 100,
                                                    adjust(1000, 10, opt_windowSize),
                                                    adjust(1000, 10, opt_windowSize),
                                                    "Two thread monoscopic view");

    View::Ref view = View::newView(renderer);

    Viewport::Ref  leftViewport = view->newViewport();
    Viewport::Ref rightViewport = view->newViewport();
    Screen::Ref      leftScreen = view->newScreen();
    Screen::Ref     rightScreen = view->newScreen();
    Eye::Ref                eye = view->newEye();
    Clip::Ref              clip = view->newClip();

    leftViewport->set(0, 0, 0.5, 1);
    rightViewport->set(0.5, 0, 0.5, 1);

    leftScreen->set(Vec3(-0.5, 0, -2),
                    Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 1);
    rightScreen->set(Vec3(0.5, 0, -2),
                     Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 1);

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref  leftPipe = conductor->addPipe(visual);
    Pipe::Ref rightPipe = conductor->addPipe(visual);

    leftPipe->addChannel(window, leftViewport, leftScreen, eye, clip);
    rightPipe->addChannel(window, rightViewport, rightScreen, eye, clip);

    return conductor;
  }


  Conductor::Ref oneThreadPaperStereo(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref window = visual->newWindow(100, 100,
                                                    adjust(1500, 10, opt_windowSize),
                                                    adjust(750, 10, opt_windowSize),
                                                    "One thread paper stereo: Use a piece of paper");

    View::Ref view = View::newView(renderer);

    Viewport::Ref  leftViewport = view->newViewport();
    Viewport::Ref rightViewport = view->newViewport();
    Screen::Ref      leftScreen = view->newScreen();
    Screen::Ref     rightScreen = view->newScreen();
    Eye::Ref            leftEye = view->newEye();
    Eye::Ref           rightEye = view->newEye();
    Clip::Ref              clip = view->newClip();

    leftViewport->set(0, 0, 0.5, 1);
    rightViewport->set(0.5, 0, 0.5, 1);

    leftEye->set(Vec3(-1.5/7, 0, 0));
    rightEye->set(Vec3(+1.5/7, 0, 0));

    leftScreen->set(Vec3(-0.5, 0, -2),
                    Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 0.5);
    rightScreen->set(Vec3(0.5, 0, -2),
                     Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 0.5);

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref pipe = conductor->addPipe(visual);

    pipe->addChannel(window, leftViewport, leftScreen, leftEye, clip);
    pipe->addChannel(window, rightViewport, rightScreen, rightEye, clip);

    return conductor;
  }

  Conductor::Ref twoThreadPaperStereo(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref window = visual->newWindow(100, 100,
                                                    adjust(1500, 10, opt_windowSize),
                                                    adjust(750, 10, opt_windowSize),
                                                    "Two thread paper stereo: Use a piece of paper");

    View::Ref view = View::newView(renderer);

    Viewport::Ref  leftViewport = view->newViewport();
    Viewport::Ref rightViewport = view->newViewport();
    Screen::Ref      leftScreen = view->newScreen();
    Screen::Ref     rightScreen = view->newScreen();
    Eye::Ref            leftEye = view->newEye();
    Eye::Ref           rightEye = view->newEye();
    Clip::Ref              clip = view->newClip();

    leftViewport->set(0, 0, 0.5, 1);
    rightViewport->set(0.5, 0, 0.5, 1);

    leftEye->set(Vec3(-1.5/7, 0, 0));
    rightEye->set(Vec3(+1.5/7, 0, 0));

    leftScreen->set(Vec3(-0.5, 0, -2),
                    Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 0.5);
    rightScreen->set(Vec3(0.5, 0, -2),
                     Vec3(1, 0, 0), Vec3(0, 1, 0), 0.5, 0.5);

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref  leftPipe = conductor->addPipe(visual);
    Pipe::Ref rightPipe = conductor->addPipe(visual);

    leftPipe->addChannel(window, leftViewport, leftScreen, leftEye, clip);
    rightPipe->addChannel(window, rightViewport, rightScreen, rightEye, clip);

    return conductor;
  }

  Conductor::Ref splitScreenStereo(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref window = visual->newWindow(100, 100,
                                                    adjust(1000, 10, opt_windowSize),
                                                    adjust(1000, 10, opt_windowSize),
                                                    "Split screen stereoscopic view");

    View::Ref view = View::newView(renderer);

    Viewport::Ref  leftViewport = view->newViewport();
    Viewport::Ref rightViewport = view->newViewport();
    Screen::Ref          screen = view->newScreen();
    Eye::Ref            leftEye = view->newEye();
    Eye::Ref           rightEye = view->newEye();
    Clip::Ref              clip = view->newClip();

    leftViewport->set(0, 0, 0.5, 1);
    rightViewport->set(0.5, 0, 0.5, 1);

    leftEye->set(Vec3(-1.0/20, 0, 0));
    rightEye->set(Vec3(+1.0/20, 0, 0));

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref  leftPipe = conductor->addPipe(visual);
    Pipe::Ref rightPipe = conductor->addPipe(visual);

    leftPipe->addChannel(window, leftViewport, screen, leftEye, clip);
    rightPipe->addChannel(window, rightViewport, screen, rightEye, clip);

    return conductor;
  }

  Conductor::Ref dualWindowStereo(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref leftWindow = visual->newWindow(100, 100,
                                                        adjust(500, 10, opt_windowSize),
                                                        adjust(500, 10, opt_windowSize),
                                                        "Left eye of stereoscopic view");
    Display::Window::Ref rightWindow = visual->newWindow(100, 100,
                                                         adjust(500, 10, opt_windowSize),
                                                         adjust(500, 10, opt_windowSize),
                                                         "Right eye of stereoscopic view");

    View::Ref view = View::newView(renderer);

    Viewport::Ref viewport = view->newViewport();
    Screen::Ref     screen = view->newScreen();
    Eye::Ref       leftEye = view->newEye();
    Eye::Ref      rightEye = view->newEye();
    Clip::Ref         clip = view->newClip();

    leftEye->set(Vec3(-1.0/5, 0, 0));
    rightEye->set(Vec3(+1.0/5, 0, 0));

    screen->set(1, M_PI/4, 10);

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref  leftPipe = conductor->addPipe(visual);
    Pipe::Ref rightPipe = conductor->addPipe(visual);

    leftPipe->addChannel(leftWindow, viewport, screen, leftEye, clip);
    rightPipe->addChannel(rightWindow, viewport, screen, rightEye, clip);

    return conductor;
  }

  Conductor::Ref angledScreens(Display::Visual::ConstRef visual, Renderer::Ref renderer)
  {
    Display::Window::Ref window = visual->newWindow(100, 100,
                                                    adjust(1500, 10, opt_windowSize),
                                                    adjust(750, 10, opt_windowSize),
                                                    "Angled screens");

    View::Ref view = View::newView(renderer);

    Viewport::Ref  leftViewport = view->newViewport();
    Viewport::Ref rightViewport = view->newViewport();
    Screen::Ref     frontScreen = view->newScreen();
    Screen::Ref      sideScreen = view->newScreen();
    Eye::Ref                eye = view->newEye();
    Clip::Ref              clip = view->newClip();

    leftViewport->set(0, 0, 0.5, 1);
    rightViewport->set(0.5, 0, 0.5, 1);

    frontScreen->set(Vec3(0, 0, -2),
                     Vec3(1, 0, 0), Vec3(0, 1, 0), 2, 2);
/*
    sideScreen->set(Vec3(2, 0, 0),
                    Vec3(0, 0, 1), Vec3(0, 1, 0), 2, 2);
*/
    sideScreen->set(Vec3(4, 0, -2),
                    Vec3(1, 0, 0), Vec3(0, 1, 0), 2, 2);

    Conductor::Ref conductor = Conductor::create();

    Pipe::Ref pipe = conductor->addPipe(visual);

    pipe->addChannel(window,  leftViewport, frontScreen, eye, clip);
    pipe->addChannel(window, rightViewport,  sideScreen, eye, clip);

    return conductor;
  }


  struct MyRenderer: Renderer
  {
    MyRenderer(): w(0) {}

    double w;

    void initOpenGlContext()
    {
      GLfloat v[4];
      v[3] = 1;

      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_COLOR_MATERIAL);

      v[0] = v[1] = v[2] = 0.2;
      glLightfv(GL_LIGHT0, GL_AMBIENT, v);

      v[0] = v[1] = v[2] = 0.9;
      glLightfv(GL_LIGHT0, GL_DIFFUSE, v);

      v[0] = v[1] = v[2] = 0.8;
      glLightfv(GL_LIGHT0, GL_SPECULAR, v);

      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

      v[0] = 0.9; v[1] = 0.9; v[2] = 0.9;
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, v);

      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32);

      glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    }

    void render()
    {
/*
      glDisable(GL_LIGHTING);
      glBegin(GL_QUADS);

      glColor3f(1, 0, 0);
      glVertex3f(-0.9, 0.4, -2);
      glVertex3f(-0.9, 0.3, -2);
      glVertex3f(-0.8, 0.3, -2);
      glVertex3f(-0.8, 0.4, -2);

      glColor3f(0, 1, 0);
      glVertex3f(-0.2, 0.4, -2);
      glVertex3f(-0.2, 0.3, -2);
      glVertex3f(-0.1, 0.3, -2);
      glVertex3f(-0.1, 0.4, -2);

      glColor3f(0, 0, 1);
      glVertex3f(-0.9, -0.3, -2);
      glVertex3f(-0.9, -0.4, -2);
      glVertex3f(-0.8, -0.4, -2);
      glVertex3f(-0.8, -0.3, -2);

      glColor3f(1, 1, 0);
      glVertex3f(-0.2, -0.3, -2);
      glVertex3f(-0.2, -0.4, -2);
      glVertex3f(-0.1, -0.4, -2);
      glVertex3f(-0.1, -0.3, -2);

      glEnd();
      glEnable(GL_LIGHTING);


      glTranslated(6, 0, -6);
//      glRotated(10, 1, 0, 0);

      GLfloat v[4];
      v[3] = 1;

      v[0] = 5; v[1] = 20; v[2] = 20;
      glLightfv(GL_LIGHT0, GL_POSITION, v);

      //glRotated(w/M_PI*180, 0, 1, 0);

      Image::Ref image1 = Image::load("/home/kristian/persist/devel/local/archon/src/archon/image/test/alley_baggett_8bit_rgb.png");

      int bitsPerByte = numeric_limits<unsigned char>::digits;
      vector<IntegerBufferFormat::Channel> channels;
      channels.push_back(IntegerBufferFormat::Channel(0*bitsPerByte, bitsPerByte));
      channels.push_back(IntegerBufferFormat::Channel(1*bitsPerByte, bitsPerByte));
      channels.push_back(IntegerBufferFormat::Channel(2*bitsPerByte, bitsPerByte));
      channels.push_back(IntegerBufferFormat::Channel(3*bitsPerByte, bitsPerByte));
      IntegerBufferFormat::ConstRef bufferFormat =
        IntegerBufferFormat::getFormat(wordType_UChar,
                                       4*bitsPerByte, // bitsPerPixel
                                       channels);
      int width = 512, height = 512;
      BufferedImage::Ref image2 =
        BufferedImage::newImage(width, height, ColorSpace::getRGB(), true, bufferFormat);
      image2->putImage(image1, 0, 0, false);
      image2->save("/tmp/img.png");

      glEnable(GL_TEXTURE_2D);
      GLuint textureId;
      glGenTextures(1, &textureId);
      glBindTexture(GL_TEXTURE_2D, textureId);   // 2d texture (x and y size)
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
      glTexImage2D(GL_TEXTURE_2D,    // GLenum target,
                   0,                // GLint level,
                   GL_RGBA,          // GLint internalFormat,
                   width,
                   height,
                   0,                // GLint border,
                   GL_RGBA,          // GLenum format,
                   GL_UNSIGNED_BYTE, // GLenum type,
                   image2->getBufferPtr());

      GLUquadric *quadric = gluNewQuadric();
      gluQuadricTexture(quadric, GL_TRUE);
      gluSphere(quadric,
                sqrt(18),
                100,
                100);

      glPopMatrix();

      gluDeleteQuadric(quadric);
*/

      glTranslated(0, 0, -6);
      glRotated(10, 1, 0, 0);

      GLfloat v[4];
      v[3] = 1;

      v[0] = 5; v[1] = 20; v[2] = -5;
      glLightfv(GL_LIGHT0, GL_POSITION, v);

      glRotated(w/M_PI*180, 0, 1, 0);

      GLUquadric *quadric = gluNewQuadric();

      glPushMatrix();
      glColor3f(0.1, 0.9, 0.9);
      glTranslated(0, 0, -16);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, -12.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.9, 0.1, 0.9);
      glTranslated(0, 0, -9);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, -5.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.9, 0.9, 0.1);
      glTranslated(0, 0, -2);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, 1.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjustDetail(50, 3), adjustDetail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.8, 0.3, 0.3);
      glTranslated(-0.07, 0, -60.5);
      gluCylinder(quadric, 0.02, 0.02, 64,
                  adjustDetail(25, 3), adjustDetail(200, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.8, 0.3, 0.3);
      glTranslated(+0.07, 0, -60.5);
      gluCylinder(quadric, 0.02, 0.02, 64,
                  adjustDetail(25, 3), adjustDetail(200, 1));
      glPopMatrix();

      gluDeleteQuadric(quadric);
    }
  };

  int _main(int argc, const char *argv[])
  {
    Options o;

    o.addSwitch("h", "help", opt_help, true,
                "Describe the parameters");
    o.addConfig("m", "view-mode", opt_viewMode, opt_viewMode,
                "0 for one thread mono, 1 for two thread mono, "
                "2 for one thread paper stereo, 3 for two thread "
                "paper stereo, 4 for split screen stereo, 5 for "
                "dual window stereo, and 6 for angled screens.",
                Options::wantArg_always, Options::range(0, 7));
    o.addConfig("f", "frame-rate", opt_frameRate, opt_frameRate,
                "Upper limit on number of frames per second.",
                Options::wantArg_always);
    o.addConfig("s", "window-size", opt_windowSize, opt_windowSize,
                "A window size modifier 1 corresponds to normal size.",
                Options::wantArg_always);
    o.addConfig("d", "detail-level", opt_detailLevel, opt_detailLevel,
                "A detail level modifier, 1 corresponds to normal level of detail.",
                Options::wantArg_always);
    o.addConfig("D", "direct-rendering", opt_directRendering, opt_directRendering,
                "Attempt to establist direct rendering contexts to gain performance.",
                Options::wantArg_always);
	
    if(o.processCommandLine(argc, argv))
    {
      cerr << "Try --help\n";
      return 1;
    }
  
    if(opt_help)
    {
      cout <<
        "Test Application for the archon::Render library\n"
        "by Brian Kristiansen & Kristian Spangsege\n"
        "\n"
        "Synopsis: " << argv[0] << "\n"
        "\n"
        "Available options:\n";
      cout << o.list();
      return 0;
    }

    if(argc > 1)
    {
      cerr << "Too many aguments\n";
      cerr << "Try --help\n";
      return 1;
    }

    Display::Implementation::ConstRef implementation =
      Display::getDefaultImplementation();
    Display::Connection::ConstRef connection =
      implementation->newConnection();
    Display::Screen::ConstRef screen =
      connection->getDefaultScreen();
    Display::Visual::ConstRef visual =
      screen->chooseVisual();

    CntRef<MyRenderer> myRenderer(new MyRenderer);

    Conductor::Ref conductor =
      opt_viewMode == 0 ? oneThreadMono(visual, myRenderer) :
      opt_viewMode == 1 ? twoThreadMono(visual, myRenderer) :
      opt_viewMode == 2 ? oneThreadPaperStereo(visual, myRenderer) :
      opt_viewMode == 3 ? twoThreadPaperStereo(visual, myRenderer) :
      opt_viewMode == 4 ? splitScreenStereo(visual, myRenderer) :
      opt_viewMode == 5 ? dualWindowStereo(visual, myRenderer) :
      angledScreens(visual, myRenderer);

    RateMeter rate_meter("Frame rate (f/s): ", 10000);

    Time time_per_frame;
    time_per_frame.set_as_nanos(1000000000L/opt_frame_rate);

    Time t = Time::now();
    for(;;)
    {
      rate_meter.tick();

      conductor->render();

      my_renderer->w += 2*M_PI/opt_frame_rate/10;

      t += time_per_frame;
      Thread::sleep_until(t);
    }

    return 0;
  }
}


int main(int argc, char const *argv[]) throw()
{
  return _main(argc, argv);
}
