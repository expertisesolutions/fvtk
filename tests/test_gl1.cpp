#include<X11/X.h>
#include<X11/Xlib.h>
// #include <GL/glew.h>
// #define GLFW_DLL
//#include <GLFW/glfw3.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <iostream>
#include <fastdraw/renderer.hpp>
#include <fastdraw/object/line.hpp>
#include <fastdraw/object/image.hpp>
#include <fastdraw/output/opengl/draw_line.hpp>
#include <fastdraw/output/opengl/draw_image.hpp>
#include <fastdraw/output/opengl/opengl_output.hpp>

#include <variant>
#include <cassert>

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

// compile the shader.
//
GLuint LoadShader(GLenum type, const char *shaderSrc)
{
   GLuint shader;
   GLint compiled;
   // Create the shader object
   shader = glCreateShader(type);
   if(shader == 0)
      return 0;
   // Load the shader source
   glShaderSource(shader, 1, &shaderSrc, NULL);
   // Compile the shader
   glCompileShader(shader);
   // Check the compile status
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
   if(!compiled) 
   {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if(infoLen > 1)
      {
         char* infoLog = (char*)malloc(sizeof(char) * infoLen);
         glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
         printf("Error compiling shader:\n%s\n", infoLog);
         free(infoLog);
      }
      glDeleteShader(shader);
      return 0;
   }
   return shader;
}

static GLuint make_buffer(
    GLenum target,
    const void *buffer_data,
    GLsizei buffer_size
) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
    return buffer;
}

// static GLuint make_texture(const char *filename)
// {
//     GLuint texture;
//     int width, height;
//     void *pixels = read_tga(filename, &width, &height);

//     if (!pixels)
//         return 0;

//     glGenTextures(1, &texture);
//     glBindTexture(GL_TEXTURE_2D, texture);

//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

//     glTexImage2D(
//         GL_TEXTURE_2D, 0,           /* target, level of detail */
//         GL_RGB8,                    /* internal format */
//         width, height, 0,           /* width, height, border */
//         GL_BGR, GL_UNSIGNED_BYTE,   /* external format, type */
//         pixels                      /* pixels */
//     );
//     free(pixels);
//     return texture;
// }    

void render(GLuint programObject, GLuint texture0, GLuint vertex_buffer, GLuint element_buffer, GLuint VAO)
{
  static int multiplier = 1;
float vVertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

// 0. copy our vertices array in a buffer for OpenGL to use
 glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
 glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
 // 1. then set the vertex attributes pointers
 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
 glEnableVertexAttribArray(0);  
 // 2. use our shader program when we want to render an object
 glUseProgram(programObject);
 // 3. now draw the object

 glBindVertexArray(VAO);
 glDrawArrays(GL_TRIANGLES, 0, 3);
 
  // // GLfloat vVertices[] = {multiplier*0.0f,  multiplier*0.5f, multiplier*0.0f, 
  // //                        multiplier*(-0.5f), multiplier*(-0.5f), multiplier*0.0f,
  // //                        multiplier*0.5f, multiplier*(-0.5f),  multiplier*0.0f};

  //  glUseProgram(programObject);

  // //glTexCoord1f( 0.001f*(/*timeGetTime()-to*/10) );

  //  //glActiveTexture(GL_TEXTURE0);
  // //glBindTexture(GL_TEXTURE_2D, texture0);

  // // 1st attribute buffer : vertices
  //  //glEnableVertexAttribArray(0);
  // glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  // assert(glGetError() == GL_NO_ERROR);

  // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
  // glEnableVertexAttribArray(0);
  
  // // Load the vertex data
  // //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
  // assert(glGetError() == GL_NO_ERROR);

  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
  
  // glEnableVertexAttribArray(0);
  // assert(glGetError() == GL_NO_ERROR);
 //glDrawArrays(GL_TRIANGLES, 0, 3);
  // assert(glGetError() == GL_NO_ERROR);
  
}

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
    return -1;
  }

