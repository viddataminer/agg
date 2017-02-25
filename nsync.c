/****  TO CONVERT NAME TO LOWERCASE, search for lower_case and toupper 
and uncomment 
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "log1.h"
#include "nsca.h"
#include "util.h"
//#include "nslookup.h"

#define SUCCESS 0
#define SERVICE_IS_STATELESS \
           !strncmp(service_name,"eventlog_",9) || \
           !strncmp(service_name,"mssql_",6) || \
           !strncmp(service_name,"logck_",6) || \
           !strncmp(service_name,"queck_",6)
#define REMOVE_NEWLINE(buf) \
    if(sptr = strchr(buf,'\n')) *sptr = '\0'; \
    if(sptr = strchr(buf,'\r')) *sptr = '\0'

#define USV3 "%[^_]%*[_]  %[^_]%*[_]  %[^_]%*[_]"
#define GOT_NAME 0
#define HOST 1
#define SERVICE 2
#define SERVICE_HOST 3
#define MAX_TOTAL_HELD_MSGS 10000
#define DESIRED 2
#define DERROR -1
#define NSYNC_BUF "/smg/tmp/nsync_buf"
#define RESUB_BUF "/smg/tmp/resub_buf"

struct ips {
    char gname[100];
    char ip[20];
} ip_array[900];
int  mcnt=0;
int tm=0;
char g_host_date[100], g_service_date[100];
int g_modified_host = 0, g_modified_service=0;
int g_num_groups = 0;
char g_last_serv_buf[5000];
char *ver="v1.17";

/* ver 1.14 fixed hosts.cfg being modified with wrong hostgroup name for subnet
*/

char mbuf[10][300];

main(int argc, char **argv)
{
    char buf[10000], tmpbuf[10000], check_result[10000], *sptr;
    char cmd[200], host_name[1000], name[1000], service_desc[10000]; 
    char time_now[20], time_of_last_log_msg[20], group_name[200], ip[100];
    int i = 0, j = 0, update_type=0;
    int process_entire_log=0, reached_the_end_of_log=0;
    int previous_line_was_CHECK_RESULT=0, nog_needs_reload = 0;
    FILE *pp_log;
    long last_time=0;

    open_log_file(argv[0]);  // opens log file /var/opt/OV/log/argv[0]
    slog("-------------- process starting -------%s----\n", ver);
    //load_hostgroup_ip_array();


    if(argc < 2) { 
       printf("Doh...gots te tell me wats file yaz wants to tail!! BYE.\n"); 
       exit(0); 
    }
    //sprintf(cmd, "rm -rf %s 2>&1 > /dev/null", NSYNC_BUF);
    //system("rm -rf /smg/out/process_resub_buf.txt 2>&1 > /dev/null");
    system(cmd);

    sprintf(buf,"tail -f %s", argv[1]);
    if(!(pp_log = popen(buf,"r"))) { 
        slog("Can NOT popen %s\n",argv[1]);
        exit(3);
    } 

    while(1) {
        while (fgets(buf, 4999, pp_log)) {
            if(strstr(buf,"PROCESS_SERVICE_CHECK_RESULT")  ||
              (strstr(buf,"PROCESS_HOST_CHECK_RESULT")))

            { 
                slog("INBUF>%s<\n",buf);
                previous_line_was_CHECK_RESULT=1;
                strcpy(check_result, buf);
                continue; 
            } 
            if(strstr(buf, "Passive check result was received for service")) {
                if(!previous_line_was_CHECK_RESULT) 
                    slog("NO RESUB: >%s< NOT CHECK_RESULT\n",check_result);
                slog("INBUF>%s<\n",buf);
                if(strstr(buf,"service could not be found")) update_type=SERVICE;
                else if(strstr(buf,"host could not be found")) update_type=SERVICE_HOST;
                else { slog("UNKNOWN >%s<\n",buf); continue; }
                parse_warning_message_for_host_and_service( 
                     buf, host_name, service_desc);
                if(host_or_service_contains_cntrl_chars(host_name, service_desc)) {
                    slog("REJECTED: buf >%s<, host_name >%s<, service_desc >%s< \n", 
                     buf ,host_name, service_desc);
                     continue;
                }
            } else if(strstr(buf, "Passive check result was received for host")) { 
                if(!previous_line_was_CHECK_RESULT) 
                    slog("NO ReSUB: >%s< NOT CHECK_RESULT\n",check_result);
                slog("INBUF>%s<\n",buf);
                parse_warning_message_for_host(buf, host_name);
                if(host_or_service_contains_cntrl_chars(host_name, service_desc)) {
                    slog("REJECTED: buf >%s<, host_name >%s<, service_desc >%s< \n", 
                     buf ,host_name, service_desc);
                     continue;
                }
                update_type=HOST;
            
            } else { previous_line_was_CHECK_RESULT = 0; continue; }

            if( strlen(service_desc) < 2 || strlen(host_name) < 2 || 
                strlen(service_desc) > 100 || strlen(host_name) > 100) {
                slog("INVALID host >%s< or service name >%s<\n", 
                      host_name, service_desc);
                continue;
            }

            if(update_type!=SERVICE && !lookup_name_in_subnet(host_name, ip)){
                slog("FAILED subnet lookup of name >%s<. Can NOT add host which fails lookup\n", host_name);
                continue;
            }
 
            // LOOKS LIKE A NEW CFG ENTRY !!!
            if(update_type == HOST) { 
                get_group(check_result, group_name);
                if(update_host_cfg(host_name, group_name) == DERROR) 
                    continue;
            } else if(update_type == SERVICE_HOST) { 
                get_group(check_result, group_name);
                if(update_host_cfg(host_name, group_name) != DERROR) {
                   if(!update_service_cfg(host_name,service_desc,check_result))
                       continue;
                } else continue;
            } else update_service_cfg(host_name, service_desc, check_result);

            write_to_resub_buf(check_result);

        } 
#ifdef NEEDED
        slog("nothing to get.... sleeping....\n");
        strcpy(time_now,ts_hhmmss());
        if(elapsed_time_in_minutes(time_now, time_of_last_log_msg) > DESIRED){
            pclose(pp_log);
            slog("Calling restart_nsync...\n");
            system("/usr/local/sbin/restart_nsync");
        } else {
            slog("No need to restart: %s, %s: minute_diff:%d\n", 
                time_now, time_of_last_log_msg, 
                elapsed_time_in_minutes(time_now, time_of_last_log_msg));
        }
#endif
    } // while(1)
    slog("-------------- process complete -------%s----\n", ver);
}


