obj-m += kernelHello-1.o
obj-m += kernelHello-2.o
obj-m += kernelHello-3.o
obj-m += kernelHello-4.o
obj-m += kernelHello-5.o
obj-m += ex_symbdriver.o
obj-m += startstop.o
startstop-objs := start.o stop.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

