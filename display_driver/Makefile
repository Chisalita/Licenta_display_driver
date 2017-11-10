CCPREFIX=arm-linux-gnueabihf-
obj-m+=hello.o

all:
	make ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /large/Licenta/linux M=$(PWD) modules
clean:
	make -C /large/Licenta/linux/ M=$(PWD) clean
