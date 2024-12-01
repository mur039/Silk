#ifndef __CMOS_H__
#define __CMOS_H__

#include <sys.h>
#include <stdint.h>

#define CMOS_PORT_ADDRESS 0x70
#define CMOS_PORT_DATA 0x71
/*
setting register on 0x70 and reading result from 0x71
defaults selected register to 0xd
 */
#define NMI_BIT_SHIFT 7



/*
for RTC:
Register  Contents            Range
 0x00      Seconds             0–59
 0x02      Minutes             0–59
 0x04      Hours               0–23 in 24-hour mode, 
                               1–12 in 12-hour mode, highest bit set if pm
 0x06      Weekday             1–7, Sunday = 1
 0x07      Day of Month        1–31
 0x08      Month               1–12
 0x09      Year                0–99
 0x32      Century (maybe)     19–20?
 0x0A      Status Register A
 0x0B      Status Register B
*/

#define CMOS_SEC_REG 0x00
#define CMOS_MIN_REG 0x02
#define CMOS_HOUR_REG 0x04
#define CMOS_WEEK_DAY_REG 0x06
#define CMOS_DAY_OF_MONTH_REG 0x07
#define CMOS_MONTH_REG 0x08
#define CMOS_YEAR_REG 0x09
#define CMOS_CENTURY_REG 0x32
#define CMOS_STATUS_A_REG 0x0A
#define CMOS_STATUS_B_REG 0x0B

u8 cmos_read_register(u8 _register);
static void cmos_set_register(u8 _register);

#endif