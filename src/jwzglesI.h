/* xscreensaver, Copyright (c) 2012-2013 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* A compatibility shim to allow OpenGL 1.3 source code to work in an
   OpenGLES environment, where almost every OpenGL 1.3 function has
   been "deprecated".  See jwzgles.c for details.
 */

#ifndef __JWZGLES_I_H__
#define __JWZGLES_I_H__

#ifdef GL_VERSION_ES_CM_1_0  /* compiling against OpenGLES 1.x */

/* These OpenGL 1.3 constants are not present in OpenGLES 1.
   Fortunately, it looks like they didn't re-use any of the numbers,
   so we can just keep using the OpenGL 1.3 values.  I'm actually
   kind of shocked that the GLES folks passed up that opportunity
   for further clusterfuckery.
 */
# define GLdouble double

# define GL_ACCUM_BUFFER_BIT			0x00000200
# define GL_ALL_ATTRIB_BITS			0x000FFFFF
# define GL_AUTO_NORMAL				0x0D80
# define GL_BLEND_SRC_ALPHA			0x80CB
# define GL_C3F_V3F				0x2A24
# define GL_C4F_N3F_V3F				0x2A26
# define GL_C4UB_V2F				0x2A22
# define GL_C4UB_V3F				0x2A23
# define GL_CLAMP				0x2900
# define GL_COLOR_BUFFER_BIT			0x00004000
# define GL_COLOR_MATERIAL_FACE			0x0B55
# define GL_COLOR_MATERIAL_PARAMETER		0x0B56
# define GL_COMPILE				0x1300
# define GL_CURRENT_BIT				0x00000001
# define GL_DEPTH_BUFFER_BIT			0x00000100
# define GL_DOUBLEBUFFER			0x0C32
# define GL_ENABLE_BIT				0x00002000
# define GL_EVAL_BIT				0x00010000
# define GL_EYE_LINEAR				0x2400
# define GL_EYE_PLANE				0x2502
# define GL_FEEDBACK				0x1C01
# define GL_FILL				0x1B02
# define GL_FOG_BIT				0x00000080
# define GL_HINT_BIT				0x00008000
# define GL_INTENSITY				0x8049
# define GL_LIGHTING_BIT			0x00000040
# define GL_LIGHT_MODEL_COLOR_CONTROL		0x81F8
# define GL_LIGHT_MODEL_LOCAL_VIEWER		0x0B51
# define GL_LINE				0x1B01
# define GL_LINE_BIT				0x00000004
# define GL_LIST_BIT				0x00020000
# define GL_N3F_V3F				0x2A25
# define GL_OBJECT_LINEAR			0x2401
# define GL_OBJECT_PLANE			0x2501
# define GL_PIXEL_MODE_BIT			0x00000020
# define GL_POINT_BIT				0x00000002
# define GL_POLYGON				0x0009
# define GL_POLYGON_BIT				0x00000008
# define GL_POLYGON_MODE			0x0B40
# define GL_POLYGON_SMOOTH			0x0B41
# define GL_POLYGON_STIPPLE			0x0B42
# define GL_POLYGON_STIPPLE_BIT			0x00000010
# define GL_Q					0x2003
# define GL_QUADS				0x0007
# define GL_QUAD_STRIP				0x0008
# define GL_R					0x2002
# define GL_RENDER				0x1C00
# define GL_RGBA_MODE				0x0C31
# define GL_S					0x2000
# define GL_SCISSOR_BIT				0x00080000
# define GL_SELECT				0x1C02
# define GL_SEPARATE_SPECULAR_COLOR		0x81FA
# define GL_SINGLE_COLOR			0x81F9
# define GL_SPHERE_MAP				0x2402
# define GL_STENCIL_BUFFER_BIT			0x00000400
# define GL_T					0x2001
# define GL_T2F_C3F_V3F				0x2A2A
# define GL_T2F_C4F_N3F_V3F			0x2A2C
# define GL_T2F_C4UB_V3F			0x2A29
# define GL_T2F_N3F_V3F				0x2A2B
# define GL_T2F_V3F				0x2A27
# define GL_T4F_C4F_N3F_V4F			0x2A2D
# define GL_T4F_V4F				0x2A28
# define GL_TEXTURE_1D				0x0DE0
# define GL_TEXTURE_ALPHA_SIZE			0x805F
# define GL_TEXTURE_BIT				0x00040000
# define GL_TEXTURE_BLUE_SIZE			0x805E
# define GL_TEXTURE_BORDER			0x1005
# define GL_TEXTURE_BORDER_COLOR		0x1004
# define GL_TEXTURE_COMPONENTS			0x1003
# define GL_TEXTURE_GEN_MODE			0x2500
# define GL_TEXTURE_GEN_Q			0x0C63
# define GL_TEXTURE_GEN_R			0x0C62
# define GL_TEXTURE_GEN_S			0x0C60
# define GL_TEXTURE_GEN_T			0x0C61
# define GL_TEXTURE_GREEN_SIZE			0x805D
# define GL_TEXTURE_HEIGHT			0x1001
# define GL_TEXTURE_INTENSITY_SIZE		0x8061
# define GL_TEXTURE_LUMINANCE_SIZE		0x8060
# define GL_TEXTURE_RED_SIZE			0x805C
# define GL_TEXTURE_WIDTH			0x1000
# define GL_TRANSFORM_BIT			0x00001000
# define GL_UNPACK_ROW_LENGTH			0x0CF2
# define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
# define GL_V2F					0x2A20
# define GL_V3F					0x2A21
# define GL_VIEWPORT_BIT			0x00000800

