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

PG_CONFIG := $(shell bin/find_pg_config)
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
#   $ make docs
#   $ git add docs
#   $ git commit -a
#   $ git push github gh-pages
#   $ git checkout master

docs/html: $(SOURCES)
	doxygen docs/Doxyfile

docs: docs/html



##
# release targets
#
ZIPFILE_BASENAME = pgbitmap-$(VERSION_NUMBER)
ZIPFILENAME = $(ZIPFILE_BASENAME).zip
ONLINE_DOCS = https://marcmunro.github.io/pgbitmap/html/index.html
GIT_UPSTREAM = github origin

# Ensure that we are in the master git branch
check_branch:
	@[ `git rev-parse --abbrev-ref HEAD` = master ] || \
	    (echo "    CURRENT GIT BRANCH IS NOT MASTER" 1>&2 && exit 2)

# Check that our metadata file for pgxs is up to date.  This is very
# simplistic but aimed only at ensuring you haven't forgotten to
# update the file.
check_meta: META.json
	@grep '"version"' META.json | head -2 | cut -d: -f2 | \
	    tr -d '",' | \
	    while read a; do \
	      	[ "x$$a" = "x$(PGBITMAP_VERSION)" ] || \
		  (echo "    INCORRECT VERSION ($$a) IN META.json"; exit 2); \
	    done
	@grep '"file"' META.json | cut -d: -f2 | tr -d '",' | \
	    while read a; do \
	      	[ "x$$a" = "xpgbitmap--$(PGBITMAP_VERSION).sql" ] || \
		  (echo "    INCORRECT FILE NAME ($$a) IN META.json"; exit 2); \
	    done

# Check that head has been tagged.  We assume that if it has, then it
# has been tagged correctly.
check_tag:
	@tag=`git tag --points-at HEAD`; \
	if [ "x$${tag}" = "x" ]; then \
	    echo "    NO GIT TAG IN PLACE"; \
	    exit 2; \
	fi

# Check that the latest docs have been published.
check_docs: docs
	@[ "x`cat docs/html/index.html | md5sum`" = \
	   "x`curl -s $(ONLINE_DOCS) | md5sum`" ] || \
	    (echo "    LATEST DOCS NOT PUBLISHED"; exit 2)

# Check that there are no uncomitted changes.
check_commit:
	@git status -s | wc -l | grep '^0$$' >/dev/null || \
	    (echo "    UNCOMMITTED CHANGES FOUND"; exit 2)

# Check that we have pushed the latest changes
check_origin:
	@err=0; \
	 for origin in $(GIT_UPSTREAM); do \
	    git diff --quiet master $${origin}/master 2>/dev/null || \
	    { echo "    UNPUSHED UPDATES FOR $${origin}"; \
	      err=2; }; \
	done; exit $$err

# Check that this version appears in the change history
check_history:
	@grep "^$(PGBITMAP_VERSION)" \
	    README.md >/dev/null || \
	    (echo "    CURRENT VERSION NOT RECORDED IN CHANGE HISTORY"; \
	     exit 2)

# Create a zipfile for release to pgxn, but only if everthing is ready
# to go.  Note that we distribute our dependencies in case our user is
# going to build things manually and can't build them themselves, and
# our built docs as we don't require users to have a suitable build
# environment for building them themselves, and having the docs
# installed locally is a good thing.
zipfile: 
	@$(MAKE) -k --no-print-directory \
	    check_branch check_meta check_tag check_docs \
	    check_commit check_origin check_history 2>&1 | \
	    bin/makefilter 1>&2
	@$(MAKE) do_zipfile

do_zipfile: mostly_clean deps
	git archive --format zip --prefix=$(ZIPFILE_BASENAME)/ \
	    --output $(ZIPFILENAME) master


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
