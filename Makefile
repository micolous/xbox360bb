obj-m := xbox360bb.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f modules.order Module.symvers 
	rm -f xbox360bb.ko xbox360bb.mod.c xbox360bb.mod.o xbox360bb.o
	rm -f .xbox360bb.ko.cmd .xbox360bb.mod.o.cmd .xbox360bb.o.cmd
	rm -fr .tmp_versions

	rm -f *~