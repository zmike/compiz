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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <compiz-core.h>

char *programName;
char **programArgv;
int programArgc;

char **initialPlugins = NULL;
int nInitialPlugins = 0;

char *backgroundImage = NULL;

REGION emptyRegion;
REGION infiniteRegion;
GLushort defaultColor[4] = { 0xffff, 0xffff, 0xffff, 0xffff };
Window currentRoot = 0;

int defaultRefreshRate = 50;
char *defaultTextureFilter = "Good";

Bool shutDown = FALSE;
Bool restartSignal = FALSE;
Bool coreInitialized = FALSE;

CompWindow *lastFoundWindow = 0;
CompWindow *lastDamagedWindow = 0;

Bool replaceCurrentWm = FALSE;
Bool indirectRendering = FALSE;
Bool strictBinding = TRUE;
Bool noDetection = FALSE;
Bool useDesktopHints = FALSE;
Bool onlyCurrentScreen = FALSE;
Bool noFBO = FALSE;
static Bool debugOutput = FALSE;

#ifdef USE_COW
Bool useCow = TRUE;
#endif

CompMetadata coreMetadata;

static void
usage(void)
{
   printf("Usage: %s\n       "
          "[--display DISPLAY] "
          "[--bg-image PNG] "
          "[--refresh-rate RATE]\n       "
          "[--fast-filter] "
          "[--indirect-rendering] "
          "[--no-detection]\n       "
          "[--keep-desktop-hints] "
          "[--loose-binding] "
          "[--replace]\n       "
          "[--no-fbo] "
          "[--sm-disable] "
          "[--sm-client-id ID] "
          "[--only-current-screen]\n      "

#ifdef USE_COW
          " [--use-root-window] "
#endif

          "[--debug] "
          "[--version] "
          "[--help] "
          "[PLUGIN]...\n",
          programName);
}

void
compLogMessage(const char *componentName,
               CompLogLevel level,
               const char *format,
               ...)
{
   va_list args;
   char message[2048];

   va_start(args, format);

   vsnprintf(message, 2048, format, args);

   if (coreInitialized)
     (*core.logMessage)(componentName, level, message);
   else
     logMessage(componentName, level, message);

   va_end(args);
}

void
logMessage(const char *componentName,
           CompLogLevel level,
           const char *message)
{
   if (!debugOutput && level >= CompLogLevelDebug)
     return;

   fprintf(stderr, "%s (%s) - %s: %s\n",
           programName, componentName,
           logLevelToString(level), message);
}

const char *
logLevelToString(CompLogLevel level)
{
   switch (level)
     {
      case CompLogLevelFatal:
        return "Fatal";

      case CompLogLevelError:
        return "Error";

      case CompLogLevelWarn:
        return "Warn";

      case CompLogLevelInfo:
        return "Info";

      case CompLogLevelDebug:
        return "Debug";

      default:
        break;
     }

   return "Unknown";
}

static void
signalHandler(int sig)
{
   int status;

   switch (sig)
     {
      case SIGCHLD:
        waitpid(-1, &status, WNOHANG | WUNTRACED);
        break;

      case SIGHUP:
        restartSignal = TRUE;
        break;

      case SIGINT:
      case SIGTERM:
        shutDown = TRUE;

      default:
        break;
     }
}

typedef struct _CompIOCtx
{
   int   offset;
   char *pluginData;
   char *textureFilterData;
   char *refreshRateData;
} CompIOCtx;

