#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "log1.h"
#include "util.h"
#include "nsca.h"
#define status_file "/usr/local/nagios/var/status.dat"
#define LINE_IS_A_COMMENT buf[0] == '#'
// ENOUGH_TIME_TO_WAIT IS IN SECONDS
#define ENOUGH_TIME_TO_WAIT 6000
int line_num=0;
char second_half[1000];

main(int argc, char **argv)
{
    char buf[5000], cmd[2000], nag_struct[5000], host[500], service[500];
    char file_to_modify[200], *ptr;
    char file_to_modify_no_path[200];
    int i = 0, j = 0;
    int secs, file_changed = 0, match_len1, match_len, change=0, tot_changes=0;
    //char hosts_removed[100][100], services_removed[100][100];
    char *sptr, cl_host[100], cl_service[200], dummy;
    char f1_desc[200], f1_name[200];
    char f2_desc[200], f2_name[200];
    char f0_desc[200], f0_name[200];
    int structs_evaluated=0;
    int srvs_rm_cnt=0, host_rm_cnt=0, remove_host=1, srvs_st_rm_cnt=0;
    FILE *fpr_host, *fpw_host, *fpw, *fpr, *fpr_st_serv, *fpw_st_serv;

    if(argc == 1) { 
        printf("\nUSAGE: rin file, name1, value1, name2, value2, name3, new_value\n\n"); 
        printf("SO, for example, if you run:\n\nrin /usr/local/nagios/etc/objects/services.cfg service_description queck_adp* check_command check_dummy use Massive_template\n\nwill result in the [31m/usr/local/nagios/etc/objects/services.cfg[0m file being opened, and whatever structure has a\n [32mservice_description[0m that STARTS with [31mqueck_adp[0m, and has a\n [32mcheck_ccommand[0m value of [31mcheck_dummy[0m, then that structures\n [32muse[0m value will be changed to [31mMassive_Template[0m\nver0.3\n");
        exit(0); 
    } 

    //if(!reload()) printf("CONFIG FILES ARE CORRUPT - NO ATTEMPT TO MODIFY. ABORYTING!!\n");
    strcpy(file_to_modify, argv[1]);
    strcpy(f0_desc, argv[2]);
    strcpy(f0_name, argv[3]);
    strcpy(f1_desc, argv[4]);
    strcpy(f1_name, argv[5]);
    strcpy(f2_desc, argv[6]);
    strcpy(f2_name, argv[7]);
    strcat(f2_name, "\n");

    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]

    if(sptr = strchr(f1_name,'*')) {
        if(f1_name[0] == '*') {
            if(f1_name[strlen(f1_name)-1] == '*') {
                f1_name[strlen(f1_name)-1] == '\0';
                match_len = 9998;
                strcpy(f1_name, &f1_name[1]);
            } else {
                match_len = 9999;
                strcpy(f1_name, &f1_name[1]);
            }
        } else {
            strcpy(second_half,sptr+1);
            if(ptr=strchr(second_half,'\n')) *ptr='\0';
            *sptr='\0';
            match_len = strlen(f1_name);
            slog("matching >%s< * >%s<\n", f1_name, second_half);
        }
    }
    if(sptr = strchr(f0_name,'*')) {
        if(f0_name[0] == '*') {
            if(f0_name[strlen(f0_name)-1] == '*') {
                f0_name[strlen(f0_name)-1] == '\0';
                match_len1 = 9998;
                strcpy(f0_name, &f0_name[1]);
            } else {
                 match_len1 = 9999;
                strcpy(f0_name, &f0_name[1]);
            }
        } else {
            strcpy(second_half,sptr+1);
            if(ptr=strchr(second_half,'\n')) *ptr='\0';
            *sptr='\0';
            match_len1 = strlen(f0_name);
            slog("matching >%s< * >%s<\n", f0_name, second_half);
        }
    }

    slog("-------------- process starting ------v1.0-------\n");
    strcpy_nopath(file_to_modify_no_path, file_to_modify);

    sprintf(cmd,"cp %s /tmp/%s.jjbk", file_to_modify, file_to_modify_no_path);
    system(cmd);

    if(!(fpr = fopen(file_to_modify,"r"))){ 
        printf("noop %s\n", file_to_modify);
        exit(1);
    }

    if(!(fpw = fopen("/tmp/replace_in_nag.tmp","w"))) { 
        printf("Can NOT fopen %s\n", status_file);
        exit(1);
    }
        slog("get_next_struct(match_len = %d, match_len1 = %d, f1_desc = %s, f1_name = %s, f2_desc = %s, f2_name = %s, f0_desc = %s, f0_name = %s\n", match_len, match_len1, f1_desc, f1_name, f2_desc, f2_name, f0_desc, f0_name);

    while(1) {
        if(!get_next_struct(fpr, match_len, match_len1, f1_desc, f1_name, f2_desc, f2_name, f0_desc, f0_name, nag_struct, &change))
            break;
        fprintf(fpw ,"%s\n\n\n",nag_struct); 
        tot_changes += change;
        ++structs_evaluated;
    }
    fclose(fpw);
    fclose(fpr);
    slog("So, %d struct's (of %d) will be modified. Continue?\n", 
         tot_changes, structs_evaluated);
    dummy=getchar();
    if(dummy != 'Y' && dummy != 'y') exit(0);

    sprintf(cmd,"cp /tmp/replace_in_nag.tmp %s", file_to_modify);
    system(cmd);
    reload();
    slog("-------------- process complete -------------\n");
}


