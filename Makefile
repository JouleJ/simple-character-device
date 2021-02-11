obj-m += phone_book.o

KERNEL_SRC = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_SRC) M=$(shell pwd) modules 

clean:
	make -C $(KERNEL_SRC) M=$(shell pwd) clean
