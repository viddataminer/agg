#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raw_util.h"
#include "date_tools.h"

int g_no_data=0;


float calculate_percent(float, char *, char *);
float calc_uptime(char *, char *, char *);

main(int argc, char **argv)
{
    char ch, *ptr, buf[5000]="	now=then", cmd[200]="100%"; 
    char *comm_strs[10]={"99U#u#U\\!x", "oc12fast", "1xR\\$bluE", ""}, comm_str[100];
    char host_list[1000][200], output_file_with_path[200];
    char host[100], start_date[100], stop_date[100], old_host[100];
    char cnt[100], d1[100], d2[100], *sptr, sub_start_date[100];
    char output_file[200], host_name[200], host_group[200], host_group_wp[200];
    //char *comm_strs[3]={"99U#u#U\\!x", "oc12fast", 0 }, comm_str[100];
    FILE *fpr_hosts, *fpw;
    int i, num=0, got_sysdescr=0, got_sysname=0, got_location=0, cmds_to_try=2;
    int last_cnt=0, icnt, line_no=0, diff, num_mins=0, num_hosts=0;
    int just_got_new=0, got_data=0, sub_mins=0, got_dashes=0, doing_dashes=0;
    float pct_down1, pct_down6, uptime, fsecs=259200, fsecs1=43200;
    float tot_uptime;

    // .1.3.6.1.2.1.1 is equiv to 'System' MIB, which dosen't work on ubuntu!?

#define FAILURE 666

    // system  oid


    if(argc != 4) {
         printf("USAGE: calc_uptime some_group.cfg startdate(yymmddhhmm) stopdate(yymmhhddmm)\n"); exit(1);
    }
    if(strlen(argv[2]) != 10 || strlen(argv[3]) != 10  ) {
         printf("USAGE: calc_uptime some_group.cfg startdate(yymmddhhmm) stopdate(yymmhhddmm)\n"); exit(1);
    } else {
        strcpy(host_group_wp, argv[1]);
        if(sptr = strchr(host_group_wp,'.')) *sptr='\0';
        rm_path(host_group_wp, host_group);
        //strcat(output_file,".txt");
        strcpy(start_date, argv[2]);
        strcpy(stop_date, argv[3]);
/*
        if(!(fpw=fopen(output_file,"w"))) {
             printf("Can NOT open >%s< to WRITE\n", output_file); exit(1);
        }
*/
    }

    if((num_hosts = load_host_array(argv[1], host_list)) < 1) {
        printf("ERROR CFG: load_host_array returns %d\n", num_hosts); exit(2);
    }
    
    for(i=0; i< num_hosts; i++) {
        uptime = calc_uptime(host_list[i], start_date, stop_date);
        if(uptime < 0) continue;
        tot_uptime+=uptime;
        nslookup(host_list[i], host_name);
        if(uptime == 1)
            printf("%-35.35s %f%%\n", host_name, uptime*100);
        else printf("%-35.35s  %f%%\n", host_name, uptime*100);
        //printf("Host %s %f\n", host_list[i], uptime);
    }
    printf("Group Availibility: \n%-35.35s %f%%\n", host_group, (float)(tot_uptime/(num_hosts-g_no_data))*100);
}

