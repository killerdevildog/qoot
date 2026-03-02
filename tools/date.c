/*
 * date - print current date/time (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static const char *day_names[] = {"Thu","Fri","Sat","Sun","Mon","Tue","Wed"};
static const char *month_names[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                     "Jul","Aug","Sep","Oct","Nov","Dec"};
static const int month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static int is_leap(int y) { return (y%4==0 && y%100!=0) || (y%400==0); }

int date_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    struct timeval tv;
    if (sys_gettimeofday(&tv, NULL) < 0) {
        eprint("date: cannot get time\n");
        return 1;
    }

    long ts = tv.tv_sec;
    int dow = (int)((ts / 86400 + 4) % 7); /* Jan 1 1970 was Thursday (index 0) */
    if (dow < 0) dow += 7;

    /* Break down UTC */
    int days = (int)(ts / 86400);
    int time_of_day = (int)(ts % 86400);
    if (time_of_day < 0) { time_of_day += 86400; days--; }

    int hour = time_of_day / 3600;
    int min = (time_of_day % 3600) / 60;
    int sec = time_of_day % 60;

    int year = 1970;
    while (1) {
        int yd = 365 + is_leap(year);
        if (days < yd) break;
        days -= yd;
        year++;
    }

    int month = 0;
    for (month = 0; month < 12; month++) {
        int md = month_days[month] + (month == 1 && is_leap(year) ? 1 : 0);
        if (days < md) break;
        days -= md;
    }
    int day = days + 1;

    char buf[8];
    /* Day Mon DD HH:MM:SS UTC YYYY */
    print(day_names[dow]);
    print(" ");
    print(month_names[month]);
    print(" ");
    if (day < 10) print(" ");
    uint_to_str(buf, day); print(buf);
    print(" ");
    if (hour < 10) print("0");
    uint_to_str(buf, hour); print(buf);
    print(":");
    if (min < 10) print("0");
    uint_to_str(buf, min); print(buf);
    print(":");
    if (sec < 10) print("0");
    uint_to_str(buf, sec); print(buf);
    print(" UTC ");
    uint_to_str(buf, year); print(buf);
    print("\n");

    return 0;
}

QEMT_ENTRY(date_main)
