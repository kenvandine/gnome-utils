#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

REQUIRED_AUTOMAKE_VERSION=1.7
PKG_NAME="GNOME Utilities"

(test -f $srcdir/configure.ac \
  && test -f $srcdir/ChangeLog \
  && test -d $srcdir/gsearchtool) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level gnome directory"
    exit 1
}


which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

USE_GNOME2_MACROS=1 USE_COMMON_DOC_BUILD=yes . gnome-autogen.sh

# we need to patch gtk-doc.make to support pretty output with
# libtool 1.x.  Should be fixed in the next version of gtk-doc.
# To be more resilient with the various versions of gtk-doc one
# can find, just sed gkt-doc.make rather than patch it.
sed -e 's#) --mode=compile#) --tag=CC --mode=compile#' gtk-doc.make > gtk-doc.temp \
                && mv gtk-doc.temp gtk-doc.make
sed -e 's#) --mode=link#) --tag=CC --mode=link#' gtk-doc.make > gtk-doc.temp \
                && mv gtk-doc.temp gtk-doc.make
