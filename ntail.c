#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulog.h"
#define SSV4 "%[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]"
#define SSV5 "%[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]"
#define SSV6 "%[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]"
#define QSV5 "%[^']%*[']  %[^']%*[']  %[^']%*[']  %[^']%*[']"
#define QSV6 "%[^']%*[']  %[^']%*[']  %[^']%*[']  %[^']%*[']  %[^']%*[']"
#define YELLOW 33
#define RED 31
#define GREEN 32
#define BLUE 34

#define UPDATE_JUST_SERVICE 1
#define UPDATE_HOST_AND_SERVICE 1
#define HOST 1
#define NAGIOS_LOG "/usr/local/nagios/var/nagios.log"
#define GROUNDWORK_LOG "/usr/local/groundwork/nagios/var/nagios.log"
#define SERVICE 2
#define MAX_TOTAL_HELD_MSGS 10000
int pretty_quotes(char *buf, char *pretty_buf);
int pretty_quotes1(char *buf, char *pretty_buf);

char *do_date(char *,char *);
char *lower_case(char *);
char mbuf[10000][300];
int  mcnt=0;
int tm=0;
char g_host_date[100], g_service_date[100];
int g_modified_host = 0, g_modified_service=0;

main(int argc, char **argv)
{
    char long_time[20], dummy[5];
    char buf[5000], sdate[5000], *sptr;
    char host_name[500], name[500], service_desc[500]; 
    int i = 0, j = 0, update_host=0, pause=0;
    int process_entire_log=0, reached_the_end_of_log=0;
    int previous_line_was_CHECK_RESULT=0, nog_needs_reload = 0;
    FILE *fp_log;
    char last_time[20], pretty_buf[5000], the_match[200]= "xyxyxy";

    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]
    slog("-------------- process starting -------v5.0----\n");

    if(argc < 2 || !strcmp(argv[1],"-m")) { 
	sprintf (buf, "tail -f %s", NAGIOS_LOG);
        if(!(fp_log = popen(buf,"r"))) { 
            printf("noop\n"); 
            slog("Can NOT open %s\n",NAGIOS_LOG);
	    sprintf (buf, "tail -f %s", NAGIOS_LOG);
            if(!(fp_log = popen(buf,"r"))) { 
                slog("Can NOT popen %s\n",NAGIOS_LOG);
                exit(3);
            }
        } 
        printf("popend tail -f %s\n", NAGIOS_LOG);
        strcpy(the_match,argv[2]);
        if(argc==4) pause=1;
    } else {
        sprintf(buf,"tail -f %s", argv[1]);
        if(!(fp_log = popen(buf,"r"))) { 
            slog("Can NOT popen %s\n",argv[1]);
            exit(3);
        } 
    }

    slog("Going into the BIG loop...\n");
    while(1) {
        while (fgets(buf, 4999, fp_log)) {
            strncpy(long_time,&buf[1],10); long_time[10]='\0';
            do_date(long_time,sdate);
            make_buf_pretty(&buf[13], pretty_buf);
            //strcat(sdate,&buf[13]);
            strcat(sdate,pretty_buf);
            if(strstr(sdate,the_match)) {
                show_match(sdate, the_match);
                printf("%s",sdate);
                if(pause) fgets(dummy, 4, stdin);
            } else printf("%s",sdate);
        }
        sleep(2);
    }
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
    sprintf(sdate,"[34m[%02d-%02d %02d:%02d:%02d][0m ", newtime->tm_mon+1, 
        newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

/*     sprintf(sdate,"%.19s, %i ", asctime(newtime),  1900+newtime->tm_year);*/
     return sdate;
}