int write_to_resub_buf(char *check_result)
{
    char *sptr, resubmission_buf[10000], clean_res[10000], buf[10000];
    int change=0;

    if(sptr = strstr(check_result, "EXTERNAL COMMAND: ")) {
        strncpy(clean_res, check_result, 13); clean_res[13] = '\0';
        strcat(clean_res, &check_result[31]);
        slog("check result WAS >%s< to >%s<\n", check_result, clean_res);
        strcpy(check_result, clean_res);
    }

    REMOVE_NEWLINE(check_result);
    strcpy(buf, check_result);

    if((sptr = strstr(check_result, "PROCESS_SERVICE_CHECK_RESULT;"))) {
        sptr += strlen("PROCESS_SERVICE_CHECK_RESULT;");
        while(*sptr != ',' && *sptr != ';' && *sptr != '\0') {
            if(isupper(*sptr)) { change=1; tolower(*sptr); }
            ++sptr;
        }
    }

    if(change) 
        slog("MODIFIED RESUB BUF from >%s< to >%s<\n",buf,check_result);

    sprintf(resubmission_buf,"echo \"%s\" >> %s", check_result, RESUB_BUF);
    slog("SYSTEM RSUB >%s<\n",resubmission_buf);
    system(resubmission_buf);
}


int parse_warning_message_for_host_and_service(
    char *buf, char *host_name, char *service_desc) 
{
    char *ptr, *hn, *sd, *sptr;
    int cnt = 0;
    hn=host_name;
    sd=service_desc;

    ptr=buf;
    // make sure there a 4 ' to parse on 
    while(ptr=strchr(ptr,'\'')) { ptr++; if(++cnt==1) sptr=ptr; }
    if(cnt != 4) { slog("cant find 4 ' in >%s<\n",buf); return 0; }

    // get service description
    while(*sptr != '\'') { *sd=*sptr; sd++; sptr++; } *sd = '\0'; ++sptr; 

    //get to beginning of host name
    while(*sptr != '\'') { sptr++; } ++sptr;

    // get host name
    while(*sptr != '\'') { *hn=*sptr; hn++; sptr++; } *hn = '\0';

    return 1;
}

