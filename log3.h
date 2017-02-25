#include <stdio.h> 
#include <stdarg.h> 
#include <string.h> 
#include <time.h> 

#define REMOVE_NEWLINE(buf) \
    if(sptr = strchr(buf,'\n')) *sptr = '\0'; \
    if(sptr = strchr(buf,'\r')) *sptr = '\0'

char  VERXXX[10]="VER300"; 
FILE *log_ptr; 
char raw_app_name[100]; 
char les_file_with_path[200]; 
char tmp_file_with_path[200]; 
int global_debug = 0;
void dbug( char *, ... );
void slog( char *, ... );

#define MAX_LOG_FILE_SIZE 10000000
#define ERROR 2
#define WARN 1
#define INFO 0

#ifndef CFG_PATH
#define CFG_PATH "/smg/cfg"
#endif

#ifndef LOG_PATH 
#define LOG_PATH "/smg/log"
#endif

extern int num_error_strs;
extern int num_errors;
extern char errors[10][500];
//extern struct cur;
int g_send_nsca=0;
char g_nsca_host[100];
struct st1 { char process[200]; char info[200]; int state; } cur[100];
/*
char g_node[200];
char g_domain[200];
char g_service[200];
char g_appliance_name[200];
*/
char setname[100] = "";
/*! \brief Library for log.
 *
 */
/*! \file wmi_info_log.c */

#define CFG_PATH "/smg/cfg"
#define BIN_PATH "/smg/bin"
#define LOG_PATH "/smg/log"
#define LES_PATH "/smg/les"

int open_log_file(char *pname, char *cfg_file_with_path, char *node)
{

    char filename_with_path[200], cmd[300], *ptr, *lptr, *sptr;
    char  event_txt[500];

    char prog_name[200], bk_filename_with_path[200];
    long log_file_size = 0;

    if(log_ptr) fclose(log_ptr);

    if(!strncmp(pname,"./",2)) strcpy(prog_name, &pname[2]);
    else strcpy(prog_name, pname);

    if((sptr = (char *)strchr(prog_name,'.'))) *sptr = '\0';
    ptr=prog_name;

    if(!(ptr=(char *)strchr(ptr,'/'))) {
        sprintf(filename_with_path,"%s/%s_%s.log",LOG_PATH, node, prog_name);
        if(!*cfg_file_with_path)
            sprintf(cfg_file_with_path,"%s/%s.cfg",CFG_PATH, node, "wmi_info");
        sprintf(les_file_with_path,"%s/%s_%s.les",LES_PATH, node, prog_name);
        sprintf(tmp_file_with_path,"%s/%s_%s.tmp",LES_PATH, node, prog_name);
        strcpy(raw_app_name, prog_name);
    } else {
        while(ptr=(char *)strchr(ptr,'/'))
            lptr = ptr++;
        ++lptr;
        strcpy(raw_app_name, lptr);
        if(!*cfg_file_with_path)
            sprintf(cfg_file_with_path,"%s/%s.cfg",CFG_PATH, node, "wmi_info");
        sprintf(filename_with_path,"%s/%s_%s.log",LOG_PATH,node,raw_app_name);
        sprintf(les_file_with_path,"%s/%s_%s.les",LES_PATH,node,raw_app_name);
        sprintf(tmp_file_with_path,"%s/%s_%s.tmp",LES_PATH,node,raw_app_name);

        sprintf(bk_filename_with_path,"%s/%s_%s.log.bk",
                LOG_PATH, node, raw_app_name);
    }

    if (!(log_ptr = fopen(filename_with_path, "r"))) {
        slog("Initializing Log for New SOURCE: %s", raw_app_name);
        //send_nsca_msg(event_txt, raw_app_name, "open_log", INFO);
    } else fclose(log_ptr);

    if (!(log_ptr = fopen(filename_with_path, "a"))) {
        slog("ABORTING: Can not fopen. Path >%s<\"", filename_with_path);
        //send_nsca_msg(cmd, raw_app_name, "open_log", ERROR);
        exit(1);
    }
    fseek (log_ptr, 0, SEEK_END);
    log_file_size=ftell(log_ptr);
    fseek (log_ptr, 0, SEEK_SET);
    if(log_file_size > MAX_LOG_FILE_SIZE) {
        fclose(log_ptr);
        printf("BACKUPLOG: (%s)\n", filename_with_path);
        sprintf(cmd, "cp %s %s 2>&1", filename_with_path, bk_filename_with_path);

        printf("going to system(%s)\n", cmd);
        system(cmd);

        //sprintf(cmd, "del %s 2>&1", filename_with_path);
        //printf("going to system(%s)\n", cmd);
        //system(cmd);
        if (!(log_ptr = fopen(filename_with_path, "w"))) {
            printf("ABORTING: BACKUP: CAN NOT OPEN LOG, NO fopen. (%s)\n", filename_with_path);
            exit(1);
        }
    } 
    return 0;
}

char *ts()
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

    sprintf(ret_buf,"%02d-%02d %02d:%02d:%02d", tptr->tm_mon+1, tptr->tm_mday,
       tptr->tm_hour, tptr->tm_min, tptr->tm_sec);

    return ret_buf;
}

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

/* eprintf( format, args ) */
void slog( char *fmt, ... )
{
    va_list     list;
    char      *p, *r;
    int       e;
    //FILE *log_ptr;
    //log_ptr=fopen("testy.txt", "w");
    if(!log_ptr) return;
    fprintf(log_ptr,"%s: ", ts());
#ifdef DEBUG
    fprintf(stdout,"%s: ", ts());
#endif
    /* prepare list for va_arg */
    va_start( list, fmt );

    for ( p = fmt ; *p ; ++p ) {
        /* check if we should later look for *
         * i (integer) or s (string) */
        if ( *p != '%' ) {
            /* not a string or integer print *
             * the character to log_ptr */
            putc( *p, log_ptr );
#ifdef DEBUG
            putc( *p, stdout );
#endif
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
#ifdef DEBUG
                    fprintf(stdout,"%s", r);
#endif
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
#ifdef DEBUG
                    fprintf(stdout,"%i", e);
#endif
                    continue;
                }

                default: putc( *p, log_ptr ); 
#ifdef DEBUG     
				  putc( *p, stdout );
#endif
            }
        }
    }
    va_end( list );
    fflush(log_ptr);
#ifdef DEBUG
    fflush(stdout);
#endif
}

/*
 * END W_LOG.H
*/
/*
#include "w.h"
*/

