# Makefile
#
#      Makefile for src directory of pgbitmap
#
#      Copyright (c) 2020 Marc Munro
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


SOURCES = src/pgbitmap.c 

ifdef EXTENSION
	LIBDIR=$(DESTDIR)$(datadir)/extension
else
	LIBDIR=$(DESTDIR)$(pkglibdir)
endif

INSTALLED_LIB = $(LIBDIR)/$(addsuffix $(DLSUFFIX), pgbitmap)
PGBITMAP_LIB = $(BUILD_DIR)/$(addsuffix $(DLSUFFIX), pgbitmap)
HEADERS = $(wildcard src/*.h)
SRC_CLEAN = $(PGBITMAP_LIB)

all: $(PGBITMAP_CONTROL)

$(PGBITMAP_CONTROL): src/pgbitmap_interface.sqs
	@echo Creating $(PGBITMAP_CONTROL)
	@sed -e 's!@LIBPATH@!$$libdir/pgbitmap!g' <$< >$@