int parse_warning_message_for_host(char *buf, char *host_name) 
{
    char *ptr, *hn, *sd, *sptr;
    int cnt = 0;
    hn=host_name;

    ptr=buf;
    // make sure there a 4 ' to parse on 
    while(ptr=strchr(ptr,'\'')) { ptr++; if(++cnt==1) sptr=ptr; }
    if(cnt != 2) { slog("cant find 2 ' in >%s<\n",buf); return 0; }

    // get host name
    while(*sptr != '\'') { *hn=*sptr; hn++; sptr++; } *hn = '\0'; ++sptr; 

    return 1;
}

int service_in_cfg(char *host_name, char *service_name)
{
    FILE *fp_scfg;
    char *ptr, *sptr, service_buf[200], tmp_buf[200], buf[1000];
    char tmp_service_desc[200], cfg_host_name[200], cfg_service_name[200];
    char lower_cfg_host[100], lower_nsca_host[100];
    int loop=0;

#ifdef NEEDED
    while(1) {
      if(!(fp_scfg = fopen("/usr/local/nagios/etc/objects/services.cfg","r"))){
        slog("Can NOT fopen services.cfg\n");
        fflush(stdout);
        system("sleep 1");
        if(loop++ > 5) {
          slog("Failed 5 consecutive times opening services.cfg. Not adding service %s\n", service_name);
          return 1;
          //exit(10);
        } 
      } else break;
    }
#endif
   if(!(fp_scfg = popen("cat /usr/local/nagios/etc/objects/services.cfg","r")))
   {
        slog("Can NOT popen services.cfg\n");
        return 1;
    }
    while (fgets(buf, BUFSIZ, fp_scfg)) {
        if(ptr=strstr(buf,"service_description")) {
            ptr+=strlen("service_description");
            while(*ptr == ' ' || *ptr == '\t') ++ptr;
            strcpy(cfg_service_name,ptr);
            if(sptr=strchr(cfg_service_name,'\n')) *sptr = '\0';
            while (fgets(buf, BUFSIZ, fp_scfg)) {
                if(buf[0] == '}') break;
                ptr=strstr(buf, "\thost_name");
                if(ptr) {
                    ptr+=strlen("host_name	");
                    while(*ptr == ' ' || *ptr == '\t') 
                        ++ptr;
                    strcpy(cfg_host_name,ptr);
                    if(sptr=strchr(cfg_host_name,'\n')) *sptr = '\0';
                    if(!strcmp(host_name, cfg_host_name) &&
                       !strcmp(service_name, cfg_service_name)) {
                        pclose(fp_scfg);
                        return 1;
                    }
                 }
            }
        }
    }
    pclose(fp_scfg);
    return 0;
}

void get_field(char *buf, char *field)
{
     char *sptr;

     while(isspace(*buf)) ++buf;
     while(!isspace(*buf)) ++buf;
     while(isspace(*buf)) ++buf;
     if(sptr=strchr(buf,'\n')) *sptr = '\0';
     strcpy(field,buf);
}

