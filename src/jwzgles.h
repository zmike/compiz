/* xscreensaver, Copyright (c) 2012 Jamie Zawinski <jwz@jwz.org>
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

#ifndef __JWZGLES_H__
#define __JWZGLES_H__

#ifndef HAVE_JWZGLES
# error: do not include this without HAVE_JWZGLES
#endif


#include "jwzglesI.h"

#endif /* __JWZGLES_H__ */
