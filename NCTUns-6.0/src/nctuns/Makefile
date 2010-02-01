NAME=Yan-Shiun Tzeng

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"

# Do not print "Entering directory ..."
MAKEFLAGS += --no-print-directory

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.o targets which finally
# turn into nctuns), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifdef V
  ifeq ("$(origin V)", "command line")
    NBUILD_VERBOSE = $(V)
  endif
endif
ifndef NBUILD_VERBOSE
  NBUILD_VERBOSE = 0
endif

ifdef O
  ifeq ("$(origin O)", "command line")
    NBUILD_OUTPUT = $(O)
  endif
endif
ifndef NBUILD_OUTPUT
  NCTUNS_BIN ?= /usr/local/nctuns/bin
  NBUILD_OUTPUT = $(NCTUNS_BIN)
endif

# Don't care compiler command has be changed
# Use 'make D_CMD=1' to skip checking arguments has same arguments.

ifdef D_CMD
  ifeq ("$(origin D_CMD)", "command line")
    NBUILD_NOCMDDEP = $(V)
  endif
endif
ifndef NBUILD_NOCMDDEP
  NBUILD_NOCMDDEP = 0
endif

ifeq ($(MAKECMDGOALS),clean)
  skip-makefile	:= 1
endif

ifneq ($(filter clean install link ln,$(MAKECMDGOALS)),)
  skip-version := 1
endif

# That's our default target when none is given on the command line
PHONY := _all
_all: all

srctree		:= $(CURDIR)
src		:= $(srctree)
TOPDIR		:= $(srctree)
VPATH		:= $(srctree)

export srctree VPATH TOPDIR

# SHELL used by nbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed. 
# If it is set to "silent_", nothing wil be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If NBUILD_VERBOSE equals 0 then the above command will be hidden.
# If NBUILD_VERBOSE equals 1 then the above command is displayed.

