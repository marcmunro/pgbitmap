# Makefile
#
#      Makefile for src directory of bitmap
#
#      Copyright (c) 2015 Marc Munro
#      Author:  Marc Munro
#      License: BSD
#
# Do not attempt to use this makefile explicitly: its targets are available
# and should be built from the main GNUmakefile in the parent directory.
# The GNUmakefile in this directory will build using the parent GNUmakefile
# so using make <target> in this directory will work as long as you don't
# try to specify this makefile.  It even works with emacs compile and 
# next-error functions though the number of makefiles involved seems a 
# little alarming at first.
# The whole strangeness of this makefile hierarchy derives from a, 
# possibly misguided, attempt to avoid recursive make (see the article
# "Recursive make considered harmful" for a rationale).


SOURCES = src/bitmap.c 

ifdef EXTENSION
	LIBDIR=$(DESTDIR)$(datadir)/extension
else
	LIBDIR=$(DESTDIR)$(pkglibdir)
endif

INSTALLED_LIB = $(LIBDIR)/$(addsuffix $(DLSUFFIX), bitmap)
BITMAP_LIB = $(BUILD_DIR)/$(addsuffix $(DLSUFFIX), bitmap)
HEADERS = $(wildcard src/*.h)
SRC_CLEAN = $(BITMAP_LIB)

all: $(BITMAP_CONTROL)

$(BITMAP_CONTROL): src/bitmap_interface.sqs
	@echo Creating $(BITMAP_CONTROL)
	@sed -e 's!@LIBPATH@!$$libdir/bitmap!g' <$< >$@

