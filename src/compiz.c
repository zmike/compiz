/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "e_mod_main.h"

static Eina_Hash *wins;
static Eina_Hash *clients;
static Eina_Hash *textures;
static Eina_Hash *wintextures;
static Eina_List *handlers;
static Eina_List *hooks;
static Eina_Array *events;
static Ecore_Animator *compiz_anim;
static Ecore_Timer *compiz_timer;

static Eina_Bool init = EINA_FALSE;

E_API int pointerX = 0;
E_API int pointerY = 0;
E_API int lastPointerX = 0;
E_API int lastPointerY = 0;

typedef struct
{
   Evas_GL_Context *ctx;
   Evas_GL_Surface *sfc;
   GLuint fbo;
   E_Client *ec;
   Evas_Object *obj;
   Eina_Tiler *damage;
   Eina_Bool clear : 1;
} Compiz_GL;
EINTERN Evas_GL *gl = NULL;
static Evas_GL_Config *glcfg = NULL;
E_API Evas_GL_API *compiz_glapi = NULL;

EINTERN int compiz_main(void);
EINTERN void compiz_fini(void);
EINTERN void handleDisplayEvent(Display *dpy, XEvent *event);
EINTERN void handleTimeouts (struct timeval *tv);
EINTERN void handleAddClient(E_Client *ec);
extern CompCore core;

static int movegrab;
static int resizegrab;

static int ecx, ecy;

static Ecore_Job *event_job;

static void
compiz_render_resize(Compiz_GL *cgl)
{
   evas_object_geometry_set(cgl->obj, 0, 0, e_comp->w, e_comp->h);
   evas_object_image_size_set(cgl->obj, e_comp->w, e_comp->h);
}

static void
compiz_damage(void)
{
   CompDisplay *d;
   CompScreen *s;
   CompWindow *w;
   Eina_Bool thaw = EINA_FALSE;

   for (d = core.displays; d; d = d->next)
     for (s = d->screens; s; s = s->next)
       {
          if (!s->damageMask)
            for (w = s->windows; w; w = w->next)
              {
                 E_Client *ec;

                 ec = compiz_win_to_client(w);
                 if ((w->attrib.x == ec->client.x) && (w->attrib.y == ec->client.y))
                   continue;
                 moveWindow(w, ec->client.x - w->attrib.x, ec->client.y - w->attrib.y, TRUE, TRUE);
                 syncWindowPosition(w);
                 if (w->grabbed)
                   w->screen->windowUngrabNotify(w);
              }
          if (s->damageMask)
            {
               evas_damage_rectangle_add(e_comp->evas, 0, 0, 1, 1);
               thaw = EINA_TRUE;
            }
       }
   if (eina_array_count(events) || thaw)
     ecore_animator_thaw(compiz_anim);
   else
     ecore_animator_freeze(compiz_anim);
}

static void compiz_timers(void);

static Eina_Bool
compiz_timer_cb(void *d EINA_UNUSED)
{
   struct timeval tv;

   gettimeofday (&tv, 0);
   handleTimeouts(&tv);
   compiz_timers();
   return EINA_TRUE;
}

static void
compiz_timers(void)
{
   if (core.timeouts)
     {
        if (compiz_timer)
          {
             ecore_timer_interval_set(compiz_timer, core.timeouts->minLeft);
             ecore_timer_reset(compiz_timer);
          }
        else
          compiz_timer = ecore_timer_add(core.timeouts->minLeft, compiz_timer_cb, NULL);
     }
   else
     E_FREE_FUNC(compiz_timer, ecore_timer_del);
}

static void
compiz_client_frame_recalc(E_Client *ec)
{
   updateWindowOutputExtents(eina_hash_find(clients, &ec));
}

static void
compiz_client_frame_calc(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   compiz_client_frame_recalc(data);
}

static Eina_Bool
compiz_idle_cb(void *data EINA_UNUSED)
{
   struct timeval tv;

   while (eina_array_count(events))
     {
        XEvent *ev;

        ev = eina_array_pop(events);
        if (ev->type == CreateNotify)
          {
             E_Client *ec;

             ec = e_pixmap_find_client(E_PIXMAP_TYPE_X, ev->xcreatewindow.window);
             if (!ec)
               ec = e_pixmap_find_client(E_PIXMAP_TYPE_WL, ev->xcreatewindow.window);
             handleAddClient(ec);
             compiz_client_frame_recalc(ec);
          }
        handleDisplayEvent(ecore_x_display_get(), ev);
        free(ev);
     }
   gettimeofday (&tv, 0);
   handleTimeouts(&tv);
   compiz_timers();
   compiz_damage();
   return EINA_TRUE;
}

