#
#Desgin of Netlink
#

MODULE_NAME :=netlink_kernel_client
obj-m +=$(MODULE_NAME).o
#obj-m +=netlink_kernel_client2.o
#obj-m +=netlink_kernel_client3.o
#obj-m +=netlink_kernel_client4.o
#obj-m +=netlink_kernel_client5.o

#obj-m +=netlink_kernel_client6.o
#obj-m +=netlink_kernel_client7.o
#obj-m +=netlink_kernel_client8.o
#obj-m +=netlink_kernel_client9.o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
