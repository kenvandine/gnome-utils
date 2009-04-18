m4_define([gnome_utils_major], [2])
m4_define([gnome_utils_minor], [27])
m4_define([gnome_utils_micro], [1])

m4_define([gnome_utils_version], [gnome_utils_major.gnome_utils_minor.gnome_utils_micro])

AC_INIT([gnome-utils],
        [gnome_utils_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-utils])

AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

GNOME_COMMON_INIT

IT_PROG_INTLTOOL([0.40.0])

AC_ISC_POSIX
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PATH_PROG(GLIB_GENMARSHAL, [glib-genmarshal])
AC_PATH_PROG(GLIB_MKENUMS, [glib-mkenums])

GNOME_COMPILE_WARNINGS([yes])

AC_PATH_XTRA

AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_RDEV
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([getpgid])


# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LIBGDICT_LT_VERSION=6:5:0
AC_SUBST(LIBGDICT_LT_VERSION)

dnl host architecture check
AS_CASE([$host],

        [*-*-solaris*], [SUN_OS="-DON_SUN_OS"],
        [*],            [SUN_OS=""]
)
AC_SUBST(SUN_OS)

dnl pkg-config check

GLIB_REQUIRED=2.20.0
GIO_REQUIRED=2.16.0
GTK_REQUIRED=2.16.0
LIBGNOME_DESKTOP_REQUIRED=2.9.91
LIBGNOME_REQUIRED=2.13.2
LIBGNOMEUI_REQUIRED=2.13.7
LIBPANEL_APPLET_REQUIRED=2.13.4
LIBGTOP_REQUIRED=2.12.0

# common checks
PKG_CHECK_MODULES(GNOME_UTILS, glib-2.0 >= $GLIB_REQUIRED dnl
                               gio-2.0 >= $GIO_REQUIRED   dnl
                               gconf-2.0                  dnl
                               gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GNOME_UTILS_CFLAGS)
AC_SUBST(GNOME_UTILS_LIBS)

# separate check for libgnome/libgnomeui, so that we can easily
# remove the variables
PKG_CHECK_MODULES(LIBGNOME, libgnome-2.0 >= $LIBGNOME_REQUIRED dnl
                            libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED)
AC_SUBST(LIBGNOME_CFLAGS)
AC_SUBST(LIBGNOME_LIBS)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

# libgdict requires just gtk+
PKG_CHECK_MODULES(LIBGDICT, glib-2.0 >= $GLIB_REQUIRED dnl
                            gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(LIBGDICT_CFLAGS)
AC_SUBST(LIBGDICT_LIBS)

# For each cycle:
# 	first release: increment major += 1, minor = micro = 0;
# 	each release before API freeze: minor += 1;
# 	each release after API freeze: micro += 1;
# Even if this library is not part of the developers platform, we
# follow the same rules: no ABI breakage (unless unavoidable) and
# no API breakage past the API freeze.
m4_define([gdict_major_version], [0])
m4_define([gdict_minor_version], [11])
m4_define([gdict_micro_version], [0])
m4_define([gdict_version], [gdict_major_version.gdict_minor_version.gdict_micro_version])

GDICT_MAJOR_VERSION=gdict_major_version
GDICT_MINOR_VERSION=gdict_minor_version
GDICT_MICRO_VERSION=gdict_micro_version
GDICT_VERSION=gdict_version

AC_SUBST(GDICT_MAJOR_VERSION)
AC_SUBST(GDICT_MINOR_VERSION)
AC_SUBST(GDICT_MICRO_VERSION)
AC_SUBST(GDICT_VERSION)

# xext for gnome-screenshot; in theory checking for xext should be
# enough but there are a lot of broken distros out there
PKG_CHECK_MODULES(XSHAPE, xext,
                  [],
                  [AC_CHECK_HEADERS(X11/extensions/shape.h, XSHAPE_LIBS="-lXext")])
AC_SUBST(XSHAPE_LIBS)

# gnome-desktop is needed for gsearchtool
PKG_CHECK_MODULES(GNOMEDESKTOP, gnome-desktop-2.0 >= $LIBGNOME_DESKTOP_REQUIRED)
AC_SUBST(GNOMEDESKTOP_CFLAGS)
AC_SUBST(GNOMEDESKTOP_LIBS)

# for anything which calls gconftool-2 correctly
AC_PATH_PROG(GCONFTOOL, [gconftool-2], [no])
AS_IF([test "x$GCONFTOOL" = "xno"],
      [AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])]
)