// make sure OpenGL's GLX extension supported
    int errorBase, eventBase;
    if( !glXQueryExtension( dpy, &errorBase, &eventBase ) )
      return -1;
    
  root = DefaultRootWindow(dpy);

  vi = glXChooseVisual(dpy, 0, att);

  if(vi == NULL) {
    printf("\n\tno appropriate visual found\n\n");
    return -1;
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

  // // Get a matching FB config
  // static int visual_attribs[] =
  //   {
  //     GLX_X_RENDERABLE    , True,
  //     GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
  //     GLX_RENDER_TYPE     , GLX_RGBA_BIT,
  //     GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
  //     GLX_RED_SIZE        , 8,
  //     GLX_GREEN_SIZE      , 8,
  //     GLX_BLUE_SIZE       , 8,
  //     GLX_ALPHA_SIZE      , 8,
  //     GLX_DEPTH_SIZE      , 24,
  //     GLX_STENCIL_SIZE    , 8,
  //     GLX_DOUBLEBUFFER    , True,
  //     //GLX_SAMPLE_BUFFERS  , 1,
  //     //GLX_SAMPLES         , 4,
  //     None
  //   };

  // int fbcount;
  // GLXFBConfig* fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attribs, &fbcount);
  
  // typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);


  // glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  // glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
  //          glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
  
  //  int context_attribs[] =
  //     {
  //       GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
  //       GLX_CONTEXT_MINOR_VERSION_ARB, 2,
  //       //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
  //       None
  //     };
  // glc = glXCreateContextAttribsARB( dpy, fbc[1]/*bestFbc*/, 0,
  //                                     True, context_attribs );

  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  assert(glGetError() == GL_NO_ERROR);
  glXMakeContextCurrent(dpy, win, win, glc);
  assert(glGetError() == GL_NO_ERROR);

  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  assert(glGetError() == GL_NO_ERROR);
  glBindVertexArray(VAO);
  assert(glGetError() == GL_NO_ERROR);

  //glewInit();
  assert(glGetError() == GL_NO_ERROR);
  
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* version = glGetString(GL_VERSION); // version as a string
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  glEnable(GL_DEPTH_TEST); 
  assert(glGetError() == GL_NO_ERROR);

// static const GLfloat g_vertex_buffer_data[] = { 
//     -1.0f, -1.0f,
//      1.0f, -1.0f,
//     -1.0f,  1.0f,
//      1.0f,  1.0f
// };
static const GLushort g_element_buffer_data[] = { 0, 1, 2, 3 };
float vVertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};  

 GLuint vertex_buffer = make_buffer(
        GL_ARRAY_BUFFER,
        /*g_vertex_buffer_data*/vVertices,
        sizeof(/*g_vertex_buffer_data*/vVertices)
    );
 // GLuint element_buffer = make_buffer(
 //        GL_ELEMENT_ARRAY_BUFFER,
 //        g_element_buffer_data,
 //        sizeof(g_element_buffer_data)
 //    );

 
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;
  {
   GLbyte vShaderStr[] =
     "#version 110\n"
#if 1
      "attribute"
#else
     "in"
#endif
     " vec4 vPosition;   \n"
      "void main()                 \n"
      "{                           \n"
     "   gl_Position = vPosition; \n"
      "}                           \n";
   GLbyte fShaderStr[] =  
     "#version 110\n"
     #if 0
      "#ifdef GL_ES\n"
      "precision mediump float;\n"
      "out vec4 fragmentColor;\n"
      "#endif\n"
     #endif
      "void main()                                \n"
      "{                                          \n"
     #if 0
      "#ifdef GL_ES\n"
      "  fragmentColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
      "#else\n"
     #endif
      "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
     #if 0
      "#endif\n"
     #endif
      "}                                          \n";

   // Load the vertex/fragment shaders
   vertexShader = LoadShader(GL_VERTEX_SHADER, (const char*)vShaderStr);
   assert(glGetError() == GL_NO_ERROR);
   fragmentShader = LoadShader(GL_FRAGMENT_SHADER, (const char*)fShaderStr);
   assert(glGetError() == GL_NO_ERROR);
   //Create the program object
   programObject = glCreateProgram();
        assert(glGetError() == GL_NO_ERROR);
   if(programObject == 0)
     std::abort();
   glAttachShader(programObject, vertexShader);
   std::cout << glGetError() << std::endl;
   assert(glGetError() == GL_NO_ERROR);
   glAttachShader(programObject, fragmentShader);
   assert(glGetError() == GL_NO_ERROR);
   // Bind vPosition to attribute 0   
   glBindAttribLocation(programObject, 0, "vPosition");
   assert(glGetError() == GL_NO_ERROR);
   // Link the program
   glLinkProgram(programObject);
   assert(glGetError() == GL_NO_ERROR);
   // Check the link status
   glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
        assert(glGetError() == GL_NO_ERROR);

        assert(!!linked);

        assert(glGetError() == GL_NO_ERROR);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        assert(glGetError() == GL_NO_ERROR);
        //glViewport(0, 0, gwa.width, gwa.height);
        assert(glGetError() == GL_NO_ERROR);
  }
  
  fastdraw::output::opengl_output output {};
  
  fastdraw::renderer<int, object> fdraw_renderer;
  fdraw_renderer.render_objects.push_back(fastdraw::object::line<int>{});
  
  while(1) {
    XNextEvent(dpy, &xev);
        
    if(xev.type == Expose) {
      XGetWindowAttributes(dpy, win, &gwa);

      {
        assert(glGetError() == GL_NO_ERROR);
        //glClear(GL_COLOR_BUFFER_BIT);
        // Use the program object

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        render(programObject, 0, vertex_buffer, /*element_buffer*/0, VAO);
        glClear(GL_COLOR_BUFFER_BIT);
        glXSwapBuffers(dpy, win);
      }
      
      //render (fdraw_renderer, output);
      
      //DrawAQuad(); 
    }
                
    else if(xev.type == KeyPress) {
      glXMakeCurrent(dpy, None, NULL);
      glXDestroyContext(dpy, glc);
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      return -1;
    }  
  }
  
  return -1;
}

