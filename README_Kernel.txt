    	HOW TO BUILD KERNEL 2.6.35 FOR SPH-M930

1. How to Build
	- get Toolchain
		From Codesourcery site(	http://www.codesourcery.com )
		recommand : up to 2009q3 version.
	- edit Makefile
			edit "CROSS_COMPILE" to right toolchain path(You downloaded).
			EX) CROSS_COMPILE	?= /opt/toolchains/arm-2009q3/bin/arm-none-linux-gnueabi-                // You have to check.

	- make
	                
		$ make clean
		$ make mrproper
		$ make SPH-M930EMMC_defconfig
		$ make
	
2. Output files
	- Kernel : Kernel/arch/arm/boot/zImage
	- module : Kernel/drivers/*/*.ko
	
3. How to make .tar binary for downloading into target.
	- change current directory to Kernel/arch/arm/boot
	- type following command
	$ tar cvf SPH-M930_Kernel.tar zImage