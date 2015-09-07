#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <e.h>
#ifdef HAVE_JWZGLES
#include "jwzgles.h"
#endif
#ifdef ENABLE_NLS
# include <libintl.h>
# define D_(str) dgettext(PACKAGE, str)
# define DP_(str, str_p, n) dngettext(PACKAGE, str, str_p, n)
#else
# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,codeset)
# define D_(str) (str)
# define DP_(str, str_p, n) (str_p)
#endif

#define N_(str) (str)
#include <compiz-core.h>
#define MOD_CONFIG_FILE_EPOCH 0
#define MOD_CONFIG_FILE_GENERATION 1
#define MOD_CONFIG_FILE_VERSION ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

#ifndef GL_TEXTURE_RECTANGLE_NV
# define GL_TEXTURE_RECTANGLE_NV 0x84F5
#endif
#ifndef GL_BGRA
# define GL_BGRA 0x80E1
#endif
#ifndef GL_RGBA4
# define GL_RGBA4 0x8056
#endif
#ifndef GL_RGBA8
# define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA12
# define GL_RGBA12 0x805A
#endif
#ifndef GL_RGBA16
# define GL_RGBA16 0x805B
#endif
#ifndef GL_R3_G3_B2
# define GL_R3_G3_B2 0x2A10
#endif
#ifndef GL_RGB4
# define GL_RGB4 0x804F
#endif
#ifndef GL_RGB5
# define GL_RGB5 0x8050
#endif
#ifndef GL_RGB8
# define GL_RGB8 0x8051
#endif
#ifndef GL_RGB10
# define GL_RGB10 0x8052
#endif
#ifndef GL_RGB12
# define GL_RGB12 0x8053
#endif
#ifndef GL_RGB16
# define GL_RGB16 0x8054
#endif
#ifndef GL_ALPHA4
# define GL_ALPHA4 0x803B
#endif
#ifndef GL_ALPHA8
# define GL_ALPHA8 0x803C
#endif
#ifndef GL_ALPHA12
# define GL_ALPHA12 0x803D
#endif
#ifndef GL_ALPHA16
# define GL_ALPHA16 0x803E
#endif
#ifndef GL_LUMINANCE4
# define GL_LUMINANCE4 0x803F
#endif
#ifndef GL_LUMINANCE8
# define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_LUMINANCE12
# define GL_LUMINANCE12 0x8041
#endif
#ifndef GL_LUMINANCE16
# define GL_LUMINANCE16 0x8042
#endif
#ifndef GL_LUMINANCE4_ALPHA4
# define GL_LUMINANCE4_ALPHA4 0x8043
#endif
#ifndef GL_LUMINANCE8_ALPHA8
# define GL_LUMINANCE8_ALPHA8 0x8045
#endif
#ifndef GL_LUMINANCE12_ALPHA12
# define GL_LUMINANCE12_ALPHA12 0x8047
#endif
#ifndef GL_LUMINANCE16_ALPHA16
# define GL_LUMINANCE16_ALPHA16 0x8048
#endif
#ifndef GL_ETC1_RGB8_OES
# define GL_ETC1_RGB8_OES 0x8D64
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
# define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
# define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif
#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
# define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
# define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
# define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
# define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_TEXTURE_EXTERNAL_OES
# define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif
#ifndef GL_UNPACK_ROW_LENGTH
# define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif
#ifndef EGL_NO_DISPLAY
# define EGL_NO_DISPLAY 0
#endif
#ifndef EGL_NO_CONTEXT
# define EGL_NO_CONTEXT 0
#endif
#ifndef EGL_NONE
# define EGL_NONE 0x3038
#endif
#ifndef EGL_TRUE
# define EGL_TRUE 1
#endif
#ifndef EGL_FALSE
# define EGL_FALSE 0
#endif

