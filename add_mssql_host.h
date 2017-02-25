#ifdef TESTMAIN

#include <stdio.h>
#include <strings.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "path.h"
#include "log1.h"
#include "util1.h"
#include "nsca.h"

#endif

#define MSSQL_LOGCK_CFG_FILE "/smg/cfg/mssql.hosts"
#define FAILED_PING 1
#define CANT_OPEN_OUTPUT_READ 2
#define CANT_OPEN_OUTPUT_WRITE 3
#define SEV_CFG_ERR 4
#define ALREADY_IN_CFG 4
#define CANT_OPEN_CFG 5
#define SUCCESS 0
char g_log_file_name[100] = "ERRORLOG";
 
char g_default_ignore_these_error_codes[1000] = "|<Error: 17806>|<Error: 17828>|<Error: 17832>|<Error: 17836>|-W<Error: 18056>|<Error: 18452>|<Error: 18456>|-W<Error: 1474>";

int get_mssql_logs(char *node, char *domain,  char log_list[][200]) ;

int g_num_errors_sent = 0;
/*--------------------------------------------------------------------------*\
|* VERSION INFO                                                             *|
|* ver 1.14 100505: changed denoting line with 'Error:' as an error line.   *|
|*                  string must also now contain the word 'Severity:'       *|
|* ver 1.15 100506: Can handle multiple logs of same name on same box       *|
|* ver 1.16 100506: clean() routine needed cleaning                         *|
|* ver 2.0  100525: no longer sending host up/down events. pre-process      *|
|*                  ping and appinfo messages all handled by                *|
|*                  sent_appinfo_event_due_to_failed_ping(...))             *|
\*--------------------------------------------------------------------------*/
char  *ver="v2.0";

#ifdef comment
sterling@voltaire:~$ /smg/bin/wmic //adssqlcl1.med.utah.edu -U srvr/nagiosadmin%nagios123 "select name from CIM_DataFile where filename='ERRORLOG'" | grep -v "log\."
CLASS: CIM_DataFile
Name
d:\microsoft sql server\mssql.1\mssql\log\errorlog
e:\microsoft sql server\mssql.2\mssql\log\errorlog
s:\microsoft sql server\mssql.3\mssql\log\errorlog
#endif

