# GNUmakefile
#
#      PGXS-based makefile for bitmap
#
#      Copyright (c) 2015,2018 Marc Munro
#      Author:  Marc Munro
#      License: BSD
#
# For a list of targets use make help.
# 

# Default target.  Dependencies for this are further defined in the 
# included makefiles for SUBDIRS below.
all:

BUILD_DIR = $(shell pwd)
MODULE_big = bitmap
OBJS = $(SOURCES:%.c=%.o)
DEPS = $(SOURCES:%.c=%.d)
EXTENSION=bitmap
MODULEDIR=extension
BITMAP_VERSION = $(shell \
    grep default_version bitmap.control | sed 's/[^0-9.]*\([0-9.]*\).*/\1/')

BITMAP_CONTROL = bitmap--$(BITMAP_VERSION).sql
OLD_BITMAP_CONTROLS = $(shell ls bitmap--*.sql | grep -v $(BITMAP_VERSION))

SUBDIRS = src test 
EXTRA_CLEAN = $(SRC_CLEAN) 
include $(SUBDIRS:%=%/Makefile)


DATA = $(wildcard bitmap--*.sql)

PG_CONFIG := $(shell ./find_pg_config)
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

ifneq ($(origin FORCE_32_BIT), undefined)
DFORCE_32_BIT = -DFORCE_32_BIT=1
endif

override CFLAGS := $(CFLAGS) -O0 $(DFORCE_32_BIT)

include $(DEPS)

.PHONY: deps tarball_clean make_deps clean bitmap_clean list help docs

# Build per-source dependency files for inclusion
# This ignores header files and any other non-local files (such as
# postgres include files).  Since I don't know if this will work
# on non-Unix platforms, we will ship bitmap with the dep files
# in place).  This target is mostly for maintainers who may wish
# to rebuild dep files.
%.d: %.c
	@echo Recreating $@
	@$(SHELL) -ec "$(CC) -MM -MT $*.o $(CPPFLAGS) $< | \
		xargs -n 1 | grep '^[^/]' | \
		sed -e '1,$$ s/$$/ \\\\/' -e '$$ s/ \\\\$$//' \
		    -e '2,$$ s/^/  /' | \
		sed 's!$*.o!& $@!g'" > $@

# Target used by recursive call from deps target below.  This ensures
# that make deps always rebuilds the dep files even if they are up to date.
make_deps: $(DEPS)

# Target that rebuilds all dep files unconditionally.  There should be a
# simpler way to do this using .PHONY but I can't figure out how.
deps: 
	rm -f $(DEPS)
	$(MAKE) MAKEFLAGS="$(MAKEFLAGS)" make_deps

# Run doxygen to build html docs
#
docs:
	doxygen docs/Doxyfile

# Target to remove generated and backup files.
clean: bitmap_clean docs_clean

bitmap_clean:
	@rm -f PG_VERSION PG_CONFIG $(OBJS) $(DEPS) \
	    $(OLD_BITMAP_CONTROLS) $(MODULE_big).so \
	    *~ src/*~ test/*~

docs_clean:
	@rm -rf docs/html docs/*~ 

# Provide a list of the targets buildable by this makefile.
list help:
	@echo "\n\
 Major targets for this makefile are:\n\n\
 all          - build bitmap library\n\
 clean        - remove target and object files\n\
 deps         - recreate the <nnn>.d dependency files\n\
 docs         - run doxygen to create html docs\n\
 test         - run unit tests on installed extension\n\
 install      - install the extension (may require root)\n\
 help         - show this list of major targets\n\
\n\
"
