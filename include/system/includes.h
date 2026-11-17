#ifndef __SYSTEM_INCLUDES_H__
#define __SYSTEM_INCLUDES_H__

/*
 * Umbrella header for application code.  Drivers and kernel internals include
 * the narrow headers directly; applications may use this to avoid a long
 * include list.
 */

#include <core/types.h>
#include <core/compiler.h>
#include <core/errno.h>
#include <core/bitops.h>
#include <core/list.h>
#include <core/atomic.h>
#include <core/spinlock.h>
#include <core/ringbuf.h>
#include <core/crc.h>
#include <core/math.h>
#include <core/string.h>
#include <core/mem.h>
#include <core/printk.h>
#include <core/assert.h>
#include <core/log.h>

#include <system/task.h>
#include <system/sync.h>
#include <system/timer.h>
#include <system/event.h>
#include <system/init.h>
#include <system/workqueue.h>
#include <system/clock.h>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/i2c.h>
#include <driver/spi.h>
#include <driver/adc.h>
#include <driver/pwm.h>
#include <driver/flash.h>
#include <driver/dma.h>
#include <driver/i2s.h>
#include <driver/rtc.h>
#include <driver/watchdog.h>
#include <driver/irq.h>
#include <driver/regmap.h>

#include <media/frame.h>
#include <media/adpcm.h>
#include <audio/dsp.h>

#endif /* __SYSTEM_INCLUDES_H__ */
