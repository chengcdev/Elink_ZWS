/*
 * time.h
 *
 *  Created on: 2018年10月29日
 *      Author: chengc
 */

#ifndef APPLICATION_MLINKDEMO_TIME_H_
#define APPLICATION_MLINKDEMO_TIME_H_

#define UTC_BASE_YEAR 1970
#define MONTH_PER_YEAR 12
#define DAY_PER_YEAR 365
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60


/* 自定义的时间结构体 */
typedef struct {
    unsigned short nYear;
    unsigned char nMonth;
    unsigned char nDay;
    unsigned char nHour;
    unsigned char nMin;
    unsigned char nSec;
    unsigned char DayIndex; /* 0 = Sunday */
} mytime_struct;

void utc_sec_2_mytime(unsigned int utc_sec, mytime_struct *result, bool daylightSaving);
unsigned int mytime_2_utc_sec(mytime_struct *currTime, bool daylightSaving);
#endif /* APPLICATION_MLINKDEMO_TIME_H_ */
