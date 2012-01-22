
obj-m			:= cmc7xx_sdio.o

cmc7xx_sdio-y	:= control.o hardware.o send.o receive.o wimax_sdio.o download.o wimax_i2c.o wimax_proc.o firmware.o

all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.* *~ .*.cmd Module.symvers modules.order

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