ifeq ($(NBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q NBUILD_VERBOSE NBUILD_NOCMDDEP

# We need some generic definitions
include  $(srctree)/scripts/Nbuild.include

ifeq ($(skip-makefile),)

# Make variables (CC, etc...)
LD		:= ld
CC		:= gcc
CPP		:= $(CC) -E
CXX		:= g++
AR		:= ar
AWK		:= awk
TR		:= tr
STRIP		:= strip

KERNEL_NAME	:= $(shell uname -s | $(TR) a-z A-Z)

TCL_INCLUDE	:= -I./tcl/include
NCTUNSINCLUDE	:= -I. -I./module -I ./protocol_HEADER -I./com_HEADER $(TCL_INCLUDE) \
		   -include autoconf.h

NCTUNSFLAGS	:= -D$(KERNEL_NAME)

CPPFLAGS	:= $(NCTUNSFLAGS) $(NCTUNSINCLUDE)
CFLAGS		:= -Wall
CXXFLAGS	:= -Wall

export LD CC AR AWK TR STRIP NCTUNSINCLUDE NCTUNSFLAGS CPPFLAGS CFLAGS CXXFLAGS

# Make variables for host tools which be used during compiling NCTUns engine
HOSTCC		:= gcc
HOSTCXX		:= g++
HOSTCFLAGS	= -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCXXFLAGS	= -O2

export HOSTCC HOSTCXX HOSTCFLAGS HOSTCXXFLAGS

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

# convert .config to autoconf.h

quiet_cmd_autoconf = GEN     $@
      cmd_autoconf = scripts/basic/config $(2) > $@

autoconf.h: .config scripts/basic/config
	$(call cmd,autoconf,.config)

quiet_cmd_nctuns_version = GEN     $(2)
      cmd_nctuns_version = set -e;			\
	if [ ! -r $(2) ]; then				\
	  $(RM) -f $(2);				\
	  echo 1 > $(2);				\
	else						\
	  mv $(2) .old_version;				\
	  expr 0$$(cat .old_version) + 1 > $(2);	\
	fi;

PHONY += .version
.version:
ifeq ($(skip-version),)
	$(call cmd,nctuns_version,.version)
endif

endif	#skip-makefile

# Objects we will link into nctuns / subdirs we need to visit
core-y		:= ./
module-y	:= module/

# read configure file for NCTUns engine
include .config

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build NCTUns engine
all: nctuns

ifdef CONFIG_DEBUG_INFO
CFLAGS		+= -g
CXXFLAGS	+= -g
endif

nctuns-dirs	:= $(patsubst %/, %, $(core-y) $(module-y))
nctuns-core	:= $(patsubst %/, %/built-in.o, $(core-y))
nctuns-module	:= $(patsubst %/, %/built-in.o, $(module-y))
nctuns-all	:= $(nctuns-core) $(nctuns-module)

# find libawp.so and libtcl8.5.so location
nctuns_flags	:= -lawp -L$(srctree)/module/phy
nctuns_flags	+= -ltcl8.5 $(call cc-library,tcl8.5,$(srctree)/tcl)

quiet_cmd_nctuns__ ?= LINK    $@
cmd_nctuns__       ?= $(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(nctuns_flags) -Wl,-Map,$@.map -o $@ $^

# Build nctuns
nctuns: $(nctuns-all)
	$(call if_changed,nctuns__)

$(nctuns-all): $(nctuns-dirs) ;

$(nctuns-dirs): prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$@

PHONY += prepare scripts FORCE
# To create some subfolder which will be used durtion compiling engine
prepare: .version

# To ready all script which will be used durtion compiling engine
scripts: scripts_basic autoconf.h

# Single targets
# ---------------------------------------------------------------------------
# Single targets are compatible with:
# - build whith mixed source and output
#
#  target-dir => where to store outputfile
#  build-dir  => directory in kernel source tree to use

build-dir  = $(patsubst %/,%,$(dir $@))
target-dir = $(dir $@)

%.s: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

%.i: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

%.o: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

%.s: %.cc prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

%.i: %.cc prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

%.o: %.cc prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# library
%.so: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# folder
%/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(patsubst %/,%,$@)


###
# Cleaning is done on three levels.
# make clean     Delete most generated files

CLEAN_FILES += nctuns nctuns.map
DISTCLEAN_FILES += autoconf.h .version .old_version

# clean - Delete most
#
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_,$(srctree) $(nctuns-dirs))

PHONY += $(clean-dirs) clean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: $(clean-dirs)
	$(call cmd,rmfiles)
	$(call cmd,findfiles)

# distclean - Delete all generated files, including autoconf.h
#
distclean: rm-files	:= $(wildcard $(DISTCLEAN_FILES))
distclean-dirs		:= $(addprefix _distclean_,scripts)

PHONY += $(distclean-dirs) distclean
$(distclean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _distclean_%,%,$@)

distclean: clean $(distclean-dirs)
	$(call cmd,rmfiles)


# FIXME Should go into a make.lib or something 
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)

quiet_cmd_findfiles = CLEAN   $(srctree)
      cmd_findfiles = find . \
	 	\( -name '*.[ois]' -o -name '.*.cmd' \
		-o -name '.*.d' \) \
		-type f -print | xargs rm -f

# read all saved command lines

targets := $(wildcard $(sort $(targets)))
cmd_files := $(wildcard .*.cmd $(foreach f,$(targets),$(dir $(f)).$(notdir $(f)).cmd))

ifneq ($(cmd_files),)
  $(cmd_files): ;	# Do not try to update included dependency files
  include $(cmd_files)
endif

# Shorthand for $(Q)$(MAKE) -f scripts/Makefile.clean obj=dir
# Usage:
# $(Q)$(MAKE) $(clean)=dir
clean := -f $(srctree)/scripts/Makefile.clean obj

# other install operation
link ln: nctuns
	@ln -sf '$(CURDIR)/$<' '$(NBUILD_OUTPUT)/nctunsse'

install: nctuns
	@install -m 755 '$<' '$(NBUILD_OUTPUT)/nctunsse'

PHONY += FORCE
FORCE:

PHONY += help
include $(srctree)/scripts/Nbuild.help

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable se we can use it in if_changed and friends.
.PHONY: $(PHONY)