#ifndef EGL_MAP_GL_TEXTURE_2D_SEC
# define EGL_MAP_GL_TEXTURE_2D_SEC 0x3201
#endif
#ifndef  EGL_MAP_GL_TEXTURE_HEIGHT_SEC
# define EGL_MAP_GL_TEXTURE_HEIGHT_SEC 0x3202
#endif
#ifndef EGL_MAP_GL_TEXTURE_WIDTH_SEC
# define EGL_MAP_GL_TEXTURE_WIDTH_SEC 0x3203
#endif
#ifndef EGL_MAP_GL_TEXTURE_FORMAT_SEC
# define EGL_MAP_GL_TEXTURE_FORMAT_SEC 0x3204
#endif
#ifndef EGL_MAP_GL_TEXTURE_RGB_SEC
# define EGL_MAP_GL_TEXTURE_RGB_SEC 0x3205
#endif
#ifndef EGL_MAP_GL_TEXTURE_RGBA_SEC
# define EGL_MAP_GL_TEXTURE_RGBA_SEC 0x3206
#endif
#ifndef EGL_MAP_GL_TEXTURE_BGRA_SEC
# define EGL_MAP_GL_TEXTURE_BGRA_SEC 0x3207
#endif
#ifndef EGL_MAP_GL_TEXTURE_LUMINANCE_SEC
# define EGL_MAP_GL_TEXTURE_LUMINANCE_SEC 0x3208
#endif
#ifndef EGL_MAP_GL_TEXTURE_LUMINANCE_ALPHA_SEC
# define EGL_MAP_GL_TEXTURE_LUMINANCE_ALPHA_SEC	0x3209
#endif
#ifndef EGL_MAP_GL_TEXTURE_PIXEL_TYPE_SEC
# define EGL_MAP_GL_TEXTURE_PIXEL_TYPE_SEC 0x320a
#endif
#ifndef EGL_MAP_GL_TEXTURE_UNSIGNED_BYTE_SEC
# define EGL_MAP_GL_TEXTURE_UNSIGNED_BYTE_SEC 0x320b
#endif
#ifndef EGL_MAP_GL_TEXTURE_STRIDE_IN_BYTES_SEC
# define EGL_MAP_GL_TEXTURE_STRIDE_IN_BYTES_SEC 0x320c
#endif
#ifndef EGL_IMAGE_PRESERVED_KHR
# define EGL_IMAGE_PRESERVED_KHR 0x30D2
#endif
#ifndef EGL_NATIVE_SURFACE_TIZEN
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#endif
#ifndef GL_PROGRAM_BINARY_LENGTH
# define GL_PROGRAM_BINARY_LENGTH 0x8741
#endif
#ifndef GL_NUM_PROGRAM_BINARY_FORMATS
# define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#endif
#ifndef GL_PROGRAM_BINARY_FORMATS
# define GL_PROGRAM_BINARY_FORMATS 0x87FF
#endif
#ifndef GL_PROGRAM_BINARY_RETRIEVABLE_HINT
# define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#endif
#ifndef GL_MAX_SAMPLES_IMG
#define GL_MAX_SAMPLES_IMG 0x9135
#endif
#ifndef GL_MAX_SAMPLES_EXT
#define GL_MAX_SAMPLES_EXT 0x8D57
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef EGL_MAP_GL_TEXTURE_DEVICE_CPU_SEC
#define EGL_MAP_GL_TEXTURE_DEVICE_CPU_SEC 1
#endif
#ifndef EGL_MAP_GL_TEXTURE_DEVICE_G2D_SEC
#define EGL_MAP_GL_TEXTURE_DEVICE_G2D_SEC 2
#endif
#ifndef EGL_MAP_GL_TEXTURE_OPTION_READ_SEC
#define EGL_MAP_GL_TEXTURE_OPTION_READ_SEC (1<<0)
#endif
#ifndef EGL_MAP_GL_TEXTURE_OPTION_WRITE_SEC
#define EGL_MAP_GL_TEXTURE_OPTION_WRITE_SEC (1<<1)
#endif
#ifndef EGL_GL_TEXTURE_2D_KHR
#define EGL_GL_TEXTURE_2D_KHR 0x30B1
#endif
#ifndef EGL_GL_TEXTURE_LEVEL_KHR
#define EGL_GL_TEXTURE_LEVEL_KHR 0x30BC
#endif
#ifndef EGL_IMAGE_PRESERVED_KHR
#define EGL_IMAGE_PRESERVED_KHR 0x30D2
#endif
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR            0x00000040
#endif

// Evas_3d require GL_BGR, but that's an extention and will not be necessary once we move to Evas_GL_Image
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