#ifdef TESTMAIN
int main(int argc, char **argv)
{   
    add_mssql_host(argc, argv);
}
int add_mssql_host(int argc, char **argv)
#else
int add_mssql_host(char *node, char *domain, int use_default_ignore_error_codes)
#endif
{
    char cfg_file_with_path[100], buf[5000], acct_file[200], filename[200], path[200];
    char *sptr, dom[100], drive[3], agg_ip[20], node_ip[20];
    char sev[20], log_list[20][200], error_num_list[100][10];
    char error_strs[100][500], sev_range[20], drive_letter, strcat_buf[1000];
    char cmd[5000];
    FILE *pptr, *fp_cfg;
    int i, num_logs = 0, logs_tailed_successfully = 0,  result = 0,info_only=0;
    int num_err_nums=0, num_err_strs=0;
    int ret_code, j;
#ifdef TESTMAIN
    char node[200], domain[200];
    if(argc < 2) { 
        printf("USAGE: mssql_info node domain [sev range] [err num to ignore] [err str to ignore\n"); 
        exit(2); 
    }
    parse_incomming(argc, argv, sev_range, acct_file, &info_only,
      error_num_list, &num_err_nums, error_strs, &num_err_strs);
    strcpy(node, argv[1]); 
    strcpy(domain, argv[2]); 
#else 
    strcpy(sev_range,"16-25");
    strcpy(acct_file,"/smg/cfg/acct_info.dat");
    strcpy(g_log_file_name,"ERRORLOG");
#endif

    num_logs = get_mssql_logs(node, domain, log_list);
#ifdef DEBUG1
    slog("returned from get_mssql_logs.  \n"); 
#endif

    for (i = 0; i < num_logs; i++) {
        REMOVE_NEWLINE(log_list[i]);
        slog("%s\n", log_list[i]);
        strcpy(path, &log_list[i][2]); 
        //path[ strlen(path) - strlen("ERRORLOG") ] = '\0';
        path[ strlen(path) - strlen(g_log_file_name) ] = '\0';

        escape_backslach(path);

        sprintf(buf, "%s|%s|%s|%c|%s|%s|%s", node, domain, acct_file,
            log_list[i][0], sev_range, g_log_file_name, path);

        for (j = 0; j < num_err_nums; j++) {

            sprintf(strcat_buf, "|<Error: %s>", error_num_list[j]);
            strcat(buf, strcat_buf);
        }

        for (j = 0; j < num_err_strs; j++) {
            sprintf(strcat_buf, "|%s", error_strs[j]);
            strcat(buf, strcat_buf);
        }
        if(use_default_ignore_error_codes) {
            if(use_default_ignore_error_codes == PROMPT) {
                printf("Enter error code string (i.e.: |<Error: 1234>)\n");
                fgets(strcat_buf,1000,stdin);
                REMOVE_NEWLINE(strcat_buf);
                strcat(buf, strcat_buf);
            } else strcat(buf, g_default_ignore_these_error_codes);
        }
        sprintf(cmd,"echo \"%s\" >> %s\n", buf, MSSQL_LOGCK_CFG_FILE);

        slog("checking for existance(%s,%s)\n", buf, MSSQL_LOGCK_CFG_FILE);
        ret_code = entry_already_in_config(buf, MSSQL_LOGCK_CFG_FILE);

        if(ret_code == ALREADY_IN_CFG) {
            printf("[31mAlready monitoring node[0m >[35m%s[0m< for file\n\t[32m%s[0m\n", 
             node, log_list[i]);
            continue;
        } else if(ret_code == CANT_OPEN_CFG) { 
            printf("CAN NOT OPEN CFG FILE >%s<\n", MSSQL_LOGCK_CFG_FILE);
            exit(0);
        }
            
        printf("Adding the follwing line to %s:\n  [32m%s[0m\n", MSSQL_LOGCK_CFG_FILE, buf);
        system(cmd);
    }
    if(!num_logs) printf("NO FILE named >%s< exists on  %s\\%s\n", 
               g_log_file_name, domain, node);
}
int entry_already_in_config(char *new_entry, char *cfg_file)
{
    FILE *fp;
    char *sptr, buf[10000];
    if(!(fp=fopen(cfg_file,"r"))) { 
        printf("NO OPEN >%s<\n", cfg_file); return CANT_OPEN_CFG;
    }
    while(fgets(buf,10000,fp)) {
        if(buf[0]=='#') continue;
        REMOVE_NEWLINE(buf);
        if(!strfcmp(buf, new_entry, '|', '\\', 7)) 
            return ALREADY_IN_CFG;
    }
    return 0; 
}

int escape_backslach(char *path)
{
    char buf[5000], *ptr, *pptr;
    ptr=buf; pptr=path;

    while(*pptr) {
        *ptr = *pptr; 
        if(*pptr == '\\') {
            *(++ptr) = '\\';
            *(++ptr) = '\\';
            *(++ptr) = '\\';
#ifdef UBU
            *(++ptr) = '\\';
            *(++ptr) = '\\';
            *(++ptr) = '\\';
            *(++ptr) = '\\';
#endif
        } 
        ++ptr; ++pptr;
    }
    *ptr='\0';
    strcpy(path,buf);
}

