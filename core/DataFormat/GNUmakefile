#
# This is an example GNUmakefile for my packages
#
PACKAGE_NAME = DataFormat

# specific names for this package
SOURCES = $(wildcard *.cxx)
FMWK_HEADERS = LinkDef.h
HEADERS = $(filter-out $(FMWK_HEADERS), $(wildcard *.h))
#HEADERS += IOManager.inl

# include options for this package
INCFLAGS  = -I.                       #Include itself
INCFLAGS += -I$(LARCV_COREDIR)

# platform-specific options
OSNAME          = $(shell uname -s)
HOST            = $(shell uname -n)
OSNAMEMODE      = $(OSNAME)

include $(LARCV_BASEDIR)/Makefile/Makefile.${OSNAME}

LDFLAGS += -L$(LARCV_LIBDIR) -lLArCVBase
# call the common GNUmakefile
include $(LARCV_BASEDIR)/Makefile/GNUmakefile.CORE

pkg_build: # nothing extra to do in this package
pkg_clean: # nothing extra to do in this package