#endif


/* Prototypes for the things re-implemented in jwzgles.c 
 */

int  glGenLists (int n);
void glNewList (int id, int mode);
void glEndList (void);
void glDeleteLists (int list, int range);
void glBegin (int mode);
void glNormal3fv (const GLfloat *);
void glNormal3f (GLfloat x, GLfloat y, GLfloat z);
void glTexCoord1f (GLfloat s);
void glTexCoord2fv (const GLfloat *);
void glTexCoord2f (GLfloat s, GLfloat t);
void glTexCoord3fv (const GLfloat *);
void glTexCoord3f (GLfloat s, GLfloat t, GLfloat r);
void glTexCoord4fv (const GLfloat *);
void glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glVertex2f (GLfloat x, GLfloat y);
void glVertex2fv (const GLfloat *);
void glVertex2i (GLint x, GLint y);
void glVertex3f (GLfloat x, GLfloat y, GLfloat z);
void glVertex3dv (const GLdouble *);
void glVertex3fv (const GLfloat *);
void glVertex3i (GLint x, GLint y, GLint z);
void glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertex4fv (const GLfloat *);
void glVertex4i (GLint x, GLint y, GLint z, GLint w);
void glEnd (void);
void glCallList (int id);
void glClearIndex(GLfloat c);
void glBitmap (GLsizei, GLsizei, GLfloat, GLfloat, GLfloat,
                              GLfloat, const GLubyte *);
void glPushAttrib(int);
void glPopAttrib(void);


/* These functions are present in both OpenGL 1.3 and in OpenGLES 1,
   but are allowed within glNewList/glEndList, so we must wrap them
   to allow them to be recorded.
 */