int seconds_since_last_update(char *host_service_combo, FILE *fpstatus) 
{
    char buf[500], tmp_combo[500], *ptr, *sptr;
    int got_combo=0;
    char last_check_time[50];
    time_t current_time, secs;
    int in_block=0;
    struct tm *newtime;


    //newtime = localtime(&current_time);
    current_time = time(NULL);

    while(fgets(buf,4999,fpstatus)) {
        if(strstr(buf,"servicestatus {")) in_block=1;
        if(buf[0] == '}') in_block=0;
        if(!in_block || LINE_IS_A_COMMENT) continue;
        if(got_combo) {
            if(!(sptr = strstr(buf,"last_check="))) 
                continue;
            sptr+=11;
            strcpy(last_check_time,sptr);
            slog("curr = %d, last_check = %s\n",current_time,last_check_time);
            return current_time - atol(last_check_time);
        }
        if(sptr = strstr(buf,"host_name=")) {
            sptr += 10;
            strcpy(tmp_combo, sptr);
            if(sptr=strchr(tmp_combo,'\n')) *sptr = '\0';
            continue;
        }
        if(sptr = strstr(buf,"service_description=")) {
            sptr += 20;
            strcat(tmp_combo, sptr);
            if(sptr=strchr(tmp_combo,'\n')) *sptr = '\0';
            if(!strcmp(host_service_combo, tmp_combo))
                got_combo=1;
            else memset(tmp_combo,'\0',499);
        }
    }
    slog("never found mactch for >%s<\n",host_service_combo);
    return -1;
}

