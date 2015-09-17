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
static Eina_Tiler *tiler;

static Eina_Bool init = EINA_FALSE;

static Eina_Bool doclear = EINA_FALSE;
EINTERN Eina_Bool noclear = EINA_FALSE;

E_API int pointerX = 0;
E_API int pointerY = 0;
E_API int lastPointerX = 0;
E_API int lastPointerY = 0;

typedef struct
{
   Evas_GL_Surface *sfc;
   GLuint fbo;
   E_Client *ec;
   Evas_Object *obj;
   Eina_Bool clear : 1;
} Compiz_GL;
EINTERN Evas_GL *gl = NULL;
static Evas_GL_Config *glcfg = NULL;
static Evas_GL_Context *glctx = NULL;
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
                 if (w->grabbed && ((!ec->moving) && (!e_client_util_resizing_get(ec))))
                   w->screen->windowUngrabNotify(w);
                 if ((w->attrib.x == ec->client.x) && (w->attrib.y == ec->client.y))
                   continue;
                 moveWindow(w, ec->client.x - w->attrib.x, ec->client.y - w->attrib.y, TRUE, TRUE);
                 syncWindowPosition(w);
                 compiz_texture_clear(w->texture);
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
   CompWindow *w;

   w = eina_hash_find(clients, &ec);
   if (w)
     updateWindowOutputExtents(w);
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
   Eina_Iterator *it;
   XEvent *ev;

   it = eina_array_iterator_new(events);
   EINA_ITERATOR_FOREACH(it, ev)
     {
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
   eina_iterator_free(it);
   eina_array_clean(events);
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
compwindow_update(E_Client *ec)
{
   CompWindow *w;

   w = eina_hash_find(clients, &ec);
   if (w)
     w->id = window_get(ec);
}

static void
mapnotify(E_Client *ec)
{
   XEvent *event;

   compwindow_update(ec);
   event = calloc(1, sizeof(XEvent));
   event->type = MapNotify;
   event->xmap.display = ecore_x_display_get();
   event->xmap.event = event->xmap.window = window_get(ec);
   event->xmap.override_redirect = ec->override;
   compiz_job(event);
}

static void
configurenotify(E_Client *ec)
{
   XEvent *event;
   E_Client *bec;
   int w, h;

   compwindow_update(ec);
   event = calloc(1, sizeof(XEvent));
   event->type = ConfigureNotify;
   event->xconfigure.display = ecore_x_display_get();
   event->xconfigure.event = event->xconfigure.window = window_get(ec);
   /* catch shading */
   e_comp_object_frame_xy_unadjust(ec->frame, ec->x, ec->y, &event->xconfigure.x, &event->xconfigure.y);
   e_comp_object_frame_wh_unadjust(ec->frame, ec->w, ec->h, &event->xconfigure.width, &event->xconfigure.height);
   bec = e_client_below_get(ec);
   if (bec)
     {
        compwindow_update(bec);
        event->xconfigure.above = window_get(bec);
     }
   event->xconfigure.override_redirect = ec->override;
   compiz_job(event);
}

static void
maprequest(E_Client *ec)
{
   XEvent *event;

   event = calloc(1, sizeof(XEvent));
   event->type = MapRequest;
   event->xmaprequest.display = ecore_x_display_get();
   if (ec->parent)
     event->xmaprequest.parent = window_get(ec->parent);
   else
     event->xmaprequest.parent = e_comp->root;
   event->xmaprequest.window = window_get(ec);
   compiz_job(event);
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
   if (evas_object_visible_get(ec->frame))
     maprequest(ec);
}

static void
unmapnotify(E_Client *ec)
{
   XEvent *event;

   event = calloc(1, sizeof(XEvent));
   event->type = UnmapNotify;
   event->xunmap.display = ecore_x_display_get();
   event->xunmap.event = event->xunmap.window = window_get(ec);
   compiz_job(event);
}

static Eina_Bool
compiz_client_add(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
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
   compiz_job(event);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_configure(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   configurenotify(ev->ec);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
compiz_client_show(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   mapnotify(ev->ec);
   return ECORE_CALLBACK_RENEW;
}

static void
compiz_client_hide(void *data, ...)
{
   XEvent *event;
   E_Client *ec = data;

   compwindow_update(ec);
   if (ec->desk && (!ec->desk->visible)) return;
   if ((!ec->iconic) || evas_object_visible_get(ec->frame))
     unmapnotify(ec);
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
compiz_client_stack(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   configurenotify(data);
}

static void
compiz_client_damage(void *data, Evas_Object *obj, void *event_info)
{
   CompWindow *w = data;
   E_Client *ec;
   Eina_Rectangle *r = event_info;
   XEvent *event;
   XDamageNotifyEvent *de;

   ec = e_comp_object_client_get(obj);
   compwindow_update(ec);
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
compiz_client_maximize(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   CompWindow *w = data;
   E_Client *ec = e_comp_object_client_get(obj);

   if (ec->maximized) //unmaximizing
     changeWindowState(w, w->state &= ~MAXIMIZE_STATE);
   else
     changeWindowState(w, w->state |= MAXIMIZE_STATE);
   updateWindowAttributes(w, CompStackingUpdateModeNone);
}

static void
compiz_client_shade(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   CompWindow *w = data;
   E_Client *ec = e_comp_object_client_get(obj);
   unsigned int state;

   state = w->state;
   if (ec->shaded)
     state &= ~CompWindowStateShadedMask;
   else
     state |= CompWindowStateShadedMask;
   if (w->state == state) return;
   changeWindowState(w, state);
   updateWindowAttributes(w, CompStackingUpdateModeNone);
   if (ec->shaded)
     mapnotify(ec);
   else
     unmapnotify(ec);
}

static Eina_Bool
compiz_client_iconify(void *d EINA_UNUSED, int t EINA_UNUSED, E_Event_Client *ev)
{
   CompWindow *w;

   w = eina_hash_find(clients, &ev->ec);
   if (!w) return ECORE_CALLBACK_RENEW;
   if (ev->ec->iconic)
     minimizeWindow(w);
   else
     {
        unminimizeWindow(w);
        compiz_client_damage(w, ev->ec->frame, &(Eina_Rectangle){0, 0, ev->ec->client.w, ev->ec->client.h});
     }
   return ECORE_CALLBACK_RENEW;
}

EINTERN void
compiz_win_hash_client(CompWindow *w, E_Client *ec)
{

   eina_hash_set(wins, &w, ec);
   eina_hash_set(clients, &ec, w);
   evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_RESTACK, compiz_client_stack, ec);
   evas_object_smart_callback_add(ec->frame, "damage", compiz_client_damage, w);
   evas_object_smart_callback_add(ec->frame, "maximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_add(ec->frame, "unmaximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_add(ec->frame, "shading", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "shaded", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "unshading", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "unshaded", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "shade_done", compiz_client_shade, w);
   evas_object_smart_callback_add(ec->frame, "frame_recalc_done", compiz_client_frame_calc, ec);
   evas_object_smart_callback_add(ec->frame, "hiding", (void*)compiz_client_hide, ec);
   evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_HIDE, (void*)compiz_client_hide, ec);
   configurenotify(ec);
   ecore_job_add((Ecore_Cb)compiz_idle_cb, NULL);
}

EINTERN void
compiz_win_hash_del(CompWindow *w)
{
   eina_hash_del_by_key(wins, &w);
}

EINTERN void
compiz_texture_activate(CompTexture *texture, Eina_Bool set, Region region)
{
   Compiz_GL *cgl;
   Eina_Iterator *it;
   Eina_Rectangle *r;
   long i;

   if (!set)
     compiz_glapi->glFinish();
   if (!set) return;
   cgl = eina_hash_find(textures, &texture);
   if (!cgl) return;
   evas_gl_make_current(gl, cgl->sfc, glctx);
   if (!region) return;
   compiz_glapi->glViewport(0, 0, e_comp->w, e_comp->h);
   for (i = 0; i < region->numRects; i++)
     evas_object_image_data_update_add(cgl->obj, region->rects[i].x1, region->rects[i].y1, region->rects[i].x2 - region->rects[i].x1, region->rects[i].y2 - region->rects[i].y1);
   if (cgl->clear)
     compiz_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   cgl->clear = 0;
}


EINTERN void
compiz_texture_clear(CompTexture *texture)
{
   Compiz_GL *cgl;

   if (noclear) return;
   cgl = eina_hash_find(textures, &texture);
   if (cgl && ((intptr_t)cgl != 0x1)) cgl->clear = 1;
   else eina_hash_set(textures, &texture, (void*)0x1);
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
   if ((intptr_t)cgl == 0x1)
     cgl = NULL;
   if (cgl) return;
     {
        cgl = malloc(sizeof(Compiz_GL));
        cgl->ec = ec;
        compiz_client_frame_recalc(ec);
        cgl->obj = evas_object_image_filled_add(e_comp->evas);
        evas_object_name_set(cgl->obj, "compiz!");
        evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_SHOW, (void*)cshow, cgl->obj);
        evas_object_event_callback_add(ec->frame, EVAS_CALLBACK_HIDE, (void*)chide, cgl->obj);
        //evas_object_smart_callback_add(ec->frame, "shaded", (void*)chide, cgl->obj);
        //evas_object_smart_callback_add(ec->frame, "unshading", (void*)cshow, cgl->obj);
        //evas_object_smart_callback_add(ec->frame, "unshaded", (void*)cshow, cgl->obj);
        evas_object_image_data_update_add(cgl->obj, 0, 0, e_comp->w, e_comp->h);
        evas_object_image_alpha_set(cgl->obj, 1);
        evas_object_pass_events_set(cgl->obj, 1);
        evas_object_smart_member_add(cgl->obj, ec->frame);
        //evas_object_layer_set(cgl->obj, E_LAYER_CLIENT_ABOVE);
        if (evas_object_visible_get(ec->frame))
          evas_object_show(cgl->obj);
        evas_gl_make_current(gl, NULL, glctx);
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
        eina_hash_set(textures, &texture, cgl);
        if (window_get(ec) != e_client_util_win_get(ec))
          win->id = window_get(ec);
     }
   compiz_render_resize(cgl);
   cgl->sfc = evas_gl_surface_create(gl, glcfg, e_comp->w, e_comp->h);
   evas_gl_make_current(gl, cgl->sfc, glctx);
   compiz_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   evas_gl_native_surface_get(gl, cgl->sfc, &ns);
   evas_object_image_native_surface_set(cgl->obj, &ns);
   e_comp_object_blank(ec->frame, 1);
}

EINTERN void
compiz_texture_bind(CompTexture *texture)
{
   Compiz_GL *cgl;
   Evas_Native_Surface ns;

   cgl = eina_hash_find(textures, &texture);
   EINA_SAFETY_ON_NULL_RETURN(cgl);
   ns.type = EVAS_NATIVE_SURFACE_OPENGL;
   ns.data.opengl.framebuffer_id = cgl->fbo;
   ns.data.opengl.texture_id = texture->name;
   ns.data.opengl.internal_format = GL_RGBA;
   ns.data.opengl.format = GL_BGRA_EXT;
   ns.data.opengl.x = ns.data.opengl.y = 0;
   ns.data.opengl.w = cgl->ec->client.w;
   ns.data.opengl.h = cgl->ec->client.h;
   compiz_glapi->glBindFramebuffer(GL_FRAMEBUFFER, cgl->fbo);
   compiz_glapi->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->name, 0);
   compiz_glapi->glBindFramebuffer(GL_FRAMEBUFFER, 0);
   e_comp_object_native_surface_override(cgl->ec->frame, &ns);
   //e_comp_object_damage(cgl->ec->frame, 0, 0, cgl->ec->w, cgl->ec->h);
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
   if ((!cgl) || ((intptr_t)cgl == 0x1)) return;
   evas_object_event_callback_del_full(cgl->ec->frame, EVAS_CALLBACK_SHOW, (void*)cshow, cgl->obj);
   evas_object_event_callback_del_full(cgl->ec->frame, EVAS_CALLBACK_HIDE, (void*)chide, cgl->obj);

   evas_gl_make_current(gl, cgl->sfc, glctx);
   compiz_glapi->glDeleteFramebuffers(1, &cgl->fbo);
   evas_gl_surface_destroy(gl, cgl->sfc);
   if (!e_object_is_del(E_OBJECT(cgl->ec)))
     {
        e_comp_object_native_surface_override(cgl->ec->frame, NULL);
        e_comp_object_blank(cgl->ec->frame, 0);
     }

   evas_object_del(cgl->obj);
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
   E_Client *ec;

   glctx = evas_gl_context_create(gl, NULL);
   assert(!compiz_main());
   for (d = core.displays; d; d = d->next)
     for (s = d->screens; s; s = s->next)
       s->getOutputExtentsForWindow = compiz_getOutputExtentsForWindow;
   E_CLIENT_FOREACH(ec)
     e_object_ref(E_OBJECT(ec));
   e_comp_shape_queue();
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
   compiz_idle_cb(NULL);
   gettimeofday (&tv, 0);
   handleTimeouts(&tv);
   compiz_timers();
   //fprintf(stderr, "COMPIZ LOOP\n");
   eventLoop();
   eina_tiler_clear(tiler);
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
   //fprintf(stderr, "MOVE START: %d,%d\n", ecx, ecy);
   //fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
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
   //fprintf(stderr, "MOVE UPDATE: %d,%d\n", ec->client.x, ec->client.y);
   //fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
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
   if ((ec->x == ec->moveinfo.down.x) && (ec->y == ec->moveinfo.down.y))
     w->screen->windowUngrabNotify(w);
   //fprintf(stderr, "MOVE END: %d,%d\n", ec->client.x, ec->client.y);
   //fprintf(stderr, "**COMPIZ: %d,%d\n", w->attrib.x, w->attrib.y);
}

EINTERN void
compiz_damage_mask(unsigned int mask)
{
   doclear = mask & COMP_SCREEN_DAMAGE_ALL_MASK;
}

static void
win_hash_free(E_Client *ec)
{
   CompWindow *w;

   w = eina_hash_set(clients, &ec, NULL);
   evas_object_event_callback_del_full(ec->frame, EVAS_CALLBACK_RESTACK, compiz_client_stack, ec);
   evas_object_smart_callback_del_full(ec->frame, "damage", compiz_client_damage, w);
   evas_object_smart_callback_del_full(ec->frame, "maximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_del_full(ec->frame, "unmaximize_pre", compiz_client_maximize, w);
   evas_object_smart_callback_del_full(ec->frame, "shading", compiz_client_shade, w);
   evas_object_smart_callback_del_full(ec->frame, "shaded", compiz_client_shade, w);
   evas_object_smart_callback_del_full(ec->frame, "unshading", compiz_client_shade, w);
   evas_object_smart_callback_del_full(ec->frame, "unshaded", compiz_client_shade, w);
   evas_object_smart_callback_del_full(ec->frame, "shade_done", compiz_client_shade, w);
   evas_object_smart_callback_del_full(ec->frame, "frame_recalc_done", compiz_client_frame_calc, ec);
   evas_object_smart_callback_del_full(ec->frame, "hiding", (void*)compiz_client_hide, ec);
   evas_object_event_callback_del_full(ec->frame, EVAS_CALLBACK_HIDE, (void*)compiz_client_hide, ec);
   e_object_unref(E_OBJECT(ec));
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

static Eina_Bool
compiz_desk_show(void *data EINA_UNUSED, int t EINA_UNUSED, E_Event_Desk_Before_Show *ev)
{
   E_Desk *desk;
   CompDisplay *d;
   CompScreen *s;

   desk = e_desk_current_get(ev->desk->zone);
   for (d = core.displays; d; d = d->next)
     for (s = d->screens; s; s = s->next)
       {
          if (s->screenNum != ev->desk->zone->num) continue;
          moveScreenViewport(s, desk->x - ev->desk->x, desk->y - ev->desk->y, 1);
          return ECORE_CALLBACK_RENEW;
       }
   return ECORE_CALLBACK_RENEW;
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
   tiler = eina_tiler_new(e_comp->w, e_comp->h);
   //evas_event_callback_add(e_comp->evas, EVAS_CALLBACK_RENDER_PRE, (Evas_Event_Cb)compiz_render_pre, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_MOVE, compiz_mouse_move, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_ADD, compiz_client_add, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_REMOVE, compiz_client_del, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_SHOW, compiz_client_show, NULL);
   //E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_HIDE, compiz_client_hide, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_MOVE, compiz_client_configure, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_RESIZE, compiz_client_configure, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_DESK_BEFORE_SHOW, compiz_desk_show, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_ICONIFY, compiz_client_iconify, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CLIENT_UNICONIFY, compiz_client_iconify, NULL);
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
   evas_gl_context_destroy(gl, glctx);
   glctx = NULL;
   E_FREE_FUNC(gl, evas_gl_free);
   E_FREE_FUNC(glcfg, evas_gl_config_free);
   e_comp->pre_render_cbs = eina_list_remove(e_comp->pre_render_cbs, compiz_gl_render);
   E_FREE_FUNC(event_job, ecore_job_del);
   eina_hash_free(wins);
   eina_hash_free(clients);
   eina_hash_free(textures);
   eina_hash_free(wintextures);
   while (eina_array_count(events))
     free(eina_array_pop(events));
   E_FREE_FUNC(tiler, eina_tiler_free);
   E_FREE_FUNC(events, eina_array_free);
   init = EINA_FALSE;
   if (!stopping)
     {
        E_Action *a;

        a = e_action_find("restart");
        if ((a) && (a->func.go)) a->func.go(NULL, NULL);
     }
}
