ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure compile depcomp install-sh ltmain.sh \
		       missing module.desktop config.rpath mkinstalldirs


include src/Makefile.mk
#if HAVE_PO

#SUBDIRS += po

#endif

filesdir = $(module_dir)/$(PACKAGE)
files_DATA = module.desktop #e-module-compiz.edj # images/icon.png

EXTRA_DIST = module.desktop.in
#	     e_modules-compiz.spec.in \
#	     e-module-compiz.edc \
#	     epic-e-256.png \
#		win_shadow.png

#e-module-compiz.edj: e-module-compiz.edc
#	$(EDJE_CC) -id $(top_srcdir) $< $@

clean-local:
	 rm -rf *.edj module.desktop e_modules-compiz.spec *~

uninstall-local:
	 rm -rf $(DESTDIR)$(module_dir)/$(PACKAGE)