int get_next_struct(FILE *fprtail, int chars_to_comp, int chars_to_comp0,
      char *d1, char *n1, char *d2, char *n2, char *d0, char *n0, char *da_struct, int *change)
{
    char buf[500], tmpbuf[500], service[100], *ptr, *sptr;
    char nb0[200], nb1[200], nb2[200];
    char unchanged_st[2000];
    char changed_st[2000];
    int got_something=0, found_match=0, found_match_0=0, names_are_diff=0;

    *change=0;
    ptr=tmpbuf;
    while(fgets(buf,4999,fprtail)) {
        line_num++;
        if(strstr(buf,"define") && strstr(buf," {")) {
            got_something = 1;
            found_match = found_match_0 = 0;
            memset(unchanged_st,0,sizeof(unchanged_st));
            memset(changed_st,0,sizeof(changed_st));
            strcat(unchanged_st, buf);
            strcat(changed_st, buf);
            continue;
        }
        if(sptr=strstr(buf,d0)) {
            while(*sptr != ' ' && *sptr != '\t') 
                ++sptr;
            while(*sptr == ' ' || *sptr == '\t') 
                ++sptr;
            memset(tmpbuf,0,499); ptr = tmpbuf;
            while(*sptr != '\n') { *ptr = *sptr; ++sptr; ++ptr; }
            *ptr='\0';
            strcpy(nb0, tmpbuf);
            if(chars_to_comp0) {
                if(chars_to_comp0 > 9997 ) {
                    if(strstr(nb1, n0))
                        found_match_0=1;
                } else if(!strncmp(nb0,n0,chars_to_comp0) 
                           && strstr(nb0, second_half)) 
                    found_match_0 = 1;
            } else if(!strcmp(nb0,n0)) found_match_0 = 1;
            strcat(unchanged_st, buf);
            strcat(changed_st, buf);
            continue;
        }
        if(sptr=strstr(buf,d1)) {
            while(*sptr != ' ' && *sptr != '\t') 
                ++sptr;
            while(*sptr == ' ' || *sptr == '\t') 
                ++sptr;
            memset(tmpbuf,0,499); ptr = tmpbuf;
            while(*sptr != '\n') { *ptr = *sptr; ++sptr; ++ptr; }
            *ptr='\0';
            strcpy(nb1, tmpbuf);
            if(chars_to_comp) {
                if(chars_to_comp > 9997) {
                    if(strstr(nb1, n1))
                        found_match=1;
                } else if(!strncmp(nb1,n1,chars_to_comp)
                           && strstr(nb0, second_half)) 
                    found_match = 1;
            } else if(!strcmp(nb1,n1)) found_match = 1;
            strcat(unchanged_st, buf);
            strcat(changed_st, buf);
            continue;
        }
        if(sptr=strstr(buf,d2)) {
            if(!strstr(buf,n2)) {
                //slog("no >%s< in >%s<\n", n2, buf);
                names_are_diff=1;
            }
            else names_are_diff=0;
            strcat(unchanged_st, buf);
            while(*sptr != ' ' && *sptr != '\t') ++sptr;
            while(*sptr == ' ' || *sptr == '\t') ++sptr;
            strcpy(sptr, n2);
            strcat(changed_st, buf);
            continue;
        }
        if(buf[0] == '}') {
            strcat(unchanged_st, buf);
            strcat(changed_st, buf);
            if(found_match && found_match_0) strcpy(da_struct , changed_st);
            else strcpy(da_struct , unchanged_st);
            
            if(found_match && found_match_0 && names_are_diff) {
                *change=1;
               slog("Match - Modify Struct\n------ Old struct -------- \n%s\n--------- New struct---------\n%s\n--------------------------\n", unchanged_st, changed_st);
            }
            return 1;
        }
        strcat(unchanged_st, buf);
        strcat(changed_st, buf);
    }
    if(!got_something) return 0;
} 


int get_next_host_struct(FILE *fprtail, char *host_struct, char *da_host)
{
    char buf[500], tmpbuf[500], service[100], *ptr, *sptr;
    int got_something=0, recording=0;

    ptr=tmpbuf;
    while(fgets(buf,4999,fprtail)) {
        got_something = 1;
        if(strstr(buf,"define host {")) {
            recording=1;
            strcpy(host_struct, buf);
            continue;
        }
        if(sptr=strstr(buf,"host_name")) {
            sptr+=10;
            while(*sptr == ' ' || *sptr == '\t') ++sptr;
            memset(tmpbuf,0,499); ptr = tmpbuf;
            while(*sptr != '\n') { *ptr = *sptr; ++sptr; ++ptr; }
            *ptr='\0';
            strcpy(da_host, tmpbuf);
        }
        if(buf[0] == '}') {
            strcat(host_struct, buf);
            return 1;
        }
        strcat(host_struct, buf);
    }
    if(!got_something) return 0;
}


int reload()
{
    FILE *ptr;
    char buf[1000], cmd[200];
    int pipe_returned_nothing=1;

    slog("in reload_nagios\n");

    slog("System( /etc/init.d/nagios reload )  Returns:\n");
    ptr=popen("/etc/init.d/nagios reload","r");
    while(fgets(buf,5000,ptr)) {
        pipe_returned_nothing=0;
        slog("%s\n",buf);
        if(strstr(buf,"Error processing object config files")) {
            slog("Reload NAGIOS returns config error...");
            return 0;
        }
        if(strstr(buf,"Running configuration check... CONFIG ERROR!")) {
            slog("Reload nagios returns config error...");
            return 0;
        }
    }
    if(pipe_returned_nothing) slog("nagios reload pipe returned nothing!\n");
    return 1;
}

int strcpy_nopath(char *file_to_modify_no_path, char *file_to_modify)
{
    char *sptr, *eventual_start_of_string;
    sptr=file_to_modify;
    if(!(sptr=strchr(sptr, '/'))) {
        strcpy(file_to_modify_no_path, file_to_modify);
        return 0;
    }
    ++sptr;
    eventual_start_of_string = sptr;
    while (sptr=strchr(sptr, '/')) { ++sptr; eventual_start_of_string=sptr; }
    strcpy(file_to_modify_no_path, eventual_start_of_string);
    slog("file to mod no path is >%s<\n", file_to_modify_no_path);
}
