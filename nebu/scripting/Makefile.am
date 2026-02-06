noinst_LIBRARIES=libscripting.a

libscripting_a_SOURCES= \
	scripting.c

AM_CFLAGS = \
	-I$(top_srcdir)/nebu/include \
	-I$(top_srcdir)/lua5/include

AM_CFLAGS += -DDATA_DIR=\"$(pkgdatadir)\"