int load_host_array(char *input_file, char host_list[][200]) {
    int num_hosts=0, got_a_line=0, collect=0;
    char buf[5000], *sptr, the_ip[100], host_group[100], host[200];
    char host_group_wp[200];
    FILE *fpr_hosts;

    if(!(fpr_hosts=fopen(input_file,"r"))) {
         printf("Can NOT open >%s< to read\n", input_file);
    }

    if(strstr(input_file,".list")) {
        while(fgets(host,200, fpr_hosts)) {
            if(host[0] == '#') continue;
            REMOVE_NEWLINE(host);
            if(!its_an_ip(host)) {
                if(nslookup(host, the_ip) != NSLOOKUP_SUCCESS) {
                    printf("Host Name >%s< NOT in DNS. Skipping.\n", host);
                    continue;
                }
                strcpy(host_list[num_hosts++], the_ip);
            } else strcpy(host_list[num_hosts++], host);
        }
        return num_hosts;
    }

    while(fgets(buf,5000, fpr_hosts)) {
        got_a_line=1;
        if(sptr = strstr(buf, "alias")) {
       
            //sptr += strlen("hostgroup_name") + 1; 
            //while(*sptr == ' ') ++sptr;
            //strcpy(host_group, sptr);
            //REMOVE_NEWLINE(host_group);
            collect=1;
            continue;
        }
        if(!collect) continue;
        if(sptr = strstr(buf, "members")) {
            sptr += strlen("members") + 1; 
            while(*sptr == ' ') ++sptr;
            strcpy(host, sptr);
            REMOVE_NEWLINE(host);
            if(!its_an_ip(host)) {
                if(nslookup(host, the_ip) != NSLOOKUP_SUCCESS) {
                    printf("Host Name >%s< NOT in DNS. Skipping.\n", host);
                    continue;
                }
                strcpy(host_list[num_hosts++], the_ip);
            } else strcpy(host_list[num_hosts++], host);
            continue;
        } else if(sptr = strstr(buf, "}")) return num_hosts;
        else return 0;
    }
    if(!got_a_line) { printf("EMPTY CFG - NEVER GOT A LINE\n"); return -1; }
    return 0;
}

//1204021002 
float calc_uptime(char *host, char *start_date, char *stop_date)
{
    FILE *fp, *fpr_hosts;
    //char downtime_list[1000][10];
    char host_file[200], start_down[20], stop_down[20], buf[5000], 
          cur_date[100], host_name[200];
    int istart, on_down_event=0, minutes_down=0, total_down=0;
    float percentage, pct_avail;
    FILE *fpr;

    sprintf(host_file,"/smg/out/ping/%s", host);
    if(!(fpr_hosts=fopen(host_file,"r"))) {
         nslookup(host, host_name);
         g_no_data++;
         printf("Can NOT open >%s< for %s to READ\n", host_file, host_name); 
         return -1;
    }

    fgets(buf,5000, fpr_hosts); strncpy(cur_date,buf,10); cur_date[10] = '\0';

    if(atoi(cur_date) > atoi(start_date)) {
        nslookup(host, host_name);
        printf("Host %s >%s<, data starts at >%s<\n",host_name,host,cur_date);
        //return 0;
    }
    
    while(fgets(buf,5000, fpr_hosts)) {
        strncpy(cur_date,buf,10); cur_date[10] = '\0';
        if(atoi(cur_date) < atoi(start_date)) continue;
        if(atoi(cur_date) > atoi(stop_date)) break;
        if(strstr(buf,"DN")) { 
            minutes_down++;
            if(!on_down_event) {
                strncpy(start_down,buf,10); start_down[10] = '\0';
                on_down_event=1;
            }
        }
        if(strstr(buf,"UP") && on_down_event) { 
            strncpy(stop_down,buf,10); stop_down[10] = '\0';
            on_down_event=0;
            char verbage[10];
            if(minutes_down == 1) strcpy(verbage, "minute ");
            else strcpy(verbage, "minutes");

            printf("\tDOWN %d %s from %s to %s\n", minutes_down, verbage,
                    start_down, stop_down);
           total_down += minutes_down;
           minutes_down = 0;
        }
    }
    pct_avail = calculate_percent(total_down, start_date, stop_date);
    return pct_avail; 
}

float calculate_percent(float minutes_down, char *start_date, 
      char *stop_date)
{
    char date[100];
    float num=0;

    strcpy(date, start_date);

    while(atoi(date) < atoi(stop_date)) {
        increment_date(date);
        num++;
    }
    return  (float) ((num - minutes_down)/num);
}

int its_an_ip(char *host)
{
    char *sptr; int i;

    if(strlen(host) > 16) return 0;
    for (i=0; i<3; i++) {
       if(sptr=strchr(host, '.')) {
           if(!isdigit(*(sptr-1)) || !isdigit(*(sptr+1))) return 0;
       } else return 0;
    }
    return 1;
}


