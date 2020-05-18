CONFIG_MODULE_SIG=n

ifneq ($(KERNELRELEASE),)
	obj-m := mypipe.o
else
	PWD := $(shell pwd)
	KVER := $(shell uname -r)
	KDIR := /lib/modules/$(KVER)/build
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
