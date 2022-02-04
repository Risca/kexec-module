KDIR ?= /lib/modules/`uname -r`/build

sources := orig/relocate_kernel.S orig/machine_kexec.c orig/kexec.c

.PHONY:
default: ${sources}
	$(MAKE) -C $(KDIR) M=$$PWD

orig:
	mkdir $@

orig/relocate_kernel.S:
	ln -fs $(KDIR)/arch/x86/kernel/relocate_kernel_32.S $@

orig/machine_kexec.c: | orig
	ln -fs $(KDIR)/arch/x86/kernel/machine_kexec_32.c $@

orig/kexec.c: | orig
	ln -fs $(KDIR)/kernel/kexec.c $@
