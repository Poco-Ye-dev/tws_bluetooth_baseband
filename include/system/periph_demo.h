#ifndef __SYSTEM_PERIPH_DEMO_H__
#define __SYSTEM_PERIPH_DEMO_H__

#include <core/types.h>

/*
 * Peripheral bring-up self test.  Factory firmware runs this before enabling
 * the product path; each entry reports whether the controller was registered
 * and whether a basic transaction succeeded.
 */

struct periph_result {
    bool gpio;
    bool uart;
    bool i2c;
    bool spi;
    bool adc;
    bool timer;
};

int periph_demo_run(struct periph_result *result);
void periph_demo_report(const struct periph_result *result);

#endif /* __SYSTEM_PERIPH_DEMO_H__ */