//int get_mssql_logs(char *acct_file, char *domain,  char *node, char *path, char *drive) 
int get_mssql_logs(char *node, char *domain, char log_list[][200]) 
{
    FILE *fp, *fpt;
 
    char *sptr, service[500], *the_sev, the_sev_buf[500], output_file_bk[200];
    char les_file[200], cmd[1000], unqual_node[200], tmp_output_file[200];
    char drive[10];
    char fileandpath[1000], savebuf[5000], logbuf[5000], output_file[200]; 
    char msg_text[5000], last_processed_msg[5000], name[200], cpath[500], pass[200];
    int i, ithe_sev, found_last_line_of_log_from_prev_proc=0, first_run=0;
    int low_cfg_sev, hi_cfg_sev, num_logs=0, got_class=0;

    slog("in get_mssql_logs()\n");

    //get_name_pass(acct_file, name, pass);

    sprintf(cmd,"/smg/bin/wmic //%s -U %s/nagiosadmin%%nagios123 \"select name from CIM_DataFile where filename='%s'\"", node, domain, g_log_file_name);
    slog("trying os cmd: >%s<\n",cmd);

    if(!(fp = popen(cmd,"r"))) { 
        slog("Cant open >%s< for host >%s< domain >%s< \n", cmd, node, domain);
        return CANT_OPEN_OUTPUT_READ;
    } else slog("Opened cmd output.  Parsing...\n");

#ifdef doda
CLASS: CIM_DataFile
Name
d:\microsoft sql server\mssql.1\mssql\log\errorlog
e:\microsoft sql server\mssql.2\mssql\log\errorlog
#endif

    while(fgets(logbuf, 5000, fp)) {

#ifdef DEBUG1
        slog("got >%s<\n", logbuf);
#endif
        REMOVE_NEWLINE(logbuf);

        if(strstr(logbuf,"CLASS:")) { 
            fgets(logbuf, 5000, fp); // get header
            got_class=1;
            continue;
        } else if (!got_class) continue;

        if(errorlog_followed_by_a_dot(logbuf)) {
#ifdef DEBUG1
             slog("SKIPPING: >%s<\n",logbuf); 
#endif
             continue; // its a backupf
        } else if(strstr(logbuf,"recycle")) {
#ifdef DEBUG1
             slog("SKIPPING: >%s<\n",logbuf); 
#endif
             continue; // recycle bin  ERRORLOG
        } else if(!strstr(logbuf,"sql")) {  // if no 'sql' in path, skip
#ifdef DEBUG1
             slog("SKIPPING: >%s<\n",logbuf); 
#endif
             continue; 
        }
        slog("strcpy to logbuf >%s<\n", logbuf);
        strcpy(log_list[num_logs++],logbuf);
    }
    fclose(fp);
    return num_logs;
}

int errorlog_followed_by_a_dot(char *logbuf)
{
    char *last, *sptr;

    sptr=logbuf;
#ifdef DEBUG1
    slog("in ..followed_by_a_dot(%s)\n", logbuf);
#endif
    while((sptr = strchr(sptr, '\\'))) { last = sptr; ++sptr; ++last; }
    // got rid of the path, 'last' now points to errorlog??????
#ifdef DEBUG1
    slog("lastptr >%s<\n", last);
#endif

    if(strchr(last,'.')) return 1; 
    
    return 0;
}


int parse_incomming(int argc, char **argv, 
 char *sev_range, char *acct_file, int *info_only, 
 char error_nums[][10] , int *num_nums, char error_strs[][500], int *num_strs)
{

    int i, j=0, k=0;

    strcpy(sev_range,"16-25");
    strcpy(acct_file,"/smg/cfg/acct_info.dat");
    strcpy(g_log_file_name,"ERRORLOG");
   
    for(i=3; i < argc; i++) {
        if(argv[i][0] == '/') {
            strcpy(acct_file, argv[i]);
        } else if (strchr(argv[i], '-')) {
            if(argv[i][0] == '-') {                // its a flag
                switch(argv[i][1]) {
                    case 'i': *info_only=1; break; // -i option
                    case 'f':  
                      if(strlen(argv[i]) > 2)      // spqce tween nam
                          strcpy(g_log_file_name, &argv[i][2]); 
                      else strcpy(g_log_file_name, argv[++i]); 
                      break;
                      default:printf("UNKNOWN FLAG >%c<\n",argv[i][1]);exit(2);
                    }
            } else strcpy(sev_range, argv[i]);     // or severity range
        } else if(its_a_number(argv[i])) {         // an error number, or an
            strcpy(error_nums[j++],argv[i]);  
        } else {
            strcpy(error_strs[k++],argv[i]);       // error string
        }
    }
    *num_nums=j; *num_strs=k;
    return 1;
}


int its_a_number(char *buf)
{
    while(*buf) if(!isdigit(*(buf++))) return 0;
    return 1;
}

