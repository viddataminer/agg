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
    char line_to_add[300], data[300], data_type[100];
    int structs_evaluated=0, remove_from_nagios=0, replace_in_nagios=0;
    int start=0, end=0, prompt=1, add_to_nagios=0, no_reload=0;
    int srvs_rm_cnt=0, host_rm_cnt=0, remove_host=1, srvs_st_rm_cnt=0;
    FILE *fpr_host, *fpw_host, *fpw, *fpr, *fpr_st_serv, *fpw_st_serv;

    if(argc == 1) { 
        system("clear"); 
        printf("\nUSAGE: gn file pattern [-no_prompt] [[pattern]...] [-v exclude pattern] [-remove] [-replace regex_pattern_to_match replace_string] [ -add data_type data]\n\n"); 
//        printf("NOTE: gn will open ./services.cfg by default\n");
        printf("Examples:\n\n");
        printf(">  gn \"host_name\\s*r2-\" \n");
        printf("will show all structs with host name starting with 'r2-'\n\n");
        printf(">  gn \"host_name\\s*r2-\" -v Pass \n");
        printf("will show all structs with host name starting with 'r2-'\n");
        printf("that DO NOT have the string 'Pass' in the struct anywhere\n\n");
        printf(">  gn \"host_name\\s*r2-\" -v Pass -add service_group epic_group\n");
        printf("will add the line:\n");
        printf("	service_group			epic_group\n");
        printf("to all structs with host name starting with 'r2-'\n");
        printf("that DO NOT have the string 'Pass' in the struct anywhere\n\n");
        printf(">  gn \"host_name\\s*r2-\" -v Pass -remove \n");
        printf("Will remove all structs with host name starting with 'r2-'\n");
        printf("that DO NOT have the string 'Pass' in the struct anywhere\n\n");
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
    } else if(!strcmp(argv[1], "h")) { 
        strcpy(input, "/usr/local/nagios/etc/objects/hosts.cfg"); 
        starton = 2; 
    } else if(!strcmp(argv[1], "t")) { 
        strcpy(input, "/usr/local/nagios/etc/objects/stateless_services.cfg"); 
        starton = 2; 
    } else if(!strcmp(argv[1], "s")) { 
        strcpy(input, "/usr/local/nagios/etc/objects/services.cfg"); 
        starton = 2; 
    } else { 
        strcpy(input, "/usr/local/nagios/etc/objects/services.cfg"); 
        starton = 1; 
    }
    for(i=starton; i< argc; i++) {
        if(!strcmp(argv[i], "-no_prompt")) {
            prompt = 0;
            continue;
        } else if(!strcmp(argv[i], "-replace")) {
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
        } else if( !strcmp(argv[i], "-dont_reload") ||
            !strcmp(argv[i], "-no_reload")) {
            no_reload=1;
            continue;
        } else if(!strcmp(argv[i], "-add")) {
            add_to_nagios=1;
            sprintf(output, "/tmp/new_nag.%d", epoch()); 
            if(!(fpw=fopen(output,"w"))) {
                printf("Can't open %s to WRITE\n", output); exit(2);
            }
            strcpy(data_type, argv[++i]);
            strcpy(data, argv[++i]);
            continue;
        } else strcpy(args_of_interest[arg_num++], argv[i]);
    }
//    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]

    if(!(fpr = fopen(input,"r"))){ 
        printf("noop %s\n", input);
        exit(1);
    }

    while(1) {

        if(!get_next_struct(fpr, nag_struct)) break;

        for(i=0; i< arg_num; i++) {
            if(!strcmp(args_of_interest[i], "-v")) {
                i++;
                if(string_contains_pat_looking_at_substrings_of_the_structure(
                  nag_struct, args_of_interest[i]))
                    break;
            } 
            else if(!string_contains_pat_looking_at_substrings_of_the_structure(
                  nag_struct, args_of_interest[i]))
                    break;
        }

        if(arg_num == i) {
            if(replace_in_nagios && 
              string_contains_pat(nag_struct, regex_pat, &start, &end)) {
                if(prompt) printf("\t[31m -------- Existing Structure starting on line %d: --------[0m\n%s", 
                    g_line_no-5, nag_struct);
              while(string_contains_pat(nag_struct, regex_pat, &start, &end))
                replace_pat(nag_struct, replace_str, start, end);
                if(prompt) printf("\t[31m            will be changed to look like:[0m\n%s\n", 
                    nag_struct);
                fprintf(fpw,"%s\n\n", nag_struct);
            } else if(add_to_nagios && 
              string_contains_pat(nag_struct, regex_pat, &start, &end)) {
                if(prompt) printf("\t[31m -------- Existing Structure starting on line %d: --------[0m\n%s", 
                    g_line_no-5, nag_struct);
                add_line(nag_struct, data_type, data);
                if(prompt) printf("\t[31m            will be changed to look like:[0m\n%s\n", 
                    nag_struct);
                fprintf(fpw,"%s\n\n", nag_struct);
            } else {
                if(prompt) printf("\t\t\t[31mstarting on line %d:[0m\n%s\n", 
                    g_line_no-5, nag_struct);
            }
            match++;
        } else if(remove_from_nagios || replace_in_nagios || add_to_nagios) 
             fprintf(fpw,"%s\n\n", nag_struct);
    }
    fclose(fpr); 

    if((remove_from_nagios || replace_in_nagios || add_to_nagios) && match) {
        fclose(fpw);
        if(prompt) { 
            if(remove_from_nagios)
                printf("The previous %d structures WILL BE REMOVED from[31m %s [0m\n", match, input);
            else printf("The previous %d structure modifications WILL BE APPLIED to\n\t[31m %s [0m\n", match, input);
            printf("Enter 'y' to apply changes: "); 
            fgets(dummy, 3, stdin); 
        }
        else dummy[0]='y';

        if(dummy[0] == 'y' || dummy[0] == 'Y') {
            sprintf(cmd,"mv %s %s", output, input);
            system(cmd);
            if(no_reload) {
                if(prompt) printf("NOT Reloading Nagios...\n");
            } else { 
                if(prompt) printf("Reloading Nagios...\n");
                system("service nagios reload");
            }
        } else {
            sprintf(cmd,"rm %s", output);
            system(cmd);
            if(prompt) printf("No changes were made\n");
        }
    } else if(prompt) {
        if(!match) printf("[31mNo Match[0m\n");
        else printf("[5m[34m[5m%d[32m matches in [34m%s[1m[0m\n", match, input);
    } else {
        sprintf(cmd,"rm %s 2>&1 > /dev/null", output);
        system(cmd);
    }
    //slog("So, %d struct's (of %d) will be modified. Continue?\n", 
    //     tot_changes, structs_evaluated);
    exit(match);
}


int get_next_struct(FILE *fprtail, char *da_struct)
{
    char buf[5000], unchanged_st[50000];
    int got_something=0;

    while(fgets(buf,4999,fprtail)) {
        if(buf[0] == '#') continue;
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

