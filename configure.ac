AC_INIT(compiz, 0.0.1, zmike@enlightenment.org)
AC_CONFIG_MACRO_DIR([m4])
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE([1.8 dist-xz])
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AM_PROG_CC_STDC

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

#m4_ifdef([AM_GNU_GETTEXT_VERSION], [
#AM_GNU_GETTEXT_VERSION([0.14])
#])

#m4_ifdef([AM_GNU_GETTEXT], [
#AM_GNU_GETTEXT([external])
#po_makefile_in=po/Makefile.in
#AM_CONDITIONAL([HAVE_PO], [true])
#],[
#AM_CONDITIONAL([HAVE_PO], [false])
#])
#AC_SUBST(LTLIBINTL)

PKG_CHECK_MODULES([E], [enlightenment >= 0.19.99 compiz < 0.9 libxml-2.0])
release=$(pkg-config --variable=release enlightenment)
MODULE_ARCH="$host_os-$host_cpu-$release"
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")

module_dir="$(pkg-config --variable=modules enlightenment)"
AC_SUBST(module_dir)
# Find edje_cc
AC_ARG_WITH(edje-cc,
  AC_HELP_STRING([--with-edje-cc=PATH], [specify a specific path to edje_cc]),
  [
    v=$withval;
    EDJE_CC=$v
  ],[
    EDJE_CC=$(pkg-config --variable=prefix edje)/bin/edje_cc
  ]
)
AC_SUBST(EDJE_CC)
AC_MSG_CHECKING([Which edje_cc to use])
AC_MSG_RESULT(${EDJE_CC})

with_gles=no
AC_ARG_WITH(gles,
  AC_HELP_STRING([--with-gles], [specify whether to enable GLES hacks])
)
if test "$with_gles" = yes; then
  AC_DEFINE_UNQUOTED([HAVE_JWZGLES], [1], [using jwz's gl hacks])
  AC_MSG_RESULT(using OpenGL ES compatiblity shim)
elif test "$with_gles" != no; then
  echo "error: must be yes or no: --with-gles"
  exit 1
fi
AM_CONDITIONAL([gles], [test "$with_gles" = yes])
plugindir=$libdir/compiz
AC_SUBST(plugindir)

imagedir=$datadir/compiz
AC_SUBST(imagedir)

metadatadir=$datadir/compiz
AC_SUBST(metadatadir)

AC_OUTPUT([
Makefile
module.desktop
e_modules-compiz.spec
], [
])

#$po_makefile_in

