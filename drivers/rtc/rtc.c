#include <driver/rtc.h>

static struct rtc_device *rtc_dev;

int rtc_register(struct rtc_device *rtc)
{
    if (!rtc || !rtc->get_time || !rtc->set_time)
        return -EINVAL;
    rtc_dev = rtc;
    return 0;
}

struct rtc_device *rtc_get(const char *name)
{
    if (rtc_dev && (!name || name[0] == '\0'))
        return rtc_dev;
    return rtc_dev;
}

int rtc_get_time(struct rtc_device *rtc, struct rtc_time *time)
{
    if (!rtc || !time)
        return -EINVAL;
    return rtc->get_time(rtc, time);
}

int rtc_set_time(struct rtc_device *rtc, const struct rtc_time *time)
{
    if (!rtc || !time || time->month < 1 || time->month > 12 ||
        time->day < 1 || time->day > 31 || time->hour > 23 ||
        time->minute > 59 || time->second > 59)
        return -EINVAL;
    return rtc->set_time(rtc, time);
}

int rtc_set_alarm(struct rtc_device *rtc, const struct rtc_time *time,
                  rtc_alarm_cb_t cb, void *arg)
{
    if (!rtc || !time || !rtc->set_alarm)
        return -ENOSYS;
    return rtc->set_alarm(rtc, time, cb, arg);
}

int rtc_clear_alarm(struct rtc_device *rtc)
{
    if (!rtc || !rtc->clear_alarm)
        return -ENOSYS;
    return rtc->clear_alarm(rtc);
}

/* Days since 1970-01-01, valid for the ranges used by a watch. */
static s32 days_from_civil(s32 y, s32 m, s32 d)
{
    y -= m <= 2;
    s32 era = (y >= 0 ? y : y - 399) / 400;
    u32 yoe = (u32)(y - era * 400);
    u32 doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    u32 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (s32)doe - 719468;
}

static void civil_from_days(s32 z, s32 *y, s32 *m, s32 *d)
{
    z += 719468;
    s32 era = (z >= 0 ? z : z - 146096) / 146097;
    u32 doe = (u32)(z - era * 146097);
    u32 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    s32 yy = (s32)yoe + era * 400;
    u32 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    u32 mp = (5 * doy + 2) / 153;
    u32 dd = doy - (153 * mp + 2) / 5 + 1;
    u32 mm = mp + (mp < 10 ? 3 : -9);
    *y = yy + (mm <= 2);
    *m = (s32)mm;
    *d = (s32)dd;
}

u32 rtc_to_seconds(const struct rtc_time *time)
{
    s32 days;

    if (!time || time->month < 1 || time->month > 12)
        return 0;

    days = days_from_civil(time->year, time->month, time->day);
    return (u32)(days * 86400 + time->hour * 3600 +
                 time->minute * 60 + time->second);
}

void rtc_from_seconds(u32 seconds, struct rtc_time *time)
{
    s32 days;
    u32 rem;
    s32 y, m, d;

    if (!time)
        return;

    days = (s32)(seconds / 86400);
    rem = seconds % 86400;
    civil_from_days(days, &y, &m, &d);

    time->year = (u16)y;
    time->month = (u8)m;
    time->day = (u8)d;
    time->hour = (u8)(rem / 3600);
    time->minute = (u8)((rem % 3600) / 60);
    time->second = (u8)(rem % 60);
}
