## Process this file with automake to produce Makefile.in

#AUTOMAKE_OPTIONS   = foreign
#
#ACLOCAL_M4	   = $(top_srcdir)/config/aclocal.m4
#AUTOHEADER = @AUTOHEADER@ --localdir=$(top_srcdir)/config

AM_CFLAGS = @CFLAGS@

bin_PROGRAMS	= rrddumper

rrddumper_LDADD          = $(CORE_LIBS)
rrddumper_LDFLAGS        = -all-static

rrddumper_SOURCES = main.c rrd_dump.c rrd_open.c rrd_error.c rrd_format.c rrd_rpncalc.c rrd_not_thread_safe.c rrd_nan_inf.c rrd_utils.c rrd_client.c