#ifndef GL_COLOR_BUFFER_BIT0_QCOM
// if GL_COLOR_BUFFER_BIT0_QCOM  just assume the rest arent... saves fluff
#define GL_COLOR_BUFFER_BIT0_QCOM                     0x00000001
#define GL_COLOR_BUFFER_BIT1_QCOM                     0x00000002
#define GL_COLOR_BUFFER_BIT2_QCOM                     0x00000004
#define GL_COLOR_BUFFER_BIT3_QCOM                     0x00000008
#define GL_COLOR_BUFFER_BIT4_QCOM                     0x00000010
#define GL_COLOR_BUFFER_BIT5_QCOM                     0x00000020
#define GL_COLOR_BUFFER_BIT6_QCOM                     0x00000040
#define GL_COLOR_BUFFER_BIT7_QCOM                     0x00000080
#define GL_DEPTH_BUFFER_BIT0_QCOM                     0x00000100
#define GL_DEPTH_BUFFER_BIT1_QCOM                     0x00000200
#define GL_DEPTH_BUFFER_BIT2_QCOM                     0x00000400
#define GL_DEPTH_BUFFER_BIT3_QCOM                     0x00000800
#define GL_DEPTH_BUFFER_BIT4_QCOM                     0x00001000
#define GL_DEPTH_BUFFER_BIT5_QCOM                     0x00002000
#define GL_DEPTH_BUFFER_BIT6_QCOM                     0x00004000
#define GL_DEPTH_BUFFER_BIT7_QCOM                     0x00008000
#define GL_STENCIL_BUFFER_BIT0_QCOM                   0x00010000
#define GL_STENCIL_BUFFER_BIT1_QCOM                   0x00020000
#define GL_STENCIL_BUFFER_BIT2_QCOM                   0x00040000
#define GL_STENCIL_BUFFER_BIT3_QCOM                   0x00080000
#define GL_STENCIL_BUFFER_BIT4_QCOM                   0x00100000
#define GL_STENCIL_BUFFER_BIT5_QCOM                   0x00200000
#define GL_STENCIL_BUFFER_BIT6_QCOM                   0x00400000
#define GL_STENCIL_BUFFER_BIT7_QCOM                   0x00800000
#define GL_MULTISAMPLE_BUFFER_BIT0_QCOM               0x01000000
#define GL_MULTISAMPLE_BUFFER_BIT1_QCOM               0x02000000
#define GL_MULTISAMPLE_BUFFER_BIT2_QCOM               0x04000000
#define GL_MULTISAMPLE_BUFFER_BIT3_QCOM               0x08000000
#define GL_MULTISAMPLE_BUFFER_BIT4_QCOM               0x10000000
#define GL_MULTISAMPLE_BUFFER_BIT5_QCOM               0x20000000
#define GL_MULTISAMPLE_BUFFER_BIT6_QCOM               0x40000000
#define GL_MULTISAMPLE_BUFFER_BIT7_QCOM               0x80000000
#endif

#ifndef GL_COMPRESSED_RGBA_ARB
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#define GL_COMPRESSED_RGBA_FXT1_3DFX      0x86B1
#endif

#define DAMAGE_EVENT 0x999 //fake


static inline Ecore_Window
window_get(const E_Client *ec)
{
   Ecore_Window win;

   win = e_client_util_pwin_get(ec);
   if (win) return win;
   return e_client_util_win_get(ec);
}

typedef struct Mod
{
   E_Config_Dialog *cfd;
   E_Module *module;
   Eina_Stringshare *edje_file;
} Mod;

typedef struct Config
{
   unsigned int config_version;
} Config;


extern Mod *compiz_mod;
extern Config *compiz_config;
extern Evas_GL *gl;
extern Evas_GL_API *compiz_glapi;


EINTERN void compiz_init(void);
EINTERN void compiz_shutdown(void);
EINTERN E_Client *compiz_win_to_client(CompWindow *w);
EINTERN void compiz_win_hash_client(CompWindow *w, E_Client *ec);
EINTERN void compiz_win_hash_del(CompWindow *w);
EINTERN unsigned int client_state_to_compiz_state(const E_Client *ec);
EINTERN unsigned int win_type_to_compiz(E_Window_Type type);

EINTERN void compiz_texture_del(CompTexture *texture);
EINTERN void compiz_texture_init(CompTexture *texture);
#endif
