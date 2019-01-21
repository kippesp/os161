#
# OS/161 build environment: build a kernel
#
# Note that kernels build in kernel build directories and not into
# $(BUILDDIR).
#

#
# The makefile stub generated by the kernel config script looks like this:
#   KTOP=../..			# top of the kernel tree
#   TOP=$(KTOP)/..		# top of the whole tree
#   KDEBUG=-g			# debug vs. optimize
#   CONFNAME=GENERIC		# name of the kernel config file
#   .include "$(TOP)/mk/os161.config.mk"
#   .include "files.mk"
#   .include "$(TOP)/mk/os161.kernel.mk"
#
# files.mk is also generated by the kernel config script.
#

#
# Additional defs for building a kernel.
#

# All sources.
ALLSRCS=$(SRCS) $(SRCS.MACHINE.$(MACHINE)) $(SRCS.PLATFORM.$(PLATFORM))

# Filename for the kernel.
KERNEL=kernel

# Don't use headers and libraries that don't belong to the kernel.
# Kernels have to be standalone.
KCFLAGS+=-nostdinc
KLDFLAGS+=-nostdlib

# Do use the kernel's header files.
KCFLAGS+=-I$(KTOP)/include -I$(KTOP)/dev -I. -Iincludelinks

# Tell gcc that we're building something other than an ordinary
# application, so it makes fewer assumptions about standard library
# functions.
KCFLAGS+=-ffreestanding

# Define _KERNEL so code in src/common can tell which header files
# it should use.
KCFLAGS+=-D_KERNEL

# Provide the linker with a "linker script". This is a piece of
# obscure mumble that tells the linker how to put together the output
# program. We need it because a kernel needs (in general) to be linked
# to run at a different virtual address from an ordinary program.
#
# Traditionally all you need to do is pass -Ttext to set the address
# for the text (code), but this doesn't work with the GNU linker by
# default because of a silly design bug.
#
# Look for a linker script for the PLATFORM first, and if not that try
# MACHINE, and if neither of those exists assume we don't actually
# need one.

.if exists($(KTOP)/arch/$(PLATFORM)/conf/ldscript)
KLDFLAGS+=-T $(KTOP)/arch/$(PLATFORM)/conf/ldscript
.elif exists($(KTOP)/arch/$(MACHINE)/conf/ldscript)
KLDFLAGS+=-T $(KTOP)/arch/$(MACHINE)/conf/ldscript
.endif

#
# This should expand to all the header files in the kernel so they can
# be fed to tags.
#
TAGS_HEADERS=\
	$(KTOP)/*/*.h \
	$(KTOP)/include/kern/*.h \
	$(KTOP)/dev/*/*.h \
	$(KTOP)/arch/$(MACHINE)/*/*.h \
	$(KTOP)/arch/$(MACHINE)/include/kern/*.h \
	$(KTOP)/arch/$(PLATFORM)/*/*.h

#
# Rules.
#

# Default rule: link the kernel.
all: includelinks .WAIT $(KERNEL)

#
# Here's how we link the kernel.
#
# vers.c/.o is generated on every build. It contains a numeric serial
# number incremented every time newvers.sh is run.  These values are
# printed out by newvers.sh and are also displayed at boot time. This
# makes it possible to tell at a glance whether you're actually
# running the same kernel you just compiled.
#
# The version number is kept in the file called "version" in the build
# directory.
#
# By immemorial tradition, "size" is run on the kernel after it's linked.
#
$(KERNEL):
	$(KTOP)/conf/newvers.sh $(CONFNAME)
	$(CC) $(KCFLAGS) -c vers.c
	$(LD) $(KLDFLAGS) $(OBJS) vers.o -o $(KERNEL)
	@echo '*** This is $(CONFNAME) build #'`cat version`' ***'
	$(SIZE) $(KERNEL)

