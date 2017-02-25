#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CENTRY 19        /* current century */

char *increment_date(char *date);
#ifdef TEST_MAIN
main(int argc, char **argv)
{
    char date[200];

    strcpy(date,"0910111213");
    while(strcmp(date,"1006221541")) {
        printf("date going in: %s\n",date);
        increment_date(date);
        printf("date going OUT: %s\n",date);
    }
}
#endif
/*! \brief Tools used to gather/manipulate date information.
 *
 */
/*! \file date_tools.h */


char *long_date_to_short_date(char *datestr)
{
    char dayname[10], month[10], day[10], time[10], year[10], junk[5], *ptr;
    char months[13][5]= { "zero", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    char *small_date;


    int i = 0;

    if(ptr = strchr(datestr, '\n')) *ptr='\0';
//printf("ds = >%s<\n",datestr); gets(junk);

    sscanf(datestr,"%s %s %s %s %s", dayname, month, day, time, year);

    while(strcmp(months[i], month)) i++;
    small_date=(char *)malloc(20);

    sprintf(small_date,"%02d/%s/%s", i, day, &year[2]);
//    printf("sd out is >%s<\n",small_date);
    return small_date;
}

// yymmddhhmm
char *increment_date(char *date)
{
    char hour[10], month[10], day[10], minute[10], year[10], junk[5], *ptr;
    int  iyear, imonth, iday, ihour, iminute;
    char month_names[13][5]= { "zero", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static int days_in_month[] = {0, 31,28,31,30,31,30,31,31,30,31,30,31};
    char *small_date, *dateout;


    int i = 0;

    strncpy(year, date, 2); year[2]='\0'; iyear=atoi(year);
    strncpy(month, &date[2], 2); month[2]='\0'; imonth = atoi(month);
    strncpy(day, &date[4], 2); day[2]='\0'; iday = atoi(day);
    strncpy(hour, &date[6], 2); hour[2]='\0'; ihour = atoi(hour);
    strncpy(minute, &date[8], 2); minute[2]='\0'; iminute = atoi(minute);

    //dateout=malloc(11);

    if(++iminute > 59) {
        iminute = 0;
        if(++ihour > 23) {
            ihour = 0;
            if(++iday > days_in_month[imonth]) {
                iday = 1;
                if(++imonth > 12) {
                    imonth = 1;
                    ++iyear;
                }
            }
        }
    }
    sprintf(date,"%2.2d%2.2d%2.2d%2.2d%2.2d",iyear, imonth, iday, ihour, iminute);

    return date;
}


long gtoj(char *indate)
{
        static int monthd[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        int i;
        int leapd,iyr,imo,iday;
        static long cdays = 36524, ydays = 365;

        /* convert into expanded format if necessary */
        //if ((cvtdate(indate)) != 0) return(-1);

        /* parse gregorian date into its pieces */
        sscanf(indate,"%2d%*1c%2d%*1c%2d",&imo,&iday,&iyr); /*Add &'s, L.P.*/
//printf(">%s<\n",indate);
        /* adjust month array for leap year/non-leap year */
        if (iyr < 0 || iyr > 99) return(-1);
        if (iyr%4 == 0 && iyr != 0 || CENTRY%4 == 0)
                monthd[1] = 29;
        else
                monthd[1] = 28;

        /* check for invalid month */
        if (imo < 1 || imo > 12) return(-1);

        /* check for invalid day */
        if (iday < 1 || iday > monthd[imo-1]) return(-1);

        /* determine the number of "extra" leap years caused by the  */
        /* %400 criteria and add to it the number of leap years that */
        /* has occured up to the prior year of current century.      */
        leapd = CENTRY/4;
        if (iyr != 0) leapd += (iyr-1)/4;
//printf(">>%s<\n",indate);

        /* determine number of days elapsed in current year */
        for (i=0;i<(imo-1);i++)
                iday = iday + monthd[i];

        /* calculate julian date */
//printf(">>>>%s - %ld<\n",indate, (CENTRY*cdays + iyr*ydays + leapd + iday));
        return (CENTRY*cdays + iyr*ydays + leapd + iday);
}

#ifdef NEEDED
char *current_date()
{
    char buf[100], *buf1, *ptr;

    time_t now;
    time(&now);
    strcpy(buf,ctime(&now));
    if(ptr = strchr(buf, '\n')) *ptr='\0';
    //printf("buf is >%s<\n", buf);
    buf1=(char *)malloc(100);
    strcpy(buf1, long_date_to_short_date(buf));
    return buf1;
}
int days_elapsed(char *date_str)
{
    long d1, d2, elapsed;
    char today[100], date_reported[100];
    
    //printf("datestr is >%s<\n", date_str);
    strcpy(date_reported, long_date_to_short_date(date_str));
    strcpy(today,current_date());
    //printf("today is >%s<, date rep is >%s<\n", today, date_reported);
    
    d1 = gtoj(today);
    d2 = gtoj(date_reported);
    //printf("d1 >%ld<, d2 >%ld<\n", d1, d2);
    elapsed = d1 - d2;
    if(elapsed < 0) elapsed *= -1;
    //printf("days_3lapsed returns: >%ld<\n", elapsed);
    return elapsed;
}

#endif