int service_in_stateless_cfg(char *host_name, char *service_name)
{
    FILE *fp_scfg;
    char *ptr, *sptr, service_buf[200], tmp_buf[200], buf[1000];
    char tmp_service_desc[200], cfg_host_name[200], cfg_service_name[200];
    char lower_cfg_host[100], lower_nsca_host[100];
    int loop=0, found_host=0;

    while(1) {
      if(!(fp_scfg = fopen("/usr/local/nagios/etc/objects/stateless_services.cfg","r"))){
        slog("Can NOT fopen stateless_services.cfg\n");
        fflush(stdout);
        system("sleep 1");
        if(loop++ > 5) {
          slog("Failed 5 consecutive times opening stateless_services.cfg. Not adding service %s\n", service_name);
          return 1;
          //exit(10);
        }
      } else break;
    }
    while (fgets(buf, BUFSIZ, fp_scfg)) {
        if(ptr=strstr(buf,"service_description")) {
            ptr+=strlen("service_description");
            while(*ptr == ' ' || *ptr == '\t') ++ptr;
            strcpy(cfg_service_name,ptr);
            if(sptr=strchr(cfg_service_name,'\n')) *sptr = '\0';
            while (fgets(buf, BUFSIZ, fp_scfg)) {
                if(buf[0] == '}') break;
                ptr=strstr(buf, "\thost_name");
                if(ptr) {
                    get_field(buf, cfg_host_name);
                    if(!strcmp(host_name, cfg_host_name)) found_host = 1;
                    if(!strcmp(host_name, cfg_host_name) &&
                       !strcmp(service_name, cfg_service_name))
                         return 1;
                }
            }
        }
    }
    if(!found_host) slog("Never found host %s\n", host_name);
    return 0;
}


host_in_cfg(char *host_name)
{
    FILE *fp_hcfg;
    char buf[1000], cfg_host_name[100], *sptr, *ptr;
    char lower_cfg_host[100], lower_nsca_host[100];
    int loop=0;
    
    if(!strcmp(host_name, "localhost")) {
        slog("host_in_cfg: got localhost - returning 1\n");
        return 1;
    } else slog("host_in_cfg: checking for host >%s<\n", host_name);

    while(1) {
      if(!(fp_hcfg = fopen("/usr/local/nagios/etc/objects/hosts.cfg","r"))){
        slog("Can NOT fopen hosts.cfg\n");
        fflush(stdout);
        system("sleep 1");
        if(loop++ > 5) {  
          slog("Failed 5 consecutive times opening hosts.cfg. Not adding hgost %s\n", host_name);
          return 1; 
          //exit(11);
        }
      } else break;
    }

    slog("checking to see if >%s< is in hosts.cfg\n", host_name);
    while (fgets(buf, BUFSIZ, fp_hcfg)) {
        if(ptr=strstr(buf,"host_name")) {
            ptr+=9;
            while(*ptr == ' ' || *ptr == '\t') ++ptr;
            strcpy(cfg_host_name,ptr);

            if(sptr=strchr(cfg_host_name,'\n')) *sptr = '\0';

            strcpy(lower_cfg_host, lower_case(cfg_host_name));
            strcpy(lower_nsca_host, lower_case(host_name));
            //strcpy(lower_cfg_host, cfg_host_name);
            //strcpy(lower_nsca_host, host_name);

            if(!strcmp(lower_cfg_host, lower_nsca_host)) {
                slog("lower(cfg_host_name) = %s, lower(host_name) = %s\n",
                 lower_cfg_host, lower_nsca_host);
                 slog("ALREADY in cfg: host: %s\n",host_name);
                 fclose(fp_hcfg);
                 return 1;
            }
        }
    }
    fclose(fp_hcfg);
    return 0;
}

int load_hostgroup_ip_array()
{
    char *sptr, buf[50000], var[100];
    int i = 0, j = 0, k = 0;
    FILE *fpw, *fp;

    slog("in load_hostgroup_ip_array\n");
    if(!(fp = fopen("/usr/local/nagios/etc/objects/hostgroups.cfg", "r"))) {
        slog("can not open %s\n","hostgroups.cfg");
        exit(1);
    }

    while(fgets(buf,5000,fp)) {
        if(strstr(buf,"hostgroup_name")) {
            if(strstr(buf,"Subnet ")) continue;
            /*slog("callin get_var(%s, %s)\n",buf,var);*/
            get_var(buf,var);
            /*slog("get_var returns >%s<\n", var);*/
            strcpy(ip_array[g_num_groups].gname, var);
            REMOVE_NEWLINE(ip_array[g_num_groups].gname);
            //if(sptr=strchr(ip_array[g_num_groups].gname,'\n')) *sptr = '\0';
        } else if(strstr(buf,"alias")) {
            if(strstr(buf,"Subnet ")) continue;
            if(sptr = strstr(buf,"Monitor sending NSCA from")) {
                sptr += 26;
                strcpy(ip_array[g_num_groups].ip, sptr);
                REMOVE_NEWLINE(ip_array[g_num_groups].ip);
                //if(sptr=strchr(ip_array[g_num_groups].ip,'\n')) *sptr = '\0';
            } else if(sptr = strstr(buf,"Subnet ")) {
                sptr += 7;
                strcpy(ip_array[g_num_groups].ip, sptr);
                REMOVE_NEWLINE(ip_array[g_num_groups].ip);
                //if(sptr=strchr(ip_array[g_num_groups].ip,'\n')) *sptr = '\0';
            } else {
                slog("ERROR: alias line contains neither NSCA or Subnet!!\n",
                 buf);
            }
            ++g_num_groups;
        } 
    }
    
    for(i = 0; i < g_num_groups; i++) {
        slog("host group %d: nm >%s<, ip >%s<\n",
              i+1, ip_array[i].gname, ip_array[i].ip);
    }  
    fclose(fp);
    slog("loaded %d host groups\n",g_num_groups);
}