void glActiveTexture (GLuint);
void glBindTexture (GLuint, GLuint);
void glBlendFunc (GLuint, GLuint);
void glClear (GLuint);
void glClearColor (GLclampf, GLclampf, GLclampf, GLclampf);
void glClearStencil (GLuint);
void glColorMask (GLuint, GLuint, GLuint, GLuint);
void glCullFace (GLuint);
void glDepthFunc (GLuint);
void glDepthMask (GLuint);
void glDisable (GLuint);
void glDrawArrays (GLuint, GLuint, GLuint);
GLboolean glIsEnabled (GLuint);
void glEnable (GLuint);
void glFrontFace (GLuint);
void glHint (GLuint, GLuint);
void glLineWidth (GLfloat);
void glLoadIdentity (void);
void glLogicOp (GLuint);
void glMatrixMode (GLuint);
void glMultMatrixf (const GLfloat *);
void glPointSize (GLfloat);
void glPolygonOffset (GLfloat, GLfloat);
void glPopMatrix (void);
void glPushMatrix (void);
void glScissor (GLuint, GLuint, GLuint, GLuint);
void glShadeModel (GLuint);
void glStencilFunc (GLuint, GLuint, GLuint);
void glStencilMask (GLuint);
void glStencilOp (GLuint, GLuint, GLuint);
void glViewport (GLuint, GLuint, GLuint, GLuint);
void glTranslatef (GLfloat, GLfloat, GLfloat);
void glRotatef (GLfloat, GLfloat, GLfloat, GLfloat);
void glRotated (GLdouble, GLdouble x, GLdouble y, GLdouble z);
void glScalef (GLfloat, GLfloat, GLfloat);
void glColor3f (GLfloat, GLfloat, GLfloat);
void glColor4f (GLfloat, GLfloat, GLfloat, GLfloat);
void glColor3fv (const GLfloat *);
void glColor4fv (const GLfloat *);
void glColor3s (GLshort, GLshort, GLshort);
void glColor4s (GLshort, GLshort, GLshort, GLshort);
void glColor3sv (const GLshort *);
void glColor4sv (const GLshort *);
void glColor3us (GLushort, GLushort, GLushort);
void glColor4us (GLushort, GLushort, GLushort, GLushort);
void glColor3usv (const GLushort *);
void glColor4usv (const GLushort *);
void glColor3d (GLdouble, GLdouble, GLdouble);
void glColor4d (GLdouble, GLdouble, GLdouble, GLdouble);
void glColor3dv (const GLdouble *);
void glColor4dv (const GLdouble *);
void glColor4i (GLint, GLint, GLint, GLint);
void glColor3i (GLint, GLint, GLint);
void glColor3iv (const GLint *);
void glColor4iv (const GLint *);
void glColor4ui (GLuint, GLuint, GLuint, GLuint);
void glColor3ui (GLuint, GLuint, GLuint);
void glColor3uiv (const GLuint *);
void glColor4uiv (const GLuint *);
void glColor4b (GLbyte, GLbyte, GLbyte, GLbyte);
void glColor3b (GLbyte, GLbyte, GLbyte);
void glColor4bv (const GLbyte *);
void glColor3bv (const GLbyte *);
void glColor4ub (GLubyte, GLubyte, GLubyte, GLubyte);
void glColor3ub (GLubyte, GLubyte, GLubyte);
void glColor4ubv (const GLubyte *);
void glColor3ubv (const GLubyte *);
void glMaterialf (GLuint, GLuint, GLfloat);
void glMateriali (GLuint, GLuint, GLuint);
void glMaterialfv (GLuint, GLuint, const GLfloat *);
void glMaterialiv (GLuint, GLuint, const GLint *);
void glFinish (void);
void glFlush (void);
void glPixelStorei (GLuint, GLuint);
void glEnableClientState (GLuint);
void glDisableClientState (GLuint);

void glInitNames (void);
void glPushName (GLuint);
GLuint glPopName (void);
GLuint glRenderMode (GLuint);
void glSelectBuffer (GLsizei, GLuint *);
void glLightf (GLenum, GLenum, GLfloat);
void glLighti (GLenum, GLenum, GLint);
void glLightfv (GLenum, GLenum, const GLfloat *);
void glLightiv (GLenum, GLenum, const GLint *);
void glLightModelf (GLenum, GLfloat);
void glLightModeli (GLenum, GLint);
void glLightModelfv (GLenum, const GLfloat *);
void glLightModeliv (GLenum, const GLint *);
void glGenTextures (GLuint, GLuint *);
void glFrustum (GLfloat, GLfloat, GLfloat, GLfloat,
                               GLfloat, GLfloat);
