AM_CPPFLAGS = -I. \
	   -I$(top_srcdir) \
	   -I$(top_srcdir)/src \
	   -I$(includedir) \
	   -DLOCALEDIR=\"$(module_dir)/compiz/locale\" \
	   -DPACKAGE_DATA_DIR=\"$(module_dir)/$(PACKAGE)\" \
	   @E_CFLAGS@ \
       	-DPLUGINDIR=\"$(plugindir)\" \
       	-DIMAGEDIR=\"$(imagedir)\" \
       	-DMETADATADIR=\"$(metadatadir)\" \
-Wno-unused-parameter \
-Wno-unused-function \
-Wno-unused-variable


pkgdir = $(module_dir)/$(PACKAGE)/$(MODULE_ARCH)
pkg_LTLIBRARIES = src/module.la src/libcompiz.la
src_module_la_SOURCES = \
src/e_mod_main.h \
src/e_mod_main.c \
src/compiz.c

include src/exports/Makefile.mk
if gles
src_module_la_SOURCES += src/jwzgles.c
endif
src_module_la_LIBADD = @E_LIBS@
src_module_la_LDFLAGS = -module -avoid-version
src_module_la_DEPENDENCIES = $(top_builddir)/config.h

src_libcompiz_la_SOURCES = \
src/compiz_gl.c

src_libcompiz_la_CPPFLAGS = @EVAS_CFLAGS@
src_libcompiz_la_LIBADD = @EVAS_LIBS@ -ldl
src_libcompiz_la_DEPENDENCIES = $(top_builddir)/config.h
