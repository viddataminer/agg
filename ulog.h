#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#define ERROR 0
#define WARN 1
#define INFO 2
#define MAX_LOG_FILE_SIZE 3000000

//#define LOG_PATH "/var/log/"
#define LOG_PATH "/usr/local/nagios/var/"

char  VERXXX[10]="VER300";
FILE *log_ptr;
char raw_app_name[100];
char les_file_with_path[200];
char tmp_file_with_path[200];
void slog( char *, ... );

/*! \brief Library for log.
 *
 * Gathers OV node information and preforms the following tasks:
 * - if the node DOES NOT exists in NNM, then
 *   - create file for loadhosts command
 *   - and, add the node to NNM
 * - if the node DOES NOT exists in OVO, then
 *   - find similar group information
 *   - add node to OVO: opcnode -add_node
 *   - add to all approiate groups: opcnode -update_node
 * - else agent is in NNM and OVO, so
 *   - check to see if agent communicates
 *   - if the agent does not communicate then
 *     -  send log_event info about agent not communicating
 *   - otherwise, agent does communicate so
 *     -   push template
 * .
 * Returns 1 if successful, 0 if not.
 */
/*! \file log.c */


int open_log_file(char *pname)
{

    char filename_with_path[200], cmd[300], *ptr, *lptr, *sptr;
    char  event_txt[500], error_loc[100],dummy[100];

    char prog_name[200], bk_filename_with_path[200];
    long log_file_size = 0;
    FILE *fp;

    //fp=popen("uname -a | cut -f2 -d' '","r");
    //fgets(hostname, 99, fp);
    //fgets(dummy, 99, fp);
    //pclose(fp);

    strcpy(prog_name, pname);

    if(sptr = (char *)strstr(prog_name,"./")) strcpy(prog_name,&pname[2]);
    else strcpy(prog_name, pname);

    if(sptr = (char *)strchr(prog_name,'.')) *sptr = '\0';
    ptr=prog_name;

    sprintf(error_loc,"/tmp");
    if(!(ptr=(char *)strchr(ptr,'/'))) {
        sprintf(filename_with_path,"%s%s.log", LOG_PATH, prog_name);
        strcpy(raw_app_name, prog_name);
        sprintf(bk_filename_with_path,"%s%s.log.bk", LOG_PATH,raw_app_name);
    } else {
        while(ptr=(char *)strchr(ptr,'/'))
            lptr = ptr++;
        ++lptr;
        strcpy(raw_app_name, lptr);
        sprintf(filename_with_path,"%s%s.log", LOG_PATH,raw_app_name);
        sprintf(bk_filename_with_path,"%s%s.log.bk", LOG_PATH,raw_app_name);
    }

    if (!(log_ptr = fopen(filename_with_path, "a"))) {
        sprintf(cmd,"echo \"Can not fopen. Path >%s<\" > %s/%s.err", 
                  filename_with_path, error_loc, raw_app_name);
        system(cmd);
        exit(1);
    }
    fseek (log_ptr, 0, SEEK_END);
    log_file_size=ftell(log_ptr);
    fseek (log_ptr, 0, SEEK_SET);
    if(log_file_size > MAX_LOG_FILE_SIZE) {
        fclose(log_ptr);
        sprintf(cmd, "cp %s %s", filename_with_path, bk_filename_with_path);
        system(cmd);
        sprintf(cmd, "rm -f %s", filename_with_path);
        system(cmd);
        if (!(log_ptr = fopen(filename_with_path, "a"))) {
            sprintf(cmd,"echo \"Can not fopen. Path >%s<\" > %s/%s.err", 
                  filename_with_path, error_loc, raw_app_name);
            system(cmd);
            exit(1);
        }
    } 
}


