#!/usr/bin/perl -w

use strict;
use Carp;

sub checkout {
  my ($file) = @_;

  print STDERR "Checking out $file ...\n";

  my @args = ("cvs", "checkout", $file);
  system (@args) == 0 or croak "cvs co $file";
}

##
## Automatically generating the AUTHORS.h will cause a lot of trouble
## if used incorrectly. If will remove people that did good work and
## add other duplicate ones that are listed with similar names or typos
## in their names in different AUTHORS files.
##
## Please do *always* manually edit the AUTHORS.h if you want to make
## change - it is not a generated file, but a file that we must maintain
## manually - noone wants to get removed from that file due to a broken
## script.
##
## Manually editing this file is a lot of work, but you destroy the good
## work of people who did this before if you blindly commit an autogen'ed
## one and they have even more work to revert your changes.
##
## You can, however use this script to create a good starting point for
## your manual edition. Make the output go to a different file and run
## a diff between them. This way you see which people will get removed
## and which will get added.
##
## To ensure that you really read this comment, you must comment out the
## following 'die' line before you can use this script. Please do not
## commit this modified (without the `die' line) script as other people
## should also read this comment.
##
## February 24, 1999
## Martin Baulig <martin@home-of-linux.org>
##

die "You must read the comment in this script first";

croak "Usage: $0 gnome-directory" unless $#ARGV == 0;

my $gnomedir = $ARGV [0];

croak "Gnome Directory `$gnomedir' is not a directory" unless -d $gnomedir;
chdir $gnomedir or croak "chdir ($gnomedir)";

# Fetch the modules file from CVS if it does not already exist.

if (!-e 'CVSROOT/modules') {
  checkout 'CVSROOT/modules';
}

# Read the modules file.

open MODULES, "$gnomedir/CVSROOT/modules" or
  croak "open ($gnomedir/CVSROOT/modules): $!";
my @modules = grep /^gnome\s+/, <MODULES>;
croak "Invalid modules file $gnomedir/CVSROOT/modules" unless
  $#modules == 0;
close MODULES;

$_ = $modules [0]; s/^gnome\s+//; s/\B\&//g;
my @gnome_modules = split;

# Read the AUTHORS files of all modules.

my %authors_files;

foreach my $module (@gnome_modules) {
  # This modules currently do not have an AUTHORS file.
  next if $module eq 'mc';

  checkout "$module/AUTHORS" unless -e "$module/AUTHORS";

  if (-e "$module/AUTHORS") {
    open AUTHORS, "$module/AUTHORS" or
      croak "open ($module/AUTHORS)";
    $authors_files{$module} = [<AUTHORS>];
    close AUTHORS;
  }
}

my %authors;

foreach my $module (keys %authors_files) {
  my @lines = grep (/^[A-Z][\w\.]*\s+([\w\.]*\s+){0,2}\s*[\<\(]/,
		    @{$authors_files{$module}});

  foreach (@lines) {
    s,[\<\(].*,,g; s,^\s*,,g; s,\s+$,,;
    $authors{$_} = $_;
  }
}


foreach (keys %authors) {
  s/^(.*)$/"$1",\n/g;
  print $_;
}
