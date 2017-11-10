CCPREFIX=arm-linux-gnueabihf-
obj-m+=hello.o

all:
	echo ${CCPREFIX}
	echo -----------
	make ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /large/Licenta/linux M=$(PWD) modules
	#make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /large/Licenta/linux/ M=$(PWD) clean
