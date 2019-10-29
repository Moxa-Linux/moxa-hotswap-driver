TARGET := moxa_hotswap
KRELEASE ?= $(shell uname -r)
KBUILD ?= /lib/modules/$(KRELEASE)/build

obj-m += $(TARGET).o

#Use multiple files
$(TARGET)-objs := main.o

modules: $(TARGET).ko

$(TARGET).ko:  main.c $(TARGET).h mxhtsp_ioctl.h
	@echo "Making modules..."
	$(MAKE) -C $(KBUILD) M=$(PWD) modules

install: modules
	/usr/bin/install -m 644 -D $(TARGET).ko /lib/modules/$(KRELEASE)/kernel/drivers/misc/$(TARGET).ko
	/usr/bin/install -m 644 -D $(TARGET).conf /usr/lib/modules-load.d/$(TARGET).conf

clean:
	$(MAKE) -C $(KBUILD) M=$(PWD) clean
