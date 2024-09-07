obj-m := main.o
ARCH?=arm
CROSS_COMPILE?=arm-linux-gnueabihf-
LINUX_SRC?=../../linux
HOST_LINUX_SRC?=/lib/modules/$(shell uname -r)/build/
all:
	@echo "make sure to have linux kernel build before you build the module"
	@make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(LINUX_SRC) M=$(PWD) modules

clean:
	@make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(LINUX_SRC) M=$(PWD) clean

#compile the moudle in host architecture
host:
	@echo "make sure to have linux kernel build before you build the module"
	@make -C $(HOST_LINUX_SRC) M=$(PWD) modules

help:
	@make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(LINUX_SRC) M=$(PWD) help