char *ts(char *buf)
{
    char *ret_buf;
    char *pret;
    char *no_fmt_date;

    struct tm *tptr;
    time_t tm;
    tm = time(NULL);
    tptr = localtime(&tm);

    no_fmt_date = asctime(tptr);
    no_fmt_date[strlen(no_fmt_date)-1] = '\0';
    //if(no_format) return no_fmt_date;

    //ret_buf=(char *)malloc(100);
    ret_buf=buf;

    sprintf(ret_buf,"%02d-%02d %02d:%02d:%02d", tptr->tm_mon+1, tptr->tm_mday,
       tptr->tm_hour, tptr->tm_min, tptr->tm_sec);

    return ret_buf;
}
#ifdef NEEDED
char *ts_hhmm()
{
    char *ret_buf;
    char *no_fmt_date;

    struct tm *tptr;
    time_t tm;
    tm = time(NULL);
    tptr = localtime(&tm);

    no_fmt_date = asctime(tptr);
    no_fmt_date[strlen(no_fmt_date)-1] = '\0';
    //if(no_format) return no_fmt_date;

    ret_buf=(char *)malloc(100);

    sprintf(ret_buf,"%02d%02d", tptr->tm_hour, tptr->tm_min);

    return ret_buf;
}
#endif
/* eprintf( format, args ) */
void slog( char *fmt, ... )
{
    va_list     list;
    char      *p, *r, buf[20];
    int       e;
    
    //FILE *log_ptr;
    //log_ptr=fopen("testy.txt", "w");
    if(!log_ptr) return;
    fprintf(log_ptr,"%s: ", ts(buf));
    fprintf(stdout,"%s: ", ts(buf));

    /* prepare list for va_arg */
    va_start( list, fmt );

    for ( p = fmt ; *p ; ++p ) {
        /* check if we should later look for *
         * i (integer) or s (string) */
        if ( *p != '%' ) {
            /* not a string or integer print *
             * the character to log_ptr */
            putc( *p, log_ptr );
            putc( *p, stdout );
        } else {
            /* character was % so check the *
             * letter after it and see if it's *
             * one of s or i */
            switch ( *++p ) {
                /* string */
                case 's':
                {
                    /* set r as the next char *
                     * in list (string) */
                    r = va_arg( list, char * );

                    /* print results to log_ptr */
                    fprintf(log_ptr,"%s", r);
                    fprintf(stdout,"%s", r);
                    continue;
                }

                /* integer */
                case 'i':
                case 'd':
                {
                    /* set e as the next char *
                     * in list (integer) */
                    e = va_arg( list, int );

                    /* print results to log_ptr */
                    fprintf(log_ptr,"%i", e);
                    fprintf(stdout,"%i", e);
                    continue;
                }

                case 'u':
                {
                    /* set e as the next char *
                     * in list (integer) */
                    e = va_arg( list, int );

                    /* print results to log_ptr */
                    fprintf(log_ptr,"%u", e);
                    fprintf(stdout,"%u", e);
                    continue;
                }

                default: putc( *p, log_ptr );      //putc( *p, stdout );
            }
        }
    }
    va_end( list );
    fflush(log_ptr);
    fflush(stdout);
}

char *upper_case(char *buf)
{
    int i, len;
    char tmpbuf[2001];
    char *ptr;

    //tmpbuf = (char *) malloc(2000);
    len=strlen(buf);

    for (i=0; i<len && buf[i]; i++) {
        if(islower(buf[i])) tmpbuf[i] = toupper(buf[i]);
        else tmpbuf[i] = buf[i];
    }
    tmpbuf[i] = '\0';
    strcpy(buf,tmpbuf);
    ptr=buf;
    return (ptr);
}

char *lower_case(char *buf)
{
    int i, len;
    char tmpbuf[2001]; 
    char *ptr;

    //tmpbuf = (char *) malloc(2000);
    len=strlen(buf);

    for (i=0; i<len && buf[i]; i++) {
        if(isupper(buf[i])) tmpbuf[i] = tolower(buf[i]);
        else tmpbuf[i] = buf[i];
    }
    tmpbuf[i] = '\0';
    strcpy(buf,tmpbuf);
    ptr=tmpbuf;
    return (ptr);
}

FILE *open_file(char *file, char *r_or_w) 
{
    FILE *fp;

    if(!(fp = fopen(file,r_or_w))) { 
        printf("Can NOT fopen %s\n", file);
        exit(1);
    }
    return fp;
}

int parse(char *buf, char *delim, char flds[50][100])
{
    char *ptr=buf;
    char *sptr;
    int fld_cnt=0;

    sptr=strtok(buf,delim);
    if(sptr==NULL) {
        strcpy(flds[fld_cnt],buf);
        return ++fld_cnt;
    }
    strcpy(flds[fld_cnt++],sptr);
    while (sptr=(char*)strtok(NULL,delim))
        strcpy(flds[fld_cnt++],sptr);
    return fld_cnt;
}
 
int elapsed_time_in_minutes(char *start_time, char *end_time)
{
    char sstartmin[5], sstarthr[5], sendmin[5], sendhr[5];
    int result, startmin, starthr, endmin, endhr, start=0, end=0, curr=0;

    sstarthr[0] = start_time[0];
    sstarthr[1] = start_time[1];
    sstartmin[2] = '\0';
    sstartmin[0] = start_time[3];
    sstartmin[1] = start_time[4];
    sstartmin[2] = '\0';

    sendhr[0] = end_time[0];
    sendhr[1] = end_time[1];
    sendmin[2] = '\0';
    sendmin[0] = end_time[3];
    sendmin[1] = end_time[4];
    sendmin[2] = '\0';

    startmin=atoi(sstartmin);
    starthr=atoi(sstarthr);
    start=startmin+60*starthr;
    endmin=atoi(sendmin);
    endhr=atoi(sendhr);
    end=endmin+60*endhr;

    if(start > end) end += (24*60);

    result = end - start;

    return result;
}

char *ts_hhmmss()
{
    char *ret_buf, *no_fmt_date;
    static char buf[100];

    struct tm *tptr;
    time_t tm;
    tzset();
    tm = time(NULL);
    tptr = localtime(&tm);

    no_fmt_date = asctime(tptr);
    no_fmt_date[strlen(no_fmt_date)-1] = '\0';
    sprintf(buf,"%02d:%02d:%02d", tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return (buf);
}