int make_buf_pretty(char *buf, char *pretty_buf)
{
    char pretty_quotes_str[500], pretty_buf1[500];

    if(strstr(buf,"PASSIVE SERVICE CHECK:")) {
        pretty_fields(&buf[23], pretty_buf1);
        if(strstr(buf,"OK"))
            sprintf(pretty_buf,"[32mPASSIVE SERVICE CHECK:[0m %s",
               pretty_buf1);
        else
            sprintf(pretty_buf,"[31mPASSIVE SERVICE CHECK:[0m %s",
               pretty_buf1);
    } else if(strstr(buf,"SERVICE ALERT:")) {
        pretty_fields1(&buf[15], pretty_buf1);
        if(strstr(buf,"OK"))
            sprintf(pretty_buf,"[32mSERVICE ALERT:[0m %s",
               pretty_buf1);
        else
            sprintf(pretty_buf,"[31mSERVICE ALERT:[0m %s",
               pretty_buf1);
    } else if(strstr(buf,"HOST ALERT:")) {
        pretty_fields2(&buf[12], pretty_buf1);
        if(strstr(buf,";UP;"))
            sprintf(pretty_buf,"[32mHOST ALERT:[0m %s",
               pretty_buf1);
        else
            sprintf(pretty_buf,"[31mHOST ALERT:[0m %s",
               pretty_buf1);
    } else if(strstr(buf,"CURRENT HOST STATE:")) {
        pretty_fields2(&buf[20], pretty_buf1);
        if(strstr(buf,"UP"))
            sprintf(pretty_buf,"[32mCURRENT HOST STATE:[0m %s",
               pretty_buf1);  
        else
            sprintf(pretty_buf,"[31mCURRENT HOST STATE:[0m %s",
               pretty_buf1);  
    } else if(strstr(buf,"CURRENT SERVICE STATE:")) {
        pretty_fields1(&buf[23], pretty_buf1);
        if(strstr(buf,";OK;"))
            sprintf(pretty_buf,"[32mCURRENT SERVICE STATE:[0m %s",
               pretty_buf1);
        else
            sprintf(pretty_buf,"[31mCURRENT SERVICE STATE:[0m %s",
               pretty_buf1);
    } else if(strstr(buf,"EXTERNAL COMMAND: PROCESS_SERVICE_CHECK_RESULT")){
        pretty_fields(&buf[47], pretty_buf1);
        if(strstr(buf,"OK"))
            sprintf(pretty_buf,"EXTERNAL COMMAND: [32mPROCESS_SERVICE_CHECK_RESULT[0m;%s",
               pretty_buf1);
        else
            sprintf(pretty_buf,"EXTERNAL COMMAND: [31mPROCESS_SERVICE_CHECK_RESULT[0m;%s",
               pretty_buf1);  
    } else if(strstr(buf,"Warning:  Passive check result")) {
        pretty_quotes1(&buf[9],pretty_quotes_str);
        sprintf(pretty_buf,"[31mWarning:[0m %s", pretty_quotes_str);
    } else if(strstr(buf,"Warning: Return code of 127")) {
        pretty_quotes1(buf,pretty_quotes_str);
        sprintf(pretty_buf,"[31mWarning:[0m %s", pretty_quotes_str);
        //sprintf(pretty_buf,"[31mWarning:[0m %s", &buf[9]);
        strcat(pretty_buf,"'r using actually exists bozo..\n");
    } else strcpy(pretty_buf,buf);
}

int pretty_quotes(char *buf, char *pretty_buf)
{
    char *bptr, *pptr, b1[500], b2[500], b3[500], b4[500], b5[500];
    sscanf(buf,QSV5, b1, b2, b3, b4, b5);
    sprintf(pretty_buf,"%s '[31m%s[0m' %s '[31m%s[0m' %s\n",
        b1, b2, b3, b4, b5);
}

int pretty_quotes1(char *buf, char *pretty_buf)
{
    char *bptr, *pptr, b1[500], b2[500], b3[500], b4[500], b5[500], b6[500];
    sscanf(buf,QSV6, b1, b2, b3, b4, b5, b6);
    sprintf(pretty_buf,"%s\n    '[31m%s[0m' %s'[31m%s[0m'%s",
        b1, b2, b3, b4, b5);
}
int pretty_fields(char *buf, char *pretty_buf)
{
    char *bptr, *pptr, b1[500], b2[500], b3[500], b4[500], b5[500], b6[500];
    int error_val_color = 41;
    sscanf(buf,SSV4, b1, b2, b3, b4);
    if(atoi(b1) == 0) error_val_color = GREEN;
    else if(atoi(b1) == 1) error_val_color = YELLOW;
    else if(atoi(b1) == 2) error_val_color = RED;
    else error_val_color = BLUE;
    sprintf(pretty_buf,"[33m%s[0m;[35m%s[0m;[%dm%s[0m;%s", 
       b1, b2, error_val_color, b3, b4);
}
int pretty_fields2(char *buf, char *pretty_buf)
{
    char *bptr, *pptr, b1[500], b2[500], b3[500], b4[500], b5[500], b6[500];
    int error_val_color = 41;
//printf("in\n");
    sscanf(buf,SSV5, b1, b2, b3, b4, b5);
    if(strstr(b2,"DOWN")) error_val_color = RED;
    else error_val_color = GREEN;
    sprintf(pretty_buf,"[33m%s[0m;[%dm%s[0m;[34m%s[0m;[36m%s[0m;%s", b1, error_val_color, b2, b3, b4, b5);
//printf("out\n");
}
int pretty_fields1(char *buf, char *pretty_buf)
{
    char *bptr, *pptr, b1[500], b2[500], b3[500], b4[500], b5[500], b6[500];
    int error_val_color = 41;
    sscanf(buf,SSV6, b1, b2, b3, b4, b5, b6);
    if(strstr(b3,"CRITICAL")) error_val_color = RED;
    else error_val_color = GREEN;
    sprintf(pretty_buf,"[33m%s[0m;[35m%s[0m;[%dm%s[0m;[34m%s[0m;[36m%s[0m;%s", b1, b2, error_val_color, b3, b4, b5, b6);
}
int show_match(char *buf, char *match)
{
    char *bptr, *sptr, *nptr, newb[5000], ms[5000];
    int i, slen;
    bptr=buf;
    nptr=newb;

    sptr=strstr(buf,match);

    while(bptr != sptr) {*nptr=*bptr; ++nptr; ++bptr;}
    *nptr='\0';
    strcat(newb,"[47m");
    slen=strlen(match);
    nptr=ms;
    for(i=0; i<slen; i++) {
       *nptr=*sptr; 
       ++sptr; ++nptr;
    }
    *nptr='\0';
    strcat(newb,ms);
    strcat(newb,"[0m");
    strcat(newb,sptr);
    strcpy(buf,newb);
}