AM_GCONF_SOURCE_2

# Gdict applet checks
PKG_CHECK_MODULES(APPLET, libpanelapplet-2.0 >= $LIBPANEL_APPLET_REQUIRED)
AC_SUBST(APPLET_LIBS)
AC_SUBST(APPLET_CFLAGS)

# Baobab checks
PKG_CHECK_MODULES(LIBGTOP, libgtop-2.0 >= $LIBGTOP_REQUIRED)
AC_SUBST(LIBGTOP_CFLAGS)
AC_SUBST(LIBGTOP_LIBS)

dnl ***************************************************************
dnl Other miscellaneous checks 
dnl ***************************************************************

dnl Enable debug messages
m4_define([debug_default], [m4_if(m4_eval(gdict_minor_version % 2), [1], [yes], [minimum])])
AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                              [Enable debug messages @<:@default=debug_default@:>@])],
              [],
              [enable_debug=debug_default])

AS_CASE([$enable_debug],

        [yes],
        [
          test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
          GDICT_DEBUG_CFLAGS="-DGDICT_ENABLE_DEBUG"
        ],

        [minimum],
        [GDICT_DEBUG_CFLAGS="-DG_DISABLE_CAST_CHECKS"],

        [no],
        [GDICT_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"],

        [AC_MSG_ERROR([Unknown argument to --enable-debug])]
)

AC_SUBST(GDICT_DEBUG_CFLAGS)

dnl IPv6 support
AC_MSG_CHECKING([whether to enable IPv6])
AC_ARG_ENABLE([ipv6],
              [AC_HELP_STRING([--enable-ipv6=@<:@yes/no@:>@],
                              [Enables compilation of IPv6 code])],
              [],
              [enable_ipv6=yes])

AS_IF([test "x$enable_ipv6" = "xyes"],
      [
        AC_TRY_COMPILE(
          [
            #include <sys/socket.h>
            #include <sys/types.h>
          ],
          [
            struct sockaddr_storage ss;
            socket(AF_INET6, SOCK_STREAM, 0)
          ],
          [have_ipv6=yes],
          [have_ipv6=no]
        )
      ]
)

AS_IF([test "x$have_ipv6" = "xyes"],
      [
        have_getaddrinfo=no
        AC_CHECK_FUNC(getaddrinfo, have_getaddrinfo=yes)

        AS_IF([test "x$have_getaddrinfo" != "xyes"],
              [
                for lib in bsd socket inet; do
                  AC_CHECK_LIB($lib,
                               [getaddrinfo],
                               ["LIBS=$LIBS -l$lib"; have_getaddrinfo=yes; break])
                done
              ]
        )

        AS_IF([test "x$have_getaddrinfo" = "xyes"],
              [AC_DEFINE([ENABLE_IPV6], [1], [Define whether IPv6 support is enabled])]
        )
      ]
)


dnl strftime extension checks
AC_TRY_RUN([
  #include <time.h>
  int main (void) {
    char buf[100];
    struct tm tm = {0};
    tm.tm_year = 99;
    if (strftime(buf, 100, "%EY", &tm) == 4 && strcmp (buf, "1999")==0)
      return 0;
    return 1;
  }
  ],
  [
    AC_DEFINE([HAVE_STRFTIME_EXTENSION], [1],
              [Define if strftime supports %E and %O modifiers.])
  ]
)

dnl gnome-search-tool checks
withval=""
AC_ARG_WITH([grep],
            AC_HELP_STRING([--with-grep=@<:@grep command@:>@]
                           [Specify where to find the grep binary]),
            [
              AS_IF([test x$withval != x],
                    [AC_MSG_RESULT(${withval} is used for gnome-search-tool.)],

                    [AC_MSG_RESULT(grep is used for gnome-search-tool.)]
              )
            ])

AS_IF([test x$withval != x],
      [GREP_COMMAND="$withval"],

      [GREP_COMMAND="grep"]
)

AC_SUBST(GREP_COMMAND)


dnl logview checks
AC_ARG_ENABLE([zlib],
              [AC_HELP_STRING([--disable-zlib], [disable zlib support])])
msg_zlib=no
Z_LIBS=

AS_IF([test "x$enable_zlib" != "xno"],
      [
        AC_CHECK_HEADER([zlib.h], [AC_CHECK_LIB([z], [inflate], [msg_zlib=yes])])

        AS_IF([test "x$msg_zlib" = "xyes"],
              [
                AC_DEFINE(HAVE_ZLIB, [1],
                          [Define to 1 if we're building with ZLib support])
                Z_LIBS="-lz"
              ]
        )
      ]
)

AC_SUBST(Z_LIBS)

dnl scrollkeeper checks 
AC_PATH_PROG(SCROLLKEEPER_CONFIG, [scrollkeeper-config], [no])
AS_IF([test x$SCROLLKEEPER_CONFIG = xno]
      [
        AC_MSG_ERROR([Couldn't find scrollkeeper-config. Please install the scrollkeeper package])
      ]
)

dnl = Enable strict compiler flags =========================================

# use strict compiler flags only on development releases
m4_define([maintainer_flags_default], [m4_if(m4_eval(gnome_utils_minor % 2), [1], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AC_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>@],
                              [Use strict compiler flags @<:@default=maintainer_flags_default@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])

AS_IF([test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
      [
        MAINTAINER_CFLAGS="-Werror -Wall -Wshadow -Wcast-align -Wno-uninitialized -Wformat-security -Winit-self"
        AC_SUBST(MAINTAINER_CFLAGS)
      ]
)

dnl Internationalization
GETTEXT_PACKAGE=gnome-utils-2.0
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Define the gettext package to use])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

# AM_GLIB_GNU_GETTEXT above substs $DATADIRNAME
# this is the directory where the *.{mo,gmo} files are installed
gnomeutilslocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gnomeutilslocaledir)

dnl gtk-doc stuff
GTK_DOC_CHECK([1.10])

dnl gnome-doc-utils stuff
GNOME_DOC_INIT

SHAVE_INIT([m4], [enable])

AC_CONFIG_FILES([
        Makefile

        m4/Makefile
        m4/shave-libtool
        m4/shave

        po/Makefile.in

        baobab/Makefile
        baobab/data/Makefile
        baobab/pixmaps/Makefile
        baobab/pixmaps/24x24/Makefile
        baobab/pixmaps/scalable/Makefile
        baobab/src/Makefile
        baobab/help/Makefile

        logview/Makefile
        logview/data/Makefile
        logview/tests/Makefile
        logview/help/Makefile

        gsearchtool/Makefile
        gsearchtool/data/Makefile
        gsearchtool/help/Makefile

        gnome-dictionary/Makefile
        gnome-dictionary/libgdict/Makefile
        gnome-dictionary/libgdict/gdict-version.h
        gnome-dictionary/libgdict/gdict-1.0.pc
        gnome-dictionary/data/Makefile
        gnome-dictionary/docs/Makefile
        gnome-dictionary/docs/reference/Makefile
        gnome-dictionary/docs/reference/gdict/Makefile
        gnome-dictionary/docs/reference/gdict/version.xml
        gnome-dictionary/help/Makefile
        gnome-dictionary/src/Makefile

        gnome-screenshot/Makefile
])

AC_OUTPUT

dnl <= Configuration summary =>

echo "
gnome-utils $VERSION configuration summary:

                   Compiler flags : $CFLAGS
                     Linker flags : $LDFLAGS
        Maintainer compiler flags : $MAINTAINER_CFLAGS

                           prefix : $prefix
                      sysconf dir : $sysconfdir
                          bin dir : $bindir
                         sbin dir : $sbindir
                         data dir : $datadir

        Debug messages (libgdict) : $enable_debug
         API Reference (libgdict) : $enable_gtk_doc
  Logview built with ZLib support : $msg_zlib
"
