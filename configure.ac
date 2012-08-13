m4_define([displayconf_major], [0])
m4_define([displayconf_minor], [1])
m4_define([displayconf_micro], [0])

m4_define([displayconf_version], [displayconf_major.displayconf_minor.displayconf_micro])

AC_PREREQ(2.53)
AC_INIT([guacamayo-displayconf], [displayconf_version], [http://guacamayo-project.org])
AC_CONFIG_SRCDIR([src/displayconf.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-xz tar-ustar])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

MAJOR_VERSION=displayconf_major
MINOR_VERSION=displayconf_minor
MICRO_VERSION=displayconf_micro
VERSION=displayconf_version
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(VERSION)

# check for programs
AC_PROG_CC
AM_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

# check for headers
AC_HEADER_STDC

modules="glib-2.0 x11 xrandr"

PKG_CHECK_MODULES(DISPLAYCONF, "$modules")

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
])

AC_OUTPUT