/* going through:
         hostgroup_name                  Subnet 155.100.139
*/
int get_var(char *buf, char *var)
{
    char *ptr;

    ptr=buf;

    while(isblank(*ptr)) ++ptr;

    while(!(isblank(*ptr))) ++ptr;

    while(isblank(*ptr)) ++ptr;

    strcpy(var,ptr);
}

/* this routine will compare two strings like
   eventlog_1002_nowisthetimeforallgoodmen
    and 
   eventlog_1002_timeforall
     WHICH SHOULD MATCH!!
   first, break into three peices with the underscore as a dilimeter
   the first two peices must match exactly, and for the third ,
   the cfg_service_name must either match or be a substring of service_name 
*/

#define USV3 "%[^_]%*[_]  %[^_]%*[_]  %[^_]%*[_]"
/*
 PROCESS_SERVICE_CHECK_RESULT;ovpi;eventlog_667_testingtoseeeifwecanfindjimmyisadonkeyionthistext;2;testing to seee if we can find jimmy is a donkey ion this text (155.100.122.36)
*/
#ifdef NEEDED
#define SSV4 "%[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]  %[^;]%*[;]"
int replace(char *cfg_name, char *resub_buf)
{
    char sn1[100], sn2[100], sn3[100]; 
    char csn1[100], csn2[100], csn3[100], new_cfg_name[1000];
    char service_name[300], head[300], node[300], elog[100], id[100],
           event[300], error_val[300], text[5000], eltext[100], tmpbuf[5000];

    slog("replace(%s, %s)\n", cfg_name, resub_buf);

    sscanf(resub_buf, SSV4, head, node, event, error_val, text);
    sscanf(event, USV3, elog, id, eltext);
    //sprintf(new_cfg_name, "%s_%s_%s", elog, id, cfg_name);
    sprintf(tmpbuf,"%s;%s;%s;%s;%s", head, node, cfg_name, error_val, text);
    slog("NEW RESUB BUF is >%s<\n", tmpbuf);
    strcpy(resub_buf, tmpbuf); 
}
#endif
int message_comming_from_satellite_get_ip(char *resub_buf, char *r_ip) {
    char *sptr, *last_left_pren, *ip_ptr, ip[1000];

#ifdef DEBUG
    slog("IN message_comming_from_satellite_get_ip(%s)\n",resub_buf);
#endif
    if(!(sptr = strchr(resub_buf,'('))) return 0;
    last_left_pren = sptr++;
    while(sptr = strchr(sptr,'(')) last_left_pren = sptr++;
    strcpy(ip,++last_left_pren);
    slog(" mcfsgi: ip is >%s<\n", ip);
    if((sptr = strchr(ip,')'))) {
        if(*(sptr+1) != '\n' && *(sptr+1) != '\r' && *(sptr+1) != '\0') {
            slog("INFO: NOT FROM SAT: DATA after right pren following last left pren is followed by data: buf>%s< char>%c< int>%d<", resub_buf, *(sptr+1), *(sptr+1));
            return 0;
        }
        *sptr='\0';
    } else { slog ("found right pren but NO LEFT!\n"); return 0; }
    slog(" mcfsgi: ip NOW is >%s<\n", ip);
    ip_ptr = ip;
    if(strlen(ip) > 16) { 
        slog("string tooo long to be an ip >%s<\n", ip);
        return 0;
    }
    // just make sure we have only digits and periods
    while(*ip_ptr) { if(*ip_ptr != '.' && !isdigit(*ip_ptr)) break; ++ip_ptr; }

#ifdef DEBUG
    slog("OUT message_comming_from_satellite_get_ip()\n");
#endif
    if(*ip_ptr) return 0;  // if we arn't at null, its not a valid ip
    strcpy(r_ip, ip);
    return 1;
}

