#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulog.h"

#define UPDATE_JUST_SERVICE 1
#define UPDATE_HOST_AND_SERVICE 1
#define HOST 1
#define SERVICE 2
#define CFG_PATH "/usr/local/nagios/etc/objects"
#define MAX_TOTAL_HELD_MSGS 10000

char *do_date(char *,char *);
char *lower_case(char *);
char mbuf[10000][300];
int  mcnt=0;
int tm=0;
char g_host_date[100], g_service_date[100];
int g_modified_host = 0, g_modified_service=0;

main(int argc, char **argv)
{
    char long_time[20];
    char buf[5000], sdate[5000], *sptr;
    char host_name[500], name[500], service_desc[500]; 
    int i = 0, j = 0, update_host=0;
    int process_entire_log=0, reached_the_end_of_log=0;
    int previous_line_was_CHECK_RESULT=0, nog_needs_reload = 0;
    FILE *fp_log, *fpo;
    char last_time[20];

    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]
    slog("-------------- process starting -------v5.0----\n");
    if(argc == 2) {
        if(!(fp_log = fopen(argv[1],"r"))) { 
            slog("Can NOT popen the nog log\n");
            exit(3);
        } 
    } else if(!(fp_log = fopen("/usr/local/nagios/var/nagios.log","r"))) { 
        slog("Can NOT popen the nog log\n");
        exit(3);
    } 
    if(!(fpo = fopen("/tmp/nagloggy.cfg","w"))) { 
        slog("Can NOT popen the nog log\n");
        exit(3);
    } 

    slog("Going into the BIG loop...\n");
    while (fgets(buf, 4999, fp_log)) {
        strncpy(long_time,&buf[1],10); long_time[10]='\0';
        do_date(long_time,sdate);
        strcat(sdate,&buf[13]);
        fprintf(fpo,sdate);
    } fclose(fpo);
    system("vim /tmp/nagloggy.cfg");
}

 
char *do_date(char *argv, char *sdate)
{
     struct tm *newtime;
     char am_pm[] = "AM";
     time_t long_time;

     long_time=atol(argv);
 
     newtime = localtime(&long_time);
/*
     if(newtime->tm_hour>12)
          strcpy(am_pm,"PM");
     if(newtime->tm_hour>12)
          newtime->tm_hour-=12;
     if(newtime->tm_hour ==0)
          newtime->tm_hour=12;
*/ 
    sprintf(sdate,"[%02d-%02d %02d:%02d:%02d] ", newtime->tm_mon+1, 
        newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

/*     sprintf(sdate,"%.19s, %i ", asctime(newtime),  1900+newtime->tm_year);*/
     return sdate;
}
