#include "config.h"
#include <Evas_GL.h>
#include <dlfcn.h>

static Evas_GL_API *glapi = NULL;

#define GLERR do { on_error(glapi, __FUNCTION__, __LINE__); } while (0)

static void
on_error(Evas_GL_API *api, const char *func, int line)
{
   int _e = api->glGetError();
   if (_e)
     {
        fprintf(stderr, "%s:%d: GL error 0x%04x\n", func, line, _e);
        abort();
     }
   return;
}

static void
init(void)
{
   Evas_GL_API **api;

   if (glapi) return;
   api = dlsym(NULL, "compiz_glapi");
   glapi = *api;
}

#define WRAP(NAME, TYPES, ARGS) \
void \
NAME TYPES \
{ \
   static int in; \
   init(); \
   if (in) \
     { \
        static void (*__##NAME)TYPES; \
\
        if (!__##NAME) \
          __##NAME = dlsym(RTLD_NEXT, #NAME); \
        __##NAME ARGS; \
        return; \
     }\
   in = 1; \
   glapi->NAME ARGS; \
   in = 0; \
}

WRAP(glVertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr), (size, type, stride, ptr))
WRAP(glTexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr), (size, type, stride, ptr))
WRAP(glEnableClientState, (GLenum cap), (cap))
WRAP(glDisableClientState, (GLenum cap), (cap))
WRAP(glPushMatrix, (void), ())
WRAP(glPopMatrix, (void), ())
WRAP(glDrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices), (mode, count, type, indices))