#
# Use the -M argument to gcc to get it to output dependency information.
# Note that we use -M, which includes deps for #include <...> files,
# rather than -MM, which doesn't. This is because we are the operating
# system: the #include <...> files are part of our project -- in fact, in
# the kernel they're the kernel's own include files -- and they will be
# changing!
#
# Each source file's depend info gets dumped into its own .depend file
# so the overall depend process parallelizes. Otherwise (assuming you
# have a reasonably modern machine) this is the slowest part of the
# kernel build.
#
depend:
	$(MAKE) includelinks
	rm -f .depend.* || true
	$(MAKE) realdepend

.for _S_ in $(ALLSRCS)
DEPFILES+=.depend.$(_S_:T)
.depend.$(_S_:T):
	$(CC) $(KCFLAGS) -M $(_S_) > .depend.$(_S_:T)
.endfor
realdepend: $(DEPFILES)
	cat $(DEPFILES) > .depend

# our make does this implicitly
#.-include ".depend"

#
# This allows includes of the forms
#    <machine/foo.h>
#    <kern/machine/foo.h>
#    <platform/foo.h>
# and also (for this platform/machine)
#    <mips/foo.h>
#    <kern/mips/foo.h>
#    <sys161/foo.h>
# to go to the right place.
#
includelinks:
	mkdir includelinks
	mkdir includelinks/kern
	ln -s ../../../arch/$(MACHINE)/include includelinks/$(MACHINE)
	ln -s ../../../../arch/$(MACHINE)/include/kern \
						includelinks/kern/$(MACHINE)
	ln -s ../../../arch/$(PLATFORM)/include includelinks/$(PLATFORM)
	ln -s $(MACHINE) includelinks/machine
	ln -s $(MACHINE) includelinks/kern/machine
	ln -s $(PLATFORM) includelinks/platform

#
# Remove everything generated during the compile.
# (To remove absolutely everything automatically generated, you can just
# blow away the whole compile directory.)
#
clean:
	rm -f *.o *.a tags $(KERNEL)
	rm -rf includelinks
	@ABSTOP=$$(readlink -f $(TOP))
	rm -f $(OSTREE)/.src
	rm -f $(TOP)/.root

distclean cleandir: clean
	rm -f .depend

#
# Rerun config for this configuration.
#
reconfig:
	(cd $(KTOP)/conf && ./config $(CONFNAME))

#
# [ -d $(OSTREE) ] succeeds if $(OSTREE) is a directory.
# (See test(1).) Thus, if $(OSTREE) doesn't exist, it will be created.
#
# The kernel gets installed at the top of the installed system tree.
# Since with OS/161 it's relatively likely that you'll be working with
# several configurations at once, it gets installed under the name of
# this config, and a symbolic link with the "real" name is set up to
# point to the last kernel installed.
#
install:
	[ -d $(OSTREE) ] || mkdir $(OSTREE)
	cp $(KERNEL) $(OSTREE)/$(KERNEL)-$(CONFNAME)
	-rm -f $(OSTREE)/$(KERNEL)
	ln -s $(KERNEL)-$(CONFNAME) $(OSTREE)/$(KERNEL)
	@ABSTOP=$$(readlink -f $(TOP))
	ln -Tsf $(ABSTOP) $(OSTREE)/.src
	ln -Tsf $(OSTREE) $(ABSTOP)/.root

#
# Run tags on all the sources and header files. This is probably not
# the most useful way to do this and may need attention. (XXX?)
#
tags:
	ctags $(ALLSRCS) $(TAGS_HEADERS)

#
# This tells make that these rules are not files so it (hopefully)
# won't become confused if files by those names appear.
#
.PHONY: all depend realdepend clean reconfig install tags

#
# Compilation rules.
#
.for _S_ in $(ALLSRCS)
OBJS+=$(_S_:T:R).o
$(_S_:T:R).o: $(_S_)
	$(CC) $(KCFLAGS) -c $(_S_)
.endfor

# Make the kernel depend on all the object files.
$(KERNEL): $(OBJS)

# End.