int message_contains_template_info(char *resub_buf, char *template) {
    char *sptr, *last_left_pren, *template_ptr;

    if(!(sptr = strchr(resub_buf,'('))) return 0;
    last_left_pren = sptr++;
    while(sptr = strchr(sptr,'('))
        last_left_pren = sptr++;
    strcpy(template,++last_left_pren);

//    if(sptr = strchr(template,')')) *sptr='\0';
//    else { slog ("found right pren but NO LEFT!\n"); return 0; }

    slog("lookin at last_left_pren in >%s<>%s<\n", last_left_pren, template);
    if(sptr = strchr(template,')')) {
        if(*(sptr+1) != '\n' && *(sptr+1) != '\r' && *(sptr+1) != '\0') {
            slog("INFO: DATA after,right pren following last left pren is followed by data >%s<", resub_buf);
            return 0;
        }
        *sptr='\0';
    } else { slog ("found right pren but NOOO LEFT!\n"); return 0; }
    if(template_is_valid(template)) return 1;
    else { slog("invalid template >%s<\n", template); return 0; }
}


int template_is_valid(char *service_being_validated)
{
    FILE *fpr;
    char buf[1000], serv_buf[1000], *sptr, *service;

    if(!(fpr = fopen("/usr/local/nagios/etc/objects/services_template.cfg","r"))){ 
        printf("noop /usr/local/nagios/etc/objects/services_template.cfg");
        exit(1);
    }
    while(fgets(buf,4999,fpr)) {
        if(strncmp(buf,"define service{", strlen("define service{")))
            continue;
        fgets(buf,4999,fpr);
        if(!(sptr=strstr(buf, "name"))) { fclose(fpr); return 0; }

        sptr+=4;
        memset(serv_buf,0,sizeof(serv_buf));
        service=serv_buf;
        while(isblank(*sptr)) ++sptr;
        while(!isblank(*sptr)) *(service++) = *(sptr++);
        *service='\0';
        if(!strcmp(serv_buf, service_being_validated)) return 1;
    }
    fclose(fpr);
    return 0;
}

int replace_illegal_chars(char *service_name)
{
  char *sp;
  sp=service_name;
  while(*sp) { if(*sp == '(' || *sp == ')') *sp = ' '; ++sp; }
}

// just getting satt name
int get_group(char *resub_buf, char *group_name)
{
    FILE *pptr;
    char *sptr, *last_left_pren, buf[1000], ip[300],
         template[200], dummy[200];
    int updated=0, i, found_group=0, subnet=0, subnet_lookup_failed=0;

        memset(group_name,0,sizeof(group_name));

        if(!message_comming_from_satellite_get_ip(resub_buf, ip))
            return 0;

        if(!lookup_ip_in_subnet(ip, group_name)) 
            return 0;

        if(sptr=strchr(group_name,'.')) *sptr='\0';
        slog("get_group returns %s\n", group_name);

        return 1;
}