static int
readCoreXmlCallback(void *context,
                    char *buffer,
                    int length)
{
   CompIOCtx *ctx = (CompIOCtx *)context;
   int offset = ctx->offset;
   int i, j;

   i = compReadXmlChunk("<compiz><core><display>", &offset, buffer, length);

   for (j = 0; j < COMP_DISPLAY_OPTION_NUM; j++)
     {
        CompMetadataOptionInfo info = coreDisplayOptionInfo[j];

        switch (j)
          {
           case COMP_DISPLAY_OPTION_ACTIVE_PLUGINS:
             if (ctx->pluginData)
               info.data = ctx->pluginData;
             break;

           case COMP_DISPLAY_OPTION_TEXTURE_FILTER:
             if (ctx->textureFilterData)
               info.data = ctx->textureFilterData;

           default:
             break;
          }

        i += compReadXmlChunkFromMetadataOptionInfo(&info,
                                                    &offset,
                                                    buffer + i,
                                                    length - i);
     }

   i += compReadXmlChunk("</display><screen>", &offset,
                         buffer + i, length - 1);

   for (j = 0; j < COMP_SCREEN_OPTION_NUM; j++)
     {
        CompMetadataOptionInfo info = coreScreenOptionInfo[j];

        switch (j)
          {
           case COMP_SCREEN_OPTION_REFRESH_RATE:
             if (ctx->refreshRateData)
               info.data = ctx->refreshRateData;

           default:
             break;
          }

        i += compReadXmlChunkFromMetadataOptionInfo(&info,
                                                    &offset,
                                                    buffer + i,
                                                    length - i);
     }

   i += compReadXmlChunk("</screen></core></compiz>", &offset, buffer + i,
                         length - i);

   if (!offset && length > i)
     buffer[i++] = '\0';

   ctx->offset += i;

   return i;
}

EINTERN int
compiz_main(void)
{
   CompIOCtx ctx;
   char *displayName = 0;
   char *plugin[256];
   int i, nPlugin = 0;
   Bool disableSm = FALSE;
   char *clientId = NULL;
   char *refreshRateArg = NULL;

   ecore_app_args_get(&programArgc, &programArgv);
   programName = programArgv[0];

   emptyRegion.rects = &emptyRegion.extents;
   emptyRegion.numRects = 0;
   emptyRegion.extents.x1 = 0;
   emptyRegion.extents.y1 = 0;
   emptyRegion.extents.x2 = 0;
   emptyRegion.extents.y2 = 0;
   emptyRegion.size = 0;

   infiniteRegion.rects = &infiniteRegion.extents;
   infiniteRegion.numRects = 1;
   infiniteRegion.extents.x1 = MINSHORT;
   infiniteRegion.extents.y1 = MINSHORT;
   infiniteRegion.extents.x2 = MAXSHORT;
   infiniteRegion.extents.y2 = MAXSHORT;

   memset(&ctx, 0, sizeof (ctx));

   if (!clientId)
     {
        clientId = getenv("DESKTOP_AUTOSTART_ID");
     }

   if (refreshRateArg)
     {
        ctx.refreshRateData = malloc(strlen(refreshRateArg) + 256);
        if (ctx.refreshRateData)
          sprintf(ctx.refreshRateData,
                  "<min>1</min><default>%s</default>",
                  refreshRateArg);
     }

   plugin[nPlugin++] = "ccp";
   if (nPlugin)
     {
        int size = 256;

        for (i = 0; i < nPlugin; i++)
          size += strlen(plugin[i]) + 16;

        ctx.pluginData = malloc(size);
        if (ctx.pluginData)
          {
             char *ptr = ctx.pluginData;

             ptr += sprintf(ptr, "<type>string</type><default>");

             for (i = 0; i < nPlugin; i++)
               ptr += sprintf(ptr, "<value>%s</value>", plugin[i]);

             ptr += sprintf(ptr, "</default>");
          }

        initialPlugins = malloc(nPlugin * sizeof (char *));
        if (initialPlugins)
          {
             memcpy(initialPlugins, plugin, nPlugin * sizeof (char *));
             nInitialPlugins = nPlugin;
          }
        else
          {
             nInitialPlugins = 0;
          }
     }

   xmlInitParser();

   LIBXML_TEST_VERSION;

   if (!compInitMetadata(&coreMetadata))
     {
        compLogMessage("core", CompLogLevelFatal,
                       "Couldn't initialize core metadata");
        return 1;
     }

   if (!compAddMetadataFromIO(&coreMetadata,
                              readCoreXmlCallback, NULL,
                              &ctx))
     return 1;

   if (ctx.refreshRateData)
     free(ctx.refreshRateData);

   if (ctx.pluginData)
     free(ctx.pluginData);

   compAddMetadataFromFile(&coreMetadata, "core");

   if (!initCore())
     return 1;

   coreInitialized = TRUE;

   if (!addDisplay(displayName))
     return 1;
   return 0;
}

EINTERN void
compiz_fini(void)
{
   coreInitialized = FALSE;

   finiCore();
   compFiniMetadata(&coreMetadata);

   xmlCleanupParser();

   if (initialPlugins)
     free(initialPlugins);
   memset(&core, 0, sizeof(core));
}

