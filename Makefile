obj-m += helloNetlinkLKM.o
#obj-m += userspace.o
all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
	gcc userspace.c -o userspace -pthread
clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
	rm ./userspace