int update_host_cfg(char *host_name, char *grp_name)
{
    FILE *fp, *fptr;
    char buf[500], cmd[300], lower_host[100], resub_service_name[300], ip[100];
    char template[200], cfg_file[200], dummy[200];
    int loop=0, host_already_defined=0, template_info=0;

    strcpy(lower_host, (char *)lower_case(host_name));
    slog("In update_host_cfg(host: %s, grp: %s)\n",host_name, grp_name);
    
    if(host_in_cfg(host_name)) {
        slog("host %s alredy in cfg\n", host_name);
        return 0;
    }

    if(!lookup_name_in_subnet(lower_host, ip)) {
        slog("IP LOOKUP ERROR: ip_lookup_in_subnet(%s)\n",lower_host);
        return -1;
    }

    if(*grp_name)
        sprintf(buf,"echo \"\n#%s\ndefine host {\n\thost_name\t\t\t%s\n\talias    \t\t\t%s\n\taddress\t\t\t\t%s\n\thostgroups\t\t\t%s\n\tuse\t\t\t\tpassive-host\n}\n\n\" >> /usr/local/nagios/etc/objects/hosts.cfg",date_stamp(dummy), lower_host, lower_host, ip, grp_name);
    else sprintf(buf,"echo \"\n#%s\ndefine host {\n\thost_name\t\t\t%s\n\talias    \t\t\t%s\n\taddress\t\t\t\t%s\n\tuse\t\t\t\tpassive-host\n}\n\n\" >> /usr/local/nagios/etc/objects/hosts.cfg",date_stamp(dummy), lower_host, lower_host, ip);
    slog("system: %s\n",buf);
    system(buf);

    return 1;
}


int update_service_cfg(char *host_name, char *service_name, char *resub_buf)
{
    FILE *fp, *fptr;
    char buf[500], cmd[300], lower_host[100], resub_service_name[300], ip[100];
    char template[200], cfg_file[200], dummy[200];
    int loop=0, host_already_defined=0, template_info=0;

    strcpy(lower_host, (char *)lower_case(host_name));

    if(message_contains_template_info(resub_buf, template)) {
        slog("Message >%s< contains template info >%s<.\n", 
            resub_buf, template);
        template_info=1;
    } else {
        strcpy(template,"Passive_template");
        slog("Message DOES NOT contains template info >%s<.\n", resub_buf);
        template_info=0;
    }

    slog("Checking if >%s< is a stateless service for host >%s<\n",service_name, lower_host);

    if(SERVICE_IS_STATELESS) {
        if(service_in_stateless_cfg(lower_host, service_name)) {
            slog("service >%s< for host >%s< is ALREADY IN stateless_services.cfg\n", service_name, lower_host);
            return 0;
        } else slog("service >%s< host >%s< is NOT IN stateless.cfg. ADDING\n",
                service_name, lower_host);
        strcpy(cfg_file,"stateless_services.cfg");
        sprintf(buf,"echo \"\n#%s\n\ndefine service {\n\tservice_description\t\t%s\n\tuse\t\t\t\t%s\n\thost_name \t\t\t%s\n\tcheck_command\t\t\tcheck_dummy\n\tis_volatile\t\t\t1\n}\n\n\" >> /usr/local/nagios/etc/objects/%s",date_stamp(dummy), service_name, template, lower_host, cfg_file);
    } else { // service is NOT stateless
        if(service_in_cfg(lower_host, service_name)) {
            slog("Service >%s< is ALREADY defined for host>%s<\n",
              service_name, lower_host);
           return 0;
        } else slog("service >%s< host >%s< is NOT IN services.cfg ADDING\n",
                service_name, lower_host);
        strcpy(cfg_file,"services.cfg");
        sprintf(buf,"echo \"\n#%s\n\ndefine service {\n\tservice_description\t\t%s\n\tuse\t\t\t\t%s\n\thost_name \t\t\t%s\n\tcheck_command\t\t\tcheck_dummy\n}\n\n\" >> /usr/local/nagios/etc/objects/%s",date_stamp(dummy), service_name, template, lower_host, cfg_file);
    }



    // If nagios gets blasted with the same 
    // message, this will prevent it from being duplicated in the cfgs
    if(!strcmp(g_last_serv_buf, buf)) {
        slog("DUPLICATE - same as last ss.cfg entry:\n\t>%s<\n", buf);
        return 0;
    }

    slog("system: %s\n",buf);
    system(buf);
    strcpy(g_last_serv_buf, buf);

    return 1;
}

int host_or_service_contains_cntrl_chars(char *host_name, char *service_desc) {
    int i; char *sptr;

    for (i=0, sptr=host_name; i<2; sptr=service_desc, i++) {
         while(*sptr) { if(iscntrl(*sptr)) return 1; ++sptr; }
    }
    return 0;
}