void glOrtho (GLfloat, GLfloat, GLfloat, GLfloat, 
                             GLfloat, GLfloat);
void glTexImage1D (GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLsizei width, GLint border,
                                  GLenum format, GLenum type,
                                  const GLvoid *pixels);
void glTexImage2D (GLenum target,
                                  GLint  	level,
                                  GLint  	internalFormat,
                                  GLsizei  	width,
                                  GLsizei  	height,
                                  GLint  	border,
                                  GLenum  	format,
                                  GLenum  	type,
                                  const GLvoid *data);
void glTexSubImage2D (GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format, GLenum type,
                                     const GLvoid *pixels);
void glCopyTexImage2D (GLenum target, GLint level, 
                                      GLenum internalformat,
                                      GLint x, GLint y, 
                                      GLsizei width, GLsizei height, 
                                      GLint border);
void glInterleavedArrays (GLenum, GLsizei, const GLvoid *);
void glTexEnvf (GLuint, GLuint, GLfloat);
void glTexEnvi (GLuint, GLuint, GLuint);
void glTexParameterf (GLuint, GLuint, GLfloat);
void glTexParameteri (GLuint, GLuint, GLuint);
void glTexGeni (GLenum, GLenum, GLint);
void glTexGenfv (GLenum, GLenum, const GLfloat *);
void glRectf (GLfloat, GLfloat, GLfloat, GLfloat);
void glRecti (GLint, GLint, GLint, GLint);
void glLightModelfv (GLenum, const GLfloat *);
void glClearDepth (GLfloat);
GLboolean glIsList (GLuint);
void glColorMaterial (GLenum, GLenum);
void glPolygonMode (GLenum, GLenum);
void glFogf (GLenum, GLfloat);
void glFogi (GLenum, GLint);
void glFogfv (GLenum, const GLfloat *);
void glFogiv (GLenum, const GLint *);
void glAlphaFunc (GLenum, GLfloat);
void glClipPlane (GLenum, const GLdouble *);
void glDrawBuffer (GLenum);
void glDeleteTextures (GLuint, const GLuint *);

void gluPerspective (GLdouble fovy, GLdouble aspect, 
                                    GLdouble near, GLdouble far);
void gluLookAt (GLfloat eyex, GLfloat eyey, GLfloat eyez,
                               GLfloat centerx, GLfloat centery, 
                               GLfloat centerz,
                               GLfloat upx, GLfloat upy, GLfloat upz);
GLint gluProject (GLdouble objx, GLdouble objy, GLdouble objz, 
                                 const GLdouble modelMatrix[16], 
                                 const GLdouble projMatrix[16],
                                 const GLint viewport[4],
                                 GLdouble *winx, GLdouble *winy, 
                                 GLdouble *winz);
int gluBuild2DMipmaps (GLenum target,
                                      GLint internalFormat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLenum format,
                                      GLenum type,
                                      const GLvoid *data);
void glGetFloatv (GLenum pname, GLfloat *params);
void glGetPointerv (GLenum pname, GLvoid *params);
void glGetDoublev (GLenum pname, GLdouble *params);
void glGetIntegerv (GLenum pname, GLint *params);
void glGetBooleanv (GLenum pname, GLboolean *params);
void glVertexPointer (GLuint, GLuint, GLuint, const void *);
void glNormalPointer (GLenum, GLuint, const void *);
void glColorPointer (GLuint, GLuint, GLuint, const void *);
void glTexCoordPointer (GLuint, GLuint, GLuint, const void *);
void glBindBuffer (GLuint, GLuint);
void glBufferData (GLenum, GLsizeiptr, const void *, GLenum);
const char *gluErrorString (GLenum error);

#endif /* __JWZGLES_I_H__ */
