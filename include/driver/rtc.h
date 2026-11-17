#ifndef __DRIVER_RTC_H__
#define __DRIVER_RTC_H__

#include <core/types.h>
#include <core/errno.h>

struct rtc_time {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 weekday;
};

typedef void (*rtc_alarm_cb_t)(void *arg);

struct rtc_device {
    const char *name;
    int (*get_time)(struct rtc_device *rtc, struct rtc_time *time);
    int (*set_time)(struct rtc_device *rtc, const struct rtc_time *time);
    int (*set_alarm)(struct rtc_device *rtc, const struct rtc_time *time,
                     rtc_alarm_cb_t cb, void *arg);
    int (*clear_alarm)(struct rtc_device *rtc);
    void *priv;
};

int rtc_register(struct rtc_device *rtc);
struct rtc_device *rtc_get(const char *name);
int rtc_get_time(struct rtc_device *rtc, struct rtc_time *time);
int rtc_set_time(struct rtc_device *rtc, const struct rtc_time *time);
int rtc_set_alarm(struct rtc_device *rtc, const struct rtc_time *time,
                  rtc_alarm_cb_t cb, void *arg);
int rtc_clear_alarm(struct rtc_device *rtc);

u32 rtc_to_seconds(const struct rtc_time *time);
void rtc_from_seconds(u32 seconds, struct rtc_time *time);

#endif /* __DRIVER_RTC_H__ */
