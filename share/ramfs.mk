ifneq ($(strip $(V)), 1)
	Q ?= @
endif

ifneq ($(strip $(RAMFS)),)

TOPDIR		?=	.

RAMFS_LIBS	:=	-lramfs
RAMFS_CFLAGS	:=	-I$(DEVKITPRO)/portlibs/wiiu/include
RAMFS_TARGET	:=	app.ramfs.o

%.ramfs.o: $(TOPDIR)/$(RAMFS)
	@echo RAMFS $(notdir $@)
	$(Q)tar -H ustar -cvf ramfs.tar -C $(TOPDIR)/$(RAMFS) .
	$(Q)$(OBJCOPY) --input binary --output elf32-powerpc --binary-architecture powerpc:common ramfs.tar $@
	@rm -f ramfs.tar

else

RAMFS_LIBS	:=
RAMFS_CFLAGS	:=
RAMFS_TARGET	:=

endif