static void
compiz_job(XEvent *event)
{
   eina_array_push(events, event);
   ecore_animator_thaw(compiz_anim);
}

static void
createnotify(E_Client *ec)
{
   XEvent *event;

   event = calloc(1, sizeof(XEvent));
   event->type = CreateNotify;
   event->xcreatewindow.display = ecore_x_display_get();
   if (ec->parent)
     event->xcreatewindow.parent = window_get(ec->parent);
   else
     event->xcreatewindow.parent = e_comp->root;
   event->xcreatewindow.window = window_get(ec);
   event->xcreatewindow.x = ec->client.x;
   event->xcreatewindow.y = ec->client.y;
   event->xcreatewindow.width = ec->client.w;
   event->xcreatewindow.height = ec->client.h;
   event->xcreatewindow.override_redirect = ec->override;
   e_object_ref(E_OBJECT(ec));
   compiz_job(event);
}

static void
compiz_client_new_client(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   E_Client *ec = data;

   createnotify(ec);
   evas_object_smart_callback_del(ec->frame, "new_client", compiz_client_new_client);
}

static Eina_Bool
compiz_client_add(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   if (ev->ec->new_client)
     evas_object_smart_callback_add(ev->ec->frame, "new_client", compiz_client_new_client, ev->ec);
   else
     createnotify(ev->ec);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_del(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   XEvent *event;
   CompWindow *w;

   w = eina_hash_find(clients, &ev->ec);
   event = calloc(1, sizeof(XEvent));
   event->type = DestroyNotify;
   event->xdestroywindow.display = ecore_x_display_get();
   event->xdestroywindow.window = event->xdestroywindow.event = w->id;
   evas_object_smart_callback_del(ev->ec->frame, "new_client", compiz_client_new_client);
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_configure(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   XEvent *event;
   E_Client *bec;

   event = calloc(1, sizeof(XEvent));
   event->type = ConfigureNotify;
   event->xconfigure.display = ecore_x_display_get();
   event->xconfigure.event = event->xconfigure.window = window_get(ev->ec);
   event->xconfigure.x = ev->ec->client.x;
   event->xconfigure.y = ev->ec->client.y;
   event->xconfigure.width = ev->ec->client.w;
   event->xconfigure.height = ev->ec->client.h;
   bec = e_client_below_get(ev->ec);
   if (bec)
     event->xconfigure.above = window_get(bec);
   event->xconfigure.override_redirect = ev->ec->override;
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_show(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   XEvent *event;

   event = calloc(1, sizeof(XEvent));
   event->type = MapNotify;
   event->xmap.display = ecore_x_display_get();
   event->xmap.event = event->xmap.window = window_get(ev->ec);
   event->xmap.override_redirect = ev->ec->override;
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_hide(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   XEvent *event;

   if (ev->ec->hidden) return ECORE_CALLBACK_RENEW;
   event = calloc(1, sizeof(XEvent));
   event->type = UnmapNotify;
   event->xunmap.display = ecore_x_display_get();
   event->xunmap.event = event->xunmap.window = window_get(ev->ec);
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

EINTERN unsigned int
client_state_to_compiz_state(const E_Client *ec)
{
   unsigned int state = 0;

   if (ec->netwm.state.modal)
     state |= CompWindowStateModalMask;
   if (ec->netwm.state.sticky)
     state |= CompWindowStateStickyMask;
   if (ec->netwm.state.maximized_v)
     state |= CompWindowStateMaximizedVertMask;
   if (ec->netwm.state.maximized_h)
     state |= CompWindowStateMaximizedHorzMask;
   if (ec->netwm.state.shaded)
     state |= CompWindowStateShadedMask;
   if (ec->netwm.state.skip_taskbar)
     state |= CompWindowStateSkipTaskbarMask;
   if (ec->netwm.state.skip_pager)
     state |= CompWindowStateSkipPagerMask;
   if (ec->netwm.state.hidden)
     state |= CompWindowStateHiddenMask;
   if (ec->netwm.state.fullscreen)
     state |= CompWindowStateFullscreenMask;
   switch (ec->netwm.state.stacking)
     {
        case E_STACKING_ABOVE:
          state |= CompWindowStateAboveMask;
          break;
        case E_STACKING_BELOW:
          state |= CompWindowStateBelowMask;
          break;
        default: break;
     }
   if (ec->icccm.urgent)
     state |= CompWindowStateDemandsAttentionMask;
//CompWindowStateDisplayModalMask;
   return state;
}

EINTERN unsigned int
win_type_to_compiz(E_Window_Type type)
{
   switch (type)
     {
      case E_WINDOW_TYPE_DESKTOP: return CompWindowTypeDesktopMask;
      case E_WINDOW_TYPE_DOCK: return CompWindowTypeDockMask;
      case E_WINDOW_TYPE_TOOLBAR: return CompWindowTypeToolbarMask;
      case E_WINDOW_TYPE_MENU: return CompWindowTypeMenuMask;
      case E_WINDOW_TYPE_UTILITY: return CompWindowTypeUtilMask;
      case E_WINDOW_TYPE_SPLASH: return CompWindowTypeSplashMask;
      case E_WINDOW_TYPE_DIALOG: return CompWindowTypeDialogMask;
      case E_WINDOW_TYPE_NORMAL: return CompWindowTypeNormalMask;
      case E_WINDOW_TYPE_DROPDOWN_MENU: return CompWindowTypeDropdownMenuMask;
      case E_WINDOW_TYPE_POPUP_MENU: return CompWindowTypePopupMenuMask;
      case E_WINDOW_TYPE_TOOLTIP: return CompWindowTypeTooltipMask;
      case E_WINDOW_TYPE_NOTIFICATION: return CompWindowTypeNotificationMask;
      case E_WINDOW_TYPE_COMBO: return CompWindowTypeComboMask;
      case E_WINDOW_TYPE_DND: return CompWindowTypeDndMask;
      default: break;
     }
   return CompWindowTypeUnknownMask;
}

EINTERN E_Client *
compiz_win_to_client(CompWindow *w)
{
   return eina_hash_find(wins, &w);
}

static void
win_hash_free(E_Client *ec)
{
   eina_hash_del_by_key(clients, &ec);
   e_object_unref(E_OBJECT(ec));
}

static void
compiz_client_stack(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Event_Client ev;

   ev.ec = data;
   compiz_client_configure(NULL, 0, &ev);
}

static void
compiz_client_damage(void *data, Evas_Object *obj, void *event_info)
{
   CompWindow *w = data;
   E_Client *ec;
   Compiz_GL *cgl;
   Eina_Rectangle *r = event_info;
   XEvent *event;
   XDamageNotifyEvent *de;

   ec = e_comp_object_client_get(obj);
   cgl = eina_hash_find(textures, &w->texture);
   if (cgl)
     eina_tiler_rect_add(cgl->damage, &(Eina_Rectangle){ec->client.x + r->x, ec->client.y + r->y, r->w, r->h});
   event = calloc(1, sizeof(XEvent));
   event->type = DAMAGE_EVENT + XDamageNotify;
   de = (XDamageNotifyEvent*)event;
   de->drawable = window_get(ec);
   de->display = ecore_x_display_get();
   de->area.x = r->x;
   de->area.y = r->y;
   de->area.width = r->w;
   de->area.height = r->h;
   de->geometry.x = ec->client.x;
   de->geometry.y = ec->client.y;
   de->geometry.width = ec->client.w;
   de->geometry.height = ec->client.h;
   compiz_job(event);
}

static void
compiz_client_render(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   fprintf(stderr, "COMPIZ RENDER\n");
}

static void
compiz_client_maximize(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   CompWindow *w = data;
   E_Client *ec = e_comp_object_client_get(obj);

   if (ec->maximized) //unmaximizing
     changeWindowState(w, w->state ^= MAXIMIZE_STATE);
   else
     changeWindowState(w, w->state |= MAXIMIZE_STATE);
   updateWindowAttributes(w, CompStackingUpdateModeNone);
}

static void
compiz_client_shade(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   CompWindow *w = data;
   E_Client *ec = e_comp_object_client_get(obj);

   if (ec->shaded)
     changeWindowState(w, w->state ^= CompWindowStateShadedMask);
   else
     changeWindowState(w, w->state |= CompWindowStateShadedMask);
   updateWindowAttributes(w, CompStackingUpdateModeNone);
}

EINTERN void
compiz_win_hash_client(CompWindow *w, E_Client *ec)
{
   E_Event_Client ev;

   eina_hash_set(wins, &w, ec);
   eina_hash_set(clients, &ec, w);
   evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_RESTACK, compiz_client_stack, ec);
   evas_object_smart_callback_add(ec->frame, "render", compiz_client_render, w);
   evas_object_smart_callback_add(ec->frame, "damage", compiz_client_damage, w);
   evas_object_smart_callback_add(ec->frame, "maximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_add(ec->frame, "unmaximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_add(ec->frame, "shaded", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "unshaded", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "shade_done", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "frame_recalc_done", compiz_client_frame_calc, ec);
   ev.ec = ec;
   compiz_client_configure(NULL, 0, &ev);
   ecore_job_add((Ecore_Cb)compiz_idle_cb, NULL);
}

EINTERN void
compiz_win_hash_del(CompWindow *w)
{
   eina_hash_del_by_key(wins, &w);
}

EINTERN void
compiz_texture_activate(CompTexture *texture, Eina_Bool set)
{
   Compiz_GL *cgl;

   if (!set)
     compiz_glapi->glFinish();
   if (!set) return;
   cgl = eina_hash_find(textures, &texture);
   evas_gl_make_current(gl, cgl->sfc, cgl->ctx);
   compiz_glapi->glViewport(0, 0, e_comp->w, e_comp->h);
   if (cgl->clear)
     compiz_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   cgl->clear = 0;
}

EINTERN void
compiz_texture_clear(CompTexture *texture)
{
   Compiz_GL *cgl;

   cgl = eina_hash_find(textures, &texture);
   if (cgl) cgl->clear = 1;
}

static void
cshow(void *data, ...)
{
   evas_object_show(data);
}

static void
chide(void *data, ...)
{
   evas_object_hide(data);
}

static void
cdirty(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Compiz_GL *cgl = data;
   Evas_Native_Surface ns;

   if (eina_tiler_empty(cgl->damage))
     evas_object_image_data_update_add(cgl->obj, cgl->ec->client.x, cgl->ec->client.y, cgl->ec->client.w, cgl->ec->client.h);
   else
     {
        Eina_Iterator *it;
        Eina_Rectangle *r;

        it = eina_tiler_iterator_new(cgl->damage);
        EINA_ITERATOR_FOREACH(it, r)
          evas_object_image_data_update_add(cgl->obj, r->x, r->y, r->w, r->h);
        eina_iterator_free(it);
     }
   eina_tiler_clear(cgl->damage);
   evas_gl_native_surface_get(gl, cgl->sfc, &ns);
   evas_object_image_native_surface_set(cgl->obj, &ns);
}

static void
cshade(void *d, Evas_Object *obj EINA_UNUSED, void *ev EINA_UNUSED)
{
   Compiz_GL *cgl = d;

   if (!cgl->ec->shaded)
     evas_object_hide(cgl->obj);
}

static void
cpixels(void *d EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{}

EINTERN void
compiz_texture_init(CompTexture *texture)
{
   Compiz_GL *cgl;
   E_Client *ec;
   CompWindow *win;
   int w, h;
   Evas_Native_Surface ns;
   GLfloat globalAmbient[] = { 0.1f, 0.1f, 0.1f, 0.1f };
   GLfloat ambientLight[] = { 0.0f, 0.0f, 0.0f, 0.0f };
   GLfloat diffuseLight[] = { 0.9f, 0.9f, 0.9f, 0.9f };
   GLfloat light0Position[] = { -0.5f, 0.5f, -9.0f, 1.0f };

   win = eina_hash_find(wintextures, &texture);
   if (!win) abort();
   evas_gl_make_current(gl, NULL, NULL);
   ec = compiz_win_to_client(win);
   e_pixmap_size_get(ec->pixmap, &w, &h);
   cgl = eina_hash_find(textures, &texture);
   if (cgl) return;
     {
        cgl = malloc(sizeof(Compiz_GL));
        cgl->ctx = evas_gl_context_create(gl, NULL);
        cgl->ec = ec;
        compiz_client_frame_recalc(ec);
        cgl->obj = evas_object_image_filled_add(e_comp->evas);
        evas_object_name_set(cgl->obj, "compiz!");
        evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_SHOW, (void*)cshow, cgl->obj);
        evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_HIDE, (void*)chide, cgl->obj);
        evas_object_smart_callback_add(ec->frame, "shaded", (void*)chide, cgl->obj);
        evas_object_smart_callback_add(ec->frame, "shade_done", cshade, cgl);
        evas_object_smart_callback_add(ec->frame, "unshading", (void*)cshow, cgl->obj);
        evas_object_smart_callback_add(ec->frame, "unshaded", (void*)cshow, cgl->obj);
        evas_object_smart_callback_add(ec->frame, "dirty", cdirty, cgl);
        evas_object_image_data_update_add(cgl->obj, 0, 0, e_comp->w, e_comp->h);
        evas_object_image_alpha_set(cgl->obj, 1);
        evas_object_pass_events_set(cgl->obj, 1);
        evas_object_image_pixels_get_callback_set(cgl->obj, cpixels, NULL);
        evas_object_smart_member_add(cgl->obj, ec->frame);
        //evas_object_layer_set(cgl->obj, E_LAYER_CLIENT_ABOVE);
        cgl->damage = eina_tiler_new(e_comp->w, e_comp->h);
        if (evas_object_visible_get(ec->frame))
          evas_object_show(cgl->obj);
        evas_gl_make_current(gl, NULL, cgl->ctx);
        compiz_glapi->glGenFramebuffers(1, &cgl->fbo);
        compiz_glapi->glClearColor (0.0, 0.0, 0.0, 0.0);
        compiz_glapi->glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        compiz_glapi->glEnable (GL_CULL_FACE);
        compiz_glapi->glDisable (GL_BLEND);
        compiz_glapi->glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        compiz_glapi->glEnableClientState (GL_VERTEX_ARRAY);
        compiz_glapi->glEnableClientState (GL_TEXTURE_COORD_ARRAY);
        compiz_glapi->glMatrixMode(GL_PROJECTION);
        compiz_glapi->glLoadIdentity();
        compiz_glapi->glMultMatrixf(win->screen->projection);
        compiz_glapi->glMatrixMode(GL_MODELVIEW);
        compiz_glapi->glLoadIdentity();
        if (!win->screen->maxTextureSize)
          compiz_glapi->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &win->screen->maxTextureSize);

        compiz_glapi->glEnable(GL_LIGHT0);
        compiz_glapi->glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
        compiz_glapi->glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
        compiz_glapi->glLightfv(GL_LIGHT0, GL_POSITION, light0Position);
        eina_hash_add(textures, &texture, cgl);
        win->id = window_get(ec);
     }
   compiz_render_resize(cgl);
   cgl->sfc = evas_gl_surface_create(gl, glcfg, e_comp->w, e_comp->h);
   evas_gl_make_current(gl, cgl->sfc, cgl->ctx);
   compiz_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   evas_gl_native_surface_get(gl, cgl->sfc, &ns);
   evas_object_image_native_surface_set(cgl->obj, &ns);
   e_comp_object_blank(ec->frame, 1);
}

EINTERN void
compiz_texture_bind(CompTexture *texture)
{
   Compiz_GL *cgl;

   cgl = eina_hash_find(textures, &texture);
   compiz_glapi->glBindFramebuffer(GL_FRAMEBUFFER, cgl->fbo);
   compiz_glapi->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->name, 0);
   compiz_glapi->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

EINTERN void
compiz_texture_to_win(CompTexture *texture, CompWindow *w)
{
   eina_hash_set(wintextures, &texture, w);
}

EINTERN void
compiz_texture_del(CompTexture *texture)
{
   Compiz_GL *cgl;

   cgl = eina_hash_set(textures, &texture, NULL);
   if (!cgl) return; //wtf
   evas_object_event_callback_del_full(cgl->ec->frame, EVAS_CALLBACK_SHOW, (void*)cshow, cgl->obj);
   evas_object_event_callback_del_full(cgl->ec->frame, EVAS_CALLBACK_HIDE, (void*)chide, cgl->obj);
   evas_object_smart_callback_del_full(cgl->ec->frame, "shaded", (void*)chide, cgl->obj);
   evas_object_smart_callback_del_full(cgl->ec->frame, "shade_done", cshade, cgl);
   evas_object_smart_callback_del_full(cgl->ec->frame, "unshading", (void*)cshow, cgl->obj);
   evas_object_smart_callback_del_full(cgl->ec->frame, "unshaded", (void*)cshow, cgl->obj);
   evas_object_smart_callback_del_full(cgl->ec->frame, "dirty", cdirty, cgl);
   evas_gl_make_current(gl, cgl->sfc, cgl->ctx);
   compiz_glapi->glDeleteFramebuffers(1, &cgl->fbo);
   evas_gl_surface_destroy(gl, cgl->sfc);
   evas_gl_context_destroy(gl, cgl->ctx);
   evas_object_del(cgl->obj);
   e_comp_object_blank(cgl->ec->frame, 0);
   eina_tiler_free(cgl->damage);
   free(cgl);
}

static Eina_Bool
compiz_mouse_move(void *data EINA_UNUSED, int type EINA_UNUSED, Ecore_Event_Mouse_Move *ev)
{
   XEvent *event;

   event = calloc(1, sizeof(XEvent));
   lastPointerX = pointerY;
   lastPointerY = pointerX;
   pointerX = ev->root.x;
   pointerY = ev->root.y;

   event->type = MotionNotify;
   event->xmotion.display = ecore_x_display_get();
   event->xmotion.window = ev->window;
   event->xmotion.root = ev->root_window;
   event->xmotion.subwindow = ev->event_window;
   event->xmotion.time = ev->timestamp;
   event->xmotion.x = ev->x;
   event->xmotion.y = ev->y;
   event->xmotion.x_root = ev->root.x;
   event->xmotion.y_root = ev->root.y;
   event->xmotion.same_screen = ev->same_screen;
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

static void
compiz_getOutputExtentsForWindow(CompWindow *w, CompWindowExtents *output)
{
   E_Client *ec;

   getOutputExtentsForWindow(w, output);
   ec = compiz_win_to_client(w);
   e_comp_object_frame_geometry_get(ec->frame, &output->left, &output->right, &output->top, &output->bottom);
}

static void
compiz_gl_init(void)
{
   CompDisplay *d;
   CompScreen *s;

   assert(!compiz_main());
   for (d = core.displays; d; d = d->next)
     for (s = d->screens; s; s = s->next)
       s->getOutputExtentsForWindow = compiz_getOutputExtentsForWindow;
}

static void
compiz_gl_render(void)
{
   struct timeval tv;

   if (!init)
     {
        compiz_gl_init();
        init = EINA_TRUE;
     }
   gettimeofday (&tv, 0);
   handleTimeouts(&tv);
   compiz_timers();
   fprintf(stderr, "COMPIZ LOOP\n");
   eventLoop();
   compiz_damage();
}

static void
compiz_resize_begin(void *d EINA_UNUSED, E_Client *ec)
{
   CompWindow *w;
   unsigned int mask;

   w = eina_hash_find(clients, &ec);
   resizegrab = pushScreenGrab(w->screen, 0, "resize");
   mask = CompWindowGrabResizeMask | CompWindowGrabButtonMask;
   w->screen->windowGrabNotify(w, ec->moveinfo.down.mx, ec->moveinfo.down.my, 0, mask);
}

static void
compiz_resize_end(void *d EINA_UNUSED, E_Client *ec)
{
   CompWindow *w;

   w = eina_hash_find(clients, &ec);
   removeScreenGrab(w->screen, resizegrab, NULL);
   resizegrab = -1;
   w->screen->windowUngrabNotify(w);
}

static void
compiz_move_begin(void *d EINA_UNUSED, E_Client *ec)
{
   CompWindow *w;
   unsigned int mask;

   w = eina_hash_find(clients, &ec);
   if (w->grabbed)
     w->screen->windowUngrabNotify(w);
   movegrab = pushScreenGrab(w->screen, 0, "move");
   mask = CompWindowGrabMoveMask | CompWindowGrabButtonMask;
   w->screen->windowGrabNotify(w, ec->moveinfo.down.mx, ec->moveinfo.down.my, 0, mask);
   addWindowDamage(w);
   ecx = ec->client.x;
   ecy = ec->client.y;
   fprintf(stderr, "MOVE START: %d,%d\n", ecx, ecy);
   fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
}

static void
compiz_move_update(void *d EINA_UNUSED, E_Client *ec)
{
   CompWindow *w;
   unsigned int mask;

   w = eina_hash_find(clients, &ec);
   moveWindow(w,
              ec->client.x - ecx, ec->client.y - ecy,
              TRUE, FALSE);
   ecx = ec->client.x;
   ecy = ec->client.y;
   syncWindowPosition(w);
   fprintf(stderr, "MOVE UPDATE: %d,%d\n", ec->client.x, ec->client.y);
   fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
}

static void
compiz_move_end(void *d EINA_UNUSED, E_Client *ec)
{
   CompWindow *w;

   w = eina_hash_find(clients, &ec);
   moveWindow(w,
              -1, -1,
              TRUE, FALSE);
   moveWindow(w,
              1, 1,
              TRUE, FALSE);
   removeScreenGrab(w->screen, movegrab, NULL);
   movegrab = -1;
   addWindowDamage(w);
   fprintf(stderr, "MOVE END: %d,%d\n", ec->client.x, ec->client.y);
   fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
}

static void
gl_fn_init(void)
{
#define GLFN(FN) \
   if (!compiz_glapi->FN) \
     compiz_glapi->FN = evas_gl_proc_address_get(gl, #FN)
   GLFN(glLightModelfv);
   GLFN(glLightfv);
   GLFN(glNormal3f);
   GLFN(glPushMatrix);
   GLFN(glPopMatrix);
   GLFN(glMatrixMode);
   GLFN(glLoadMatrixf);
   GLFN(glLoadIdentity);
   GLFN(glMultMatrixf);
   GLFN(glTexEnvi);
   GLFN(glColor4f);
   GLFN(glVertexPointer);
   GLFN(glEnableClientState);
   GLFN(glDisableClientState);
   GLFN(glTexCoordPointer);
   GLFN(glFrustumf);
#undef GLFN  
}

EINTERN void
compiz_init(void)
{
   glcfg = evas_gl_config_new();
   glcfg->color_format = EVAS_GL_RGBA_8888;
   glcfg->gles_version = EVAS_GL_GLES_2_X;
   glcfg->depth_bits = EVAS_GL_DEPTH_BIT_24;
   glcfg->stencil_bits = EVAS_GL_STENCIL_BIT_8;
   gl = evas_gl_new(e_comp->evas);
   compiz_glapi = evas_gl_api_get(gl);
   gl_fn_init();
   wins = eina_hash_pointer_new((Eina_Free_Cb)win_hash_free);
   clients = eina_hash_pointer_new(NULL);
   textures = eina_hash_pointer_new(NULL);
   wintextures = eina_hash_pointer_new(NULL);
   //evas_event_callback_add(e_comp->evas, EVAS_CALLBACK_RENDER_PRE, (Evas_Event_Cb)compiz_render_pre, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_MOVE, compiz_mouse_move, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_ADD, compiz_client_add, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_REMOVE, compiz_client_del, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_SHOW, compiz_client_show, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_HIDE, compiz_client_hide, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_MOVE, compiz_client_configure, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_RESIZE, compiz_client_configure, NULL);
   hooks = eina_list_append(hooks, e_client_hook_add(E_CLIENT_HOOK_MOVE_BEGIN, compiz_move_begin, NULL));
   hooks = eina_list_append(hooks, e_client_hook_add(E_CLIENT_HOOK_MOVE_UPDATE, compiz_move_update, NULL));
   hooks = eina_list_append(hooks, e_client_hook_add(E_CLIENT_HOOK_MOVE_END, compiz_move_end, NULL));
   hooks = eina_list_append(hooks, e_client_hook_add(E_CLIENT_HOOK_RESIZE_BEGIN, compiz_resize_begin, NULL));
   hooks = eina_list_append(hooks, e_client_hook_add(E_CLIENT_HOOK_RESIZE_END, compiz_resize_end, NULL));
   events = eina_array_new(1);
   e_comp->pre_render_cbs = eina_list_append(e_comp->pre_render_cbs, compiz_gl_render);
   compiz_anim = ecore_animator_add(compiz_idle_cb, NULL);
   ecore_animator_freeze(compiz_anim);
}

EINTERN void
compiz_shutdown(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
   E_FREE_LIST(hooks, e_client_hook_del);
   E_FREE_FUNC(compiz_anim, ecore_animator_del);
   E_FREE_FUNC(compiz_timer, ecore_timer_del);
   compiz_fini();
   E_FREE_FUNC(gl, evas_gl_free);
   E_FREE_FUNC(glcfg, evas_gl_config_free);
   e_comp->pre_render_cbs = eina_list_remove(e_comp->pre_render_cbs, compiz_gl_render);
   E_FREE_FUNC(event_job, ecore_job_del);
   eina_hash_free(wins);
   eina_hash_free(clients);
   eina_hash_free(textures);
   eina_hash_free(wintextures);
   init = EINA_FALSE;
}
