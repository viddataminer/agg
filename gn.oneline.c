#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "log1.h"
#include "nsca.h"
#include "util.h"
#define status_file "/usr/local/nagios/var/status.dat"
#define LINE_IS_A_COMMENT buf[0] == '#'
// ENOUGH_TIME_TO_WAIT IS IN SECONDS
#define ENOUGH_TIME_TO_WAIT 6000
int g_line_no=0, minus_v=0;

main(int argc, char **argv)
{
    char buf[50000], cmd[2000], nag_struct[50000], host[500], service[500];
    char input[200], output[200], args_of_interest[10][100];
    int i = 0, j = 0, starton = 2, match=0, arg_num=0;
    int secs, file_changed = 0, match_len, change=0, tot_changes=0;
    //char hosts_removed[100][100], services_removed[100][100];
    char *sptr, cl_host[100], cl_service[200], dummy[10];
    char f1_desc[200], f1_name[200], regex_pat[1000], replace_str[1000];
    char f2_desc[200], f2_name[200];
    int structs_evaluated=0, remove_from_nagios=0, replace_in_nagios=0;
    int start=0, end=0;
    int srvs_rm_cnt=0, host_rm_cnt=0, remove_host=1, srvs_st_rm_cnt=0;
    FILE *fpr_host, *fpw_host, *fpw, *fpr, *fpr_st_serv, *fpw_st_serv;

    if(argc == 1) { 
        printf("\nUSAGE: gn file pattern [[pattern]...] [-v exclude pattern] [-remove] [-replace regex_pattern_to_match replace_string]\n\n"); 
        exit(0); 
    } 

    if(strstr(argv[1], ".cfg")) {
        if(argv[1][0] == '-') { 
            remove_from_nagios = 1; 
            strcpy(input, &argv[1][1]); 
            sprintf(output, "/tmp/new_nag.%d", epoch()); 
            if(!(fpw=fopen(output,"w"))) {
                printf("Can't open %s to WRITE\n", output); exit(2);
            }
        } else strcpy(input, argv[1]);
    } else { 
        strcpy(input, "/usr/local/nagios/etc/objects/services.cfg"); 
        starton = 1; 
    }
    for(i=starton; i< argc; i++) {
        if(!strcmp(argv[i], "-replace")) {
            if(remove_from_nagios) {
                printf("-replace and -remove are incompatible options\n");
                exit(0);
            }
            sprintf(output, "/tmp/new_nag.%d", epoch()); 
            if(!(fpw=fopen(output,"w"))) {
                printf("Can't open %s to WRITE\n", output); exit(2);
            }
            replace_in_nagios=1;
            strcpy(regex_pat, argv[++i]);
            strcpy(args_of_interest[arg_num++], argv[i]);
            strcpy(replace_str, argv[++i]);
            continue;
        } else if(!strcmp(argv[i], "-remove")) {
            if(replace_in_nagios) {
                printf("-replace and -remove are incompatible options\n");
                exit(0);
            }
            remove_from_nagios=1;
            sprintf(output, "/tmp/new_nag.%d", epoch()); 
            if(!(fpw=fopen(output,"w"))) {
                printf("Can't open %s to WRITE\n", output); exit(2);
            }
            continue;
        } else strcpy(args_of_interest[arg_num++], argv[i]);
    }
//    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]

    if(!(fpr = fopen(input,"r"))){ 
        printf("noop %s\n", input);
        exit(1);
    }

    while(1) {
        //memset(nag_struct,0,sizeof(nag_struct));
        if(!get_next_struct(fpr, nag_struct)) break;
        //printf("strlen is %d\n",(int)strlen(nag_struct));

        for(i=0; i< arg_num; i++) {
            if(!strcmp(args_of_interest[i], "-v")) {
                i++;
                if(string_contains_pat(nag_struct, args_of_interest[i], 
                        &start, &end))
                //if(strstr(nag_struct, argv[i])) 
                    break;
            } 
            //else if(!strstr(nag_struct, argv[i])) 
            else if(!string_contains_pat(nag_struct, args_of_interest[i], 
                 &start, &end))
                break;
        }
        if(arg_num == i) {
            if(replace_in_nagios && 
              string_contains_pat(nag_struct, regex_pat, &start, &end)) {
                printf("\t\t\t[31mExisting Structure on line %d:[0m\n%s\n", 
                     g_line_no-5, nag_struct);
                replace_pat(nag_struct, replace_str, start, end);
                printf("\t\t\t[31mModified Structure:[0m\n%s\n", 
                     nag_struct);
                fprintf(fpw,"%s\n\n", nag_struct);
            } else {
                printf("\t\t\t[31mstarting on line %d:[0m\n%s\n", 
                     g_line_no-5, nag_struct);
            }
            match++;
        } else if(remove_from_nagios || replace_in_nagios) 
             fprintf(fpw,"%s\n\n", nag_struct);
    }
    fclose(fpr); 
    if((remove_from_nagios || replace_in_nagios) && match) {
        fclose(fpw);
        if(remove_from_nagios)
            printf("The previous %d structures WILL BE REMOVED from[31m %s [0m\nEnter 'y' to apply changes: ", match, input);
        else printf("The previous %d structure modifications  WILL BE APPLIED to[31m %s [0m\nEnter 'y' to apply changes: ", match, input);
        fgets(dummy, 3, stdin);
        if(dummy[0] == 'y' || dummy[0] == 'Y') {
            sprintf(cmd,"mv %s %s", output, input);
            system(cmd);
            printf("You must manually reload Nagios before changes will take affect\n");
        } else {
            sprintf(cmd,"rm %s", output);
            system(cmd);
        }
    }
    if(!match) printf("[31mNo Match[0m\n");
    //slog("So, %d struct's (of %d) will be modified. Continue?\n", 
    //     tot_changes, structs_evaluated);
}


int get_next_struct(FILE *fprtail, char *da_struct)
{
    char buf[5000], unchanged_st[50000];
    int got_something=0;

    while(fgets(buf,4999,fprtail)) {
        g_line_no++;
        if( (strstr(buf,"define") && strstr(buf,"{") ) ||
           ( strstr(buf,"hoststatus") && strstr(buf,"{") )  ||
           ( strstr(buf,"servicestatus") && strstr(buf,"{")) ) 
        {
            got_something = 1;
            memset(unchanged_st,0,sizeof(unchanged_st));
            strcat(unchanged_st, buf);
            continue;
        }
        if(strstr(buf,"}")) {
            strcat(unchanged_st, buf);
            strcpy(da_struct, unchanged_st);
            return 1;    
        }
        strcat(unchanged_st, buf);
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

