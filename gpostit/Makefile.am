dtdir=$(datadir)/gnome/apps/Utilities

INCLUDES = $(GNOME_INCLUDEDIR)

#-----------------------------------------------------------------------------
## Rules similar to `automake 1.2f' for building ObjC files, with dependencies
SUFFIXES = .m

OBJCFLAGS = $(CFLAGS)
OBJCCOMPILE = $(OBJC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(OBJCFLAGS)
LTOBJCCOMPILE = $(LIBTOOL) --mode=compile $(OBJCCOMPILE)

# ** This has to be called `LINK', not `OBJCLINK'
LINK = $(LIBTOOL) --mode=link $(OBJC) $(OBJCFLAGS) $(LDFLAGS) -o $@

gen_deps_o = @MAINT@-Wp,-MD,.deps/$(*F).P
gen_deps_lo = @MAINT@-Wp,-MD,.deps/$(*F).p

%.o: %.m
	$(OBJCCOMPILE) $(gen_deps_o) -c $<

%.lo: %.m
	$(LTOBJCCOMPILE) $(gen_deps_lo) -c $<
	@MAINT@@-sed -e 's/^\([^:]*\)\.o:/\1.lo \1.o:/' \
	@MAINT@	< .deps/$(*F).p > .deps/$(*F).P
	@MAINT@@-rm -f .deps/$(*F).p
#-----------------------------------------------------------------------------

bin_PROGRAMS = gpostit

gpostit_SOURCES = 	\
	gpostit.m

gpostit_LDADD = \
        $(OBGNOME_LIBS) -lgtktty $(OBGTK_LIBS) $(GNOMEUI_LIBS) \
        $(OBJC_LIBS) $(GNOME_LIBDIR) $(INTLLIBS)

EXTRA_DIST = gpostit.desktop

install-data-local:
	$(mkinstalldirs) $(dtdir)
	$(INSTALL_DATA) $(srcdir)/gpostit.desktop $(dtdir)/gpostit.desktop
