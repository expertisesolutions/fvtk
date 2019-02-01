#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>
#include <iostream>
#include <fastdraw/renderer.hpp>
#include <fastdraw/object/line.hpp>
#include <fastdraw/object/image.hpp>
#include <fastdraw/output/opengl/draw_line.hpp>
#include <fastdraw/output/opengl/draw_image.hpp>

#include <variant>

typedef std::variant<fastdraw::object::line<int>, fastdraw::object::image<int>> object_variant;

struct object : object_variant
{
  template <typename T>
  object(T&& v) : object_variant(std::move(v)) {}
};

template <typename Output>
void draw (object& obj, Output& out)
{
  std::visit([&out] (auto&& v) { draw(v, out); }, static_cast<object_variant&>(obj));
}

double output;

int main()
{
  Display                 *dpy;
  Window                  root;
  GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
  XVisualInfo             *vi;
  Colormap                cmap;
  XSetWindowAttributes    swa;
  Window                  win;
  GLXContext              glc;
  XWindowAttributes       gwa;
  XEvent                  xev;
  
  dpy = XOpenDisplay(NULL);
 
  if(dpy == NULL) {
    printf("\n\tcannot connect to X server\n\n");
    exit(0);
  }
        
  root = DefaultRootWindow(dpy);

  vi = glXChooseVisual(dpy, 0, att);

  if(vi == NULL) {
    printf("\n\tno appropriate visual found\n\n");
    exit(0);
  } 
  else {
    printf("\n\tvisual %p selected\n", (void *)vi->visualid); /* %p creates hexadecimal output like in glxinfo */
  }

  cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask;
 
  win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

  XMapWindow(dpy, win);
  XStoreName(dpy, win, "VERY SIMPLE APPLICATION");
 
  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glXMakeCurrent(dpy, win, glc);
 
  glEnable(GL_DEPTH_TEST); 

  fastdraw::renderer<int, object> renderer;
  renderer.render_objects.push_back(fastdraw::object::line<int>{});
  
  while(1) {
    XNextEvent(dpy, &xev);
        
    if(xev.type == Expose) {
      XGetWindowAttributes(dpy, win, &gwa);
      glViewport(0, 0, gwa.width, gwa.height);

      render (renderer, output);
      
      //DrawAQuad(); 
      glXSwapBuffers(dpy, win);
    }
                
    else if(xev.type == KeyPress) {
      glXMakeCurrent(dpy, None, NULL);
      glXDestroyContext(dpy, glc);
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      exit(0);
    }  
  }
  
  return -1;
}

