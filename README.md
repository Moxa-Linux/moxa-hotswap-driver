# moxa-hotswap-driver

## Moxa HDD/SSD hot swap driver
The hotswap driver is for moxa V2616A embedded computer. `/dev/hotswap` is the entry point for tracking the disks hotswapping. You can use the 'mxhtspd' to monitor and programming the button and harddisk hotswap. The mxhtspd provides a callback script for stopping your program and hotswapping your disk in run time. Following describes how to compile this driver.


#### Setting the path to your kernel source tree in Makefile
To compile this driver, you should modify the KDIR to the path of your kernel headers. We configure the KDIR in Makefile for configuring the path of the kernel headers.
```
# to install Linux headers
$ apt update && apt install linux-headers-`uname -r`
```
```
KDIR=/lib/modules/`uname -r`/build
```

#### Compile & install the driver
```
$ cd moxa-hotswap-driver
$ make && make install
Making modules...
make -C /lib/modules/`uname -r`/build M=`pwd` modules
make[1]: Entering directory '/usr/src/linux-headers-4.9.0-7-amd64'
  CC [M]  /workspace/da-NGS/v2406c/source/moxa-hotswap-driver/main.o
  LD [M]  /workspace/da-NGS/v2406c/source/moxa-hotswap-driver/moxa_hotswap.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /workspace/da-NGS/v2406c/source/moxa-hotswap-driver/moxa_hotswap.mod.o
  LD [M]  /workspace/da-NGS/v2406c/source/moxa-hotswap-driver/moxa_hotswap.ko
make[1]: Leaving directory '/usr/src/linux-headers-4.9.0-7-amd64'

# Load this driver
$ insmod moxa_hotswap.ko
[    3.771694] Initializing MOXA hotswap module
```

#### Moxa HDD/SSD hot swap utility
[gitlab source](http://gitlab.syssw.moxa.com/ElvisCW_Yao/moxa-v2406c-hotswapd)
