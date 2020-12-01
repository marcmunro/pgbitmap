# GNUmakefile
#
#      PGXS-based makefile for pgbitmap
#
#      Copyright (c) 2020 Marc Munro
#      Author:  Marc Munro
#      License: BSD
#
# For a list of targets use make help.
# 

# Default target.  Dependencies for this are further defined in the 
# included makefiles for SUBDIRS below.
all:

BUILD_DIR = $(shell pwd)
MODULE_big = pgbitmap
OBJS = $(SOURCES:%.c=%.o)
DEPS = $(SOURCES:%.c=%.d)
EXTENSION=pgbitmap
MODULEDIR=extension
PGBITMAP_VERSION = $(shell \
    grep default_version pgbitmap.control | sed 's/[^0-9.]*\([0-9.]*\).*/\1/')

PGBITMAP_CONTROL = pgbitmap--$(PGBITMAP_VERSION).sql
OLD_PGBITMAP_CONTROLS = $(shell ls pgbitmap--*.sql | grep -v $(PGBITMAP_VERSION))

SUBDIRS = src test 
EXTRA_CLEAN = $(SRC_CLEAN) 
include $(SUBDIRS:%=%/Makefile)


DATA = $(wildcard pgbitmap--*.sql)

PG_CONFIG := $(shell ./find_pg_config)
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

ifneq ($(origin FORCE_32_BIT), undefined)
DFORCE_32_BIT = -DFORCE_32_BIT=1
endif

override CFLAGS := $(CFLAGS) -g -O0 $(DFORCE_32_BIT)

include $(DEPS)

.PHONY: deps zipfile clean make_deps clean pgbitmap_clean list \
	zipfile help docs

# Build per-source dependency files for inclusion
# This ignores header files and any other non-local files (such as
# postgres include files).  Since I don't know if this will work
# on non-Unix platforms, we will ship pgbitmap with the dep files
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
	@>/dev/null

# Target that rebuilds all dep files unconditionally.  There should be a
# simpler way to do this using .PHONY but I can't figure out how.
deps: 
	rm -f $(DEPS)
	$(MAKE) MAKEFLAGS="$(MAKEFLAGS)" make_deps

# Run doxygen to build html docs
# Docs are installed to github using the following set of commands:
#   $ git commit -a
#   $ git checkout gh-pages
#   $ git merge master
#   $ make html
#   $ git commit -a
#   $ git push origin gh-pages
#   $ git checkout master
docs:
	doxygen docs/Doxyfile

# Create tarball for distribution to pgxn
zipfile: clean desp
	git archive --format zip --prefix=pgbitmap-$(PGBITMAP_VERSION)/ \
	    --output ./pgbitmap-$(PGBITMAP_VERSION).zip master

# Target to remove generated and backup files.
clean: pgbitmap_clean docs_clean

pgbitmap_clean:
	@rm -f PG_VERSION PG_CONFIG $(OBJS) \
	    $(OLD_PGBITMAP_CONTROLS) $(MODULE_big).so \
	    *~ src/*~ test/*~ pgbitmap*.zip

docs_clean:
	@rm -rf docs/html docs/*~ 

# Provide a list of the targets buildable by this makefile.
list help:
	@echo "\n\
 Major targets for this makefile are:\n\n\
 all          - build pgbitmap library\n\
 clean        - remove target and object files\n\
 deps         - recreate the <nnn>.d dependency files\n\
 docs         - run doxygen to create html docs\n\
 zipfile      - create a zipfile suitable for pgxn\n\
 test         - run unit tests on installed extension\n\
 install      - install the extension (may require root)\n\
 help         - show this list of major targets\n\
\n\
"
