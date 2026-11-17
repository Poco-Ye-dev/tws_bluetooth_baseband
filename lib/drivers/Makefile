# SPDX-License-Identifier: MIT
objs := $(ROOT)/drivers/gpio/gpio.o \
	$(ROOT)/drivers/uart/uart.o \
	$(ROOT)/drivers/i2c/i2c.o \
	$(ROOT)/drivers/spi/spi.o \
	$(ROOT)/drivers/adc/adc.o \
	$(ROOT)/drivers/pwm/pwm.o \
	$(ROOT)/drivers/flash/flash.o \
	$(ROOT)/drivers/dma/dma.o \
	$(ROOT)/drivers/i2s/i2s.o \
	$(ROOT)/drivers/rtc/rtc.o \
	$(ROOT)/drivers/watchdog/watchdog.o \
	$(ROOT)/drivers/irq/irq.o \
	$(ROOT)/drivers/core/regmap.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/drivers.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
