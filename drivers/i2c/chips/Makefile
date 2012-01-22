#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#
obj-$(CONFIG_WIBRO_CMC)	+= max8893.o
obj-$(CONFIG_SENSORS_MT9T013)	+= mt9t013.o
obj-$(CONFIG_SENSORS_YDA165)	+= yda165.o
obj-$(CONFIG_SENSORS_MAX9877)	+= max9877.o
obj-$(CONFIG_SENSORS_MAX9879)	+= max9879.o
obj-$(CONFIG_SENSORS_FSA9480)	+= fsa9480.o
obj-$(CONFIG_SENSORS_KR3D_ACCEL)		+= kr3dh.o
obj-$(CONFIG_INPUT_YAS_MAGNETOMETER) 	+= yas_mag_kernel_driver.o
obj-$(CONFIG_SENSORS_YAS529_MAGNETIC) 	+= yas529.o
obj-$(CONFIG_SENSORS_ORIENTATION) 	+= orientation.o
obj-$(CONFIG_OPTICAL_GP2A)		+= gp2a_proximity.o gp2a_light.o gp2a_rpc.o
obj-$(CONFIG_GYRO_L3G4200D) 	+= l3g4200d.o
obj-$(CONFIG_SENSORS_BMA222)		+=  bma222_i2c.o bma222.o bma222_acc.o


ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

