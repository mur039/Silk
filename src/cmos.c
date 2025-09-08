#include <cmos.h>
#include <stdint-gcc.h>


static void cmos_set_register(u8 _register){
    outb( 
        CMOS_PORT_ADDRESS,
        (0 << NMI_BIT_SHIFT) | ( _register & ~(1 << NMI_BIT_SHIFT) )
        );
}


u8 cmos_read_register(u8 _register){
    cmos_set_register(_register);
    return inb(CMOS_PORT_DATA);
}

uint8_t cmos_read(uint16_t regi){
    return cmos_read_register(regi);
}


// Convert from BCD to binary
static inline uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val >> 4) * 10);
}




void read_rtc(int *year, int *month, int *day,
                     int *hour, int *minute, int *second) {
    uint8_t regA, century = 0, y, mon, d, h, min, s;
    uint8_t statusB;

    // Wait until not updating
    do {
        outb(0x70, 0x0A);
        regA = inb(0x71);
    } while (regA & 0x80);

    s   = cmos_read(0x00);
    min = cmos_read(0x02);
    h   = cmos_read(0x04);
    d   = cmos_read(0x07);
    mon = cmos_read(0x08);
    y   = cmos_read(0x09);
    century = cmos_read(0x32); // if available

    statusB = cmos_read(0x0B);

    // Convert BCD to binary if needed
    if (!(statusB & 0x04)) {
        s   = bcd_to_bin(s);
        min = bcd_to_bin(min);
        h   = bcd_to_bin(h);
        d   = bcd_to_bin(d);
        mon = bcd_to_bin(mon);
        y   = bcd_to_bin(y);
        if (century) century = bcd_to_bin(century);
    }

    // Convert 12-hour clock to 24-hour if needed
    if (!(statusB & 0x02) && (h & 0x80)) {
        h = ((h & 0x7F) + 12) % 24;
    }

    *second = s;
    *minute = min;
    *hour   = h;
    *day    = d;
    *month  = mon;
    *year   = (century ? century * 100 : 2000) + y;
}


// Check if leap year
static int is_leap(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// Days before each month
static const int days_before_month[] = {
    0,   // Jan
    31,  // Feb
    59,  // Mar
    90,  // Apr
    120, // May
    151, // Jun
    181, // Jul
    212, // Aug
    243, // Sep
    273, // Oct
    304, // Nov
    334  // Dec
};

uint32_t to_unix_timestamp(int year, int month, int day,
                           int hour, int minute, int second) {
    // Days since epoch
    uint64_t days = 0;

    // Years
    for (int y = 1970; y < year; y++) {
        days += is_leap(y) ? 366 : 365;
    }

    // Months
    days += days_before_month[month - 1];
    if (month > 2 && is_leap(year)) days++;

    // Days
    days += day - 1;

    // Convert to seconds
    uint32_t ts = days * 86400ULL
                + hour * 3600ULL
                + minute * 60ULL
                + second;

    return ts;
}
