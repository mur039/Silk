#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>


const int days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

const char* month_string[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

const char* day_string[] = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};


int is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

void unix_to_utc(uint32_t epoch, int* year, int* month, int* day,
                 int* hour, int* minute, int* second) {
    uint32_t days = epoch / 86400;
    uint32_t secs = epoch % 86400;

    *hour = secs / 3600;
    secs %= 3600;
    *minute = secs / 60;
    *second = secs % 60;

    int y = 1970;
    while (1) {
        int days_in_year = is_leap(y) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            y++;
        } else {
            break;
        }
    }

    *year = y;
    int m = 0;
    while (1) {
        int dim = days_in_month[m];
        if (m == 1 && is_leap(y)) dim++; // February leap year

        if (days >= dim) {
            days -= dim;
            m++;
        } else {
            break;
        }
    }

    *month = m + 1;
    *day = days + 1;
}

int dayofweek(int y, int m, int d) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

void format_unix_time(uint32_t epoch, char* buf) {
    int year, month, day, hour, minute, second;
    unix_to_utc(epoch, &year, &month, &day, &hour, &minute, &second);
    sprintf(buf, "%s %d %s %d %d:%d:%d +0",
            day_string[dayofweek(year, month, day) - 1], day, month_string[month - 1], year, hour, minute, second);
}

int main(){
 
    uint32_t loc;
    time((time_t*)&loc);
    char lbuff[48];
    format_unix_time(loc, lbuff);
    printf(lbuff);
    putchar('\n');
    return 0;
}