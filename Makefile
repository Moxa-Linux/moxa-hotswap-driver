KDIR=/lib/modules/`uname -r`/build

TARGET_MODULE=moxa_hotswap
obj-m += $(TARGET_MODULE).o

#Use multiple files
$(TARGET_MODULE)-objs := main.o 

modules: $(TARGET_MODULE).ko 

$(TARGET_MODULE).ko:  main.c $(TARGET_MODULE).h mxhtsp_ioctl.h
	@echo "Making modules..."
	$(MAKE) -C $(KDIR) M=`pwd` modules

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean

