obj-m := driver_7segs.o
KERNEL_DIR ?= $(HOME)/linux_rpi/linux

all:
	make -C $(KERNEL_DIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(PWD) clean

deploy:
	scp *.ko pi@10.0.0.10:/home/pi/syscom
