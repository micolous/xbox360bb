obj-m := xbox360bb.o
CURRENT := $(shell uname -r)
KDIR := /lib/modules/$(CURRENT)/build
PWD := $(shell pwd)
MDIR := drivers/misc
DEST := /lib/modules/$(CURRENT)/kernel/$(MDIR)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	install -g0 -o0 xbox360bb.ko $(DEST)
	/sbin/depmod -a

clean:
	rm -f modules.order Module.symvers 
	rm -f xbox360bb.ko xbox360bb.mod.c xbox360bb.mod.o xbox360bb.o
	rm -f .xbox360bb.ko.cmd .xbox360bb.mod.o.cmd .xbox360bb.o.cmd
	rm -fr .tmp_versions

	rm -f *~
