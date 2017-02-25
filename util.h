#include <string.h>
#include <time.h>
#ifndef BIN_PATH
#define BIN_PATH "/smg/bin"
#endif
#ifndef NAG_BIN_PATH
#define NAG_BIN_PATH "/usr/local/nagios/libexec"
#endif
#define NSLOOKUP_SUCCESS 0
#define PING_OK 0
#define PING_WARNING 1
#define PING_CRITICAL 2
#define UNKNOWN 3
#define PING_PIPE_FAILED 4
#define PING_PARSE_FAILED 5
#define PING_EXECUTION_FAILED 6
int recoverable_err(char *buf, char *cmd) {
        if(strstr(buf,"Connection refused") ||
             strstr(buf,"Permission denied") ||
             strstr(buf,"can't be established") )
        {
           slog("FATEL ERROR. got >%s<\n", buf);
           return 0;
        }
        if(strstr(buf,"REMOTE HOST IDENTIFICATION HAS CHANGED")) {
           slog("looks like host id has changed. got >%s<\n", buf);
           return 0;
        }
        if(strstr(buf,"sudo: sorry")) {
           slog("SUDO error: popen(%s) returns >%s<\n", cmd, buf);
           return 0;
        }
        if(strstr(buf,"No such file")) {
           slog("No such file: popen(%s) returns >%s<\n", cmd, buf);
           return 0;
        }
        if(strstr(buf,"does not exist")) {
           slog("The file does not exist: popen(%s) returns >%s<\n", cmd, buf);
           return 0;
        }
        if(strstr(buf,"POSSIBLE BREAK-IN ATTEMPT")) {
           slog("Setup Error: >%s< returns >%s<\n", cmd, buf);
           return 0;
        }
        if(strstr(buf,"denied")) {
           slog("SUDO ERROR: nagadm MUST BE ADDED TO SUDOers to do 'ls'!\n");
           return 0;
        }
        if(strstr(buf,"not found")) {
           slog("FILE DOES NOT EXIST:\n\t>%s< returns >%s<\n", cmd, buf);
           return 0;
        }
        return 1;
}

long epoch_time(char *buf)
{
    struct tm t, t2;
    time_t t_of_day, cur_t_of_day;
    char ddate[300];

    strcpy(ddate,buf);
    if(strlen(ddate) != 12) return time(NULL); // return current time

    t.tm_sec = atoi(&ddate[10]);ddate[10]='\0';
    t.tm_min = atoi(&ddate[8]); ddate[8]='\0';
    t.tm_hour =atoi(&ddate[6])+1; ddate[6]='\0';
    t.tm_mday =atoi(&ddate[4]); ddate[4]='\0';
    t.tm_mon = atoi(&ddate[2])-1; ddate[2]='\0';
    t.tm_year = atoi(ddate)+100;
    t.tm_isdst = 1;
    t_of_day = mktime(&t);

    printf("convert time isdst = 1: %ld\n", (long) t_of_day);
    t.tm_isdst = 0;
    t_of_day = mktime(&t);
    printf("convert time isdst = 0: %ld\n", (long) t_of_day);
    t.tm_isdst = -1;
    t_of_day = mktime(&t);
    printf("convert time isdst = -1: %ld\n", (long) t_of_day);
    cur_t_of_day = time(NULL);
    printf("current: %ld\n", (long) cur_t_of_day);
    return t_of_day;
}

long convert_mstime_to_epoch(char *msdate)
{
char yydate[13];
// 
//2011-11-08 19:19:01.50 Logon       Error: 18456, Severity: 14, State: 16.
//or
//2011-11-08 19:19:01.50 Logon       Login failed for user 'SRVR\ADSXENPROV'. [CLIENT: 155.100.123.12]
yydate[0]=msdate[2];
yydate[1]=msdate[3];

yydate[2]=msdate[5];
yydate[3]=msdate[6];

yydate[4]=msdate[8];
yydate[5]=msdate[9];

yydate[6]=msdate[11];
yydate[7]=msdate[12];
yydate[8]=msdate[14];
yydate[9]=msdate[15];
yydate[10]=msdate[17];
yydate[11]=msdate[18];
yydate[12]='\0';
printf("calling epoch(%s)\n", yydate);
return epoch_time(yydate);
}


int lower_case_string(char *in_string, char *out_string)
{
    char *sptr;
    sptr=out_string;
    while(*in_string) *(sptr++) = tolower(*(in_string++));
    *sptr='\0';
}

int strfcmp(char *buf1, char *buf2, char delim, char ignore, int up2nth_field)
{
    int cur_field = 0;
    while(*buf1 && *buf2) {
        while(*buf1 == ignore && *buf1) ++buf1;
        while(*buf2 == ignore && *buf2) ++buf2;
        if(!*buf1 || !*buf2) break;
        if(tolower(*buf1) != tolower(*buf2)) return 1;
        if(*buf1 == delim) if(++cur_field == up2nth_field) return 0;
        ++buf1; ++buf2;
    }
    if(cur_field + 1 != up2nth_field) { // if up to last field is requested
        return 2;
    }
    return 0;
}

int ping(char *ip, char *result_text)
{
    FILE *pp;
    char *sptr, *tptr, host[200], cmd[150], buf[200];
    char rtab[20], pct_lossb[20], *rta, *pct_loss;

    rta=rtab; pct_loss=pct_lossb;

    //sprintf(cmd,"%s/check_icmp %s 72.20.132.78 -w800,80% -c1000,100%", BIN_PATH, ip);
    sprintf(cmd,"%s/check_icmp -c 500000.00,100%% -w 200.00,20%% -t 5 %s", 
          NAG_BIN_PATH, ip);
    slog("trying >%s<\n", cmd);
    if(!(pp=popen(cmd,"r"))) { 
        sprintf(result_text, "cant open pipe >%s<\n",cmd); 
        slog("ping(): cant open pipe  >%s<\n",cmd); 
        return PING_PIPE_FAILED; 
    }
    fgets(buf, 200, pp);
    pclose(pp);

// CRITICAL - 155.100.238.209: rta nan, lost 100%|rta=0.000ms;200.000;500.000;0; pl=100%;40;80;; 
// WARNING - 155.100.238.209: rta nan, lost 60%|rta=0.222ms;200.000;500.000;0; pl=100%;40;80;; 
// OK - 155.100.118.12: rta 0.243ms, lost 0%|rta=0.243ms;200.000;500.000;0; pl=0%;40;80;;
//slog("ping(%s): returns >%s<\n", cmd, buf);

    //tptr=strstr(buf, "pl="); sptr=strstr(buf, "rta=");
    //if(*sptr) {
    if(sptr=strstr(buf, "rta=")) {
        sptr+=4;
        while(*sptr!='m') *(rta++) = *(sptr++);
        *rta='\0';
        if(!(sptr=strstr(buf, "pl="))) {
            sprintf(result_text,"ERROR: Could not parse buf >%s<\n", buf);
            slog("ERROR: Could not parse buf >%s<\n", buf);
            return PING_PARSE_FAILED;
        }
        sptr+=3; 
        while(*sptr!='%') *(pct_loss++) = *(sptr++);
        *pct_loss='\0';
    } else {
        REMOVE_NEWLINE(buf);
        strcpy(result_text,buf);
        return PING_EXECUTION_FAILED;
    }
    //slog(">%s< >%s<\n", rtab, pct_lossb);

    if(nslookup(ip, host) != NSLOOKUP_SUCCESS) strcpy(host, ip);
    
    sprintf(result_text,"%s %s %s %s",host, ip, rtab, pct_lossb);
    //if(sptr = strchr(result_text, ' ')) *sptr='\0';
    if(!strncmp(buf,"OK - ",5))
        return PING_OK;
    if(!strncmp(buf,"WARNING - ",10))
        return PING_WARNING;
    if(!strncmp(buf,"CRITICAL - ",11))
        return PING_CRITICAL;
    return UNKNOWN;
}

int sent_appinfo_event_due_to_failed_ping(char *host, char *host_ip, char *service, char *agg_ip)
{
    char result_text[1000], info[1000], service_name[200];
    char tmp_host[200];
    int ping_result;


    ping_result = ping(host_ip, result_text);

    if(ping_result != PING_OK && ping_result != PING_WARNING) {
        if(nslookup(host_ip, tmp_host) != NSLOOKUP_SUCCESS) {
            strcat(result_text, tmp_host);
        }
        sprintf(info, "ping failed, returned %d >%s<\n", 
              ping_result, result_text);
        slog("event info >%s<\n", info);

        sprintf(service_name, "%s_ping", service);
        slog("service_name >%s<\n", service_name);

        slog("saedtf: calling send_appinfo_event(%s, %s, %d, %s, %s)\n", 
           host, service_name, 2, info, agg_ip);
//        if(!send_appinfo_event(host, service_name, 2, info, agg_ip))
//           slog("FAILED send_appinfo_event(%s %s %s)\n",
//                  host, service_name, result_text); 
        return 1;
    } else return 0;
}
// subnet will look like 'subnet.155.100.102'
int send_appinfo_subnet_event(char *subnet, char *service, int result,
        char *result_text, char *agg_ip)
{
    FILE *pp;
    char *sptr, appliance_name[200], app_ip[100], app_fq_name[200];
    char service_name[200], non_qual_host[200];

    if(!(pp=popen("hostname","r"))) { slog("cant open >%s<\n"); return 0; }
    else {
        fgets(appliance_name,100,pp);
        pclose(pp);
        REMOVE_NEWLINE(appliance_name);
        slog("Appliance name is >%s<\n", appliance_name);
        if(sptr = (char *)strchr(appliance_name,'\n')) *sptr = '\0';
        if(sptr = (char *)strchr(appliance_name,'\r')) *sptr = '\0';
        if(nslookup(appliance_name, app_ip) == NSLOOKUP_SUCCESS)
            nslookup(app_ip, app_fq_name);
        else strcpy(app_fq_name, appliance_name);
        //slog("Fully Qualified Appliance name is >%s<\n", app_fq_name);
    }

    sprintf(service_name, "appinfo_%s_%s", subnet, service);
    //slog("service_name >%s<\n", service_name);

    //slog("send_nsca(%s %s %d %s %s)\n", 
    //   app_fq_name, service_name, result, result_text, agg_ip);
    send_nsca(app_fq_name, service_name, result, result_text, agg_ip);
    return 1;
}


int send_appinfo_event(char *host, char *service, int result, 
        char *result_text, char *agg_ip)
{
    FILE *pp;
    char *sptr, appliance_name[200], app_ip[100], app_fq_name[200];
    char service_name[200], non_qual_host[200];

#ifdef I_DONT_LIKE_LOCALHOST_FOR_MY_AGG_NAME_SO_USE_WHAT_HOSTNAME_RETURNS
    if(!(pp=popen("hostname","r"))) { slog("cant open >%s<\n"); return 0; }
    else {
        fgets(appliance_name,100,pp);
        pclose(pp);
        REMOVE_NEWLINE(appliance_name);
        slog("Appliance name is >%s<\n", appliance_name);
        if(sptr = (char *)strchr(appliance_name,'\n')) *sptr = '\0';
        if(sptr = (char *)strchr(appliance_name,'\r')) *sptr = '\0';
        if(nslookup(appliance_name, app_ip) == NSLOOKUP_SUCCESS)
            nslookup(app_ip, app_fq_name);
        else strcpy(app_fq_name, appliance_name);
        //slog("Fully Qualified Appliance name is >%s<\n", app_fq_name);
    }
#else
    strcpy(app_fq_name,"localhost");
#endif

    strcpy(non_qual_host, host);
    if(sptr = strchr(non_qual_host,'.')) *sptr = '\0';
    sprintf(service_name, "appinfo_%s_%s", non_qual_host, service);
    //slog("service_name >%s<\n", service_name);

    //slog("send_nsca(%s %s %d %s %s)\n", 
    //   app_fq_name, service_name, result, result_text, agg_ip);
    send_nsca(app_fq_name, service_name, result, result_text, agg_ip);
    return 1;
}

int send_appinfo_ping_event(char *subnet, char *service, int result,
        char *result_text, char *agg_ip)
{
    FILE *pp;
    char *sptr, appliance_name[200], app_ip[100], app_fq_name[200];
    char service_name[200], non_qual_host[200];

    if(!(pp=popen("hostname","r"))) { slog("cant open >%s<\n"); return 0; }
    else {
        fgets(appliance_name,100,pp);
        pclose(pp);
        REMOVE_NEWLINE(appliance_name);
        slog("Appliance name is >%s<\n", appliance_name);
        if(sptr = (char *)strchr(appliance_name,'\n')) *sptr = '\0';
        if(sptr = (char *)strchr(appliance_name,'\r')) *sptr = '\0';
        if(nslookup(appliance_name, app_ip) == NSLOOKUP_SUCCESS)
            nslookup(app_ip, app_fq_name);
        else strcpy(app_fq_name, appliance_name);
        //slog("Fully Qualified Appliance name is >%s<\n", app_fq_name);
    }
    sprintf(service_name, "appinfo_%s_%s", subnet, service);
    //slog("service_name >%s<\n", service_name);

    //slog("send_nsca(%s %s %d %s %s)\n", 
    //   app_fq_name, service_name, result, result_text, agg_ip);
    send_nsca(app_fq_name, service_name, result, result_text, agg_ip);
    return 1;
}




int nag_fld_val(char *buf, char *fld, char *val)
{
    char *name, name_buf[100];

    name = name_buf;

    while(isspace(*buf)) ++buf;
    while(!isspace(*buf)) *(fld++) = *(buf++);
    *fld='\0';

    while(isspace(*buf)) ++buf;
    while(!isspace(*buf) && !iscntrl(*buf)) *(val++) = *(buf++);
    *val='\0';
}

int lookup_ip_in_subnet(char *ip, char *nm)
{
    FILE *pipe;
    long got_line=0;
    char *sptr, buf[1000], cmd[1000], ping_result[500], *p;
    int cnt=0, in_type, return_code = 3;

    //sprintf(cmd,"/smg/bin/lookup_ip_in_subnet.sh %s", ip);
    sprintf(cmd, "grep %s[[:space:]] /smg/cfg/ping/* 2>/dev/null | cut -f2 -d':' | cut -f1 -d' '", ip);
    slog("attempting popen(%s)\n", cmd);
    if(!(pipe = popen(cmd,"r"))) {
        return 0;
    }

    while (fgets(buf, 4999, pipe)) {
        ++cnt; 
        if(sptr=strchr(buf,'\n')) *sptr = '\0'; 
        strcpy(nm, buf);
        slog("lookup_ip_in_subnet(%s) returns >%s<\n", ip, nm);
    }

    pclose(pipe);

    if(cnt < 1) {
        slog("ERROR: command  >%s< returns ZERO LINES ", cmd);
        return 0;
    }
    if(cnt > 1) {
        slog("ERROR: command  >%s< returns %d LINES (TOO MANY!!)", cmd, cnt);
        return 0;
    }
    if(strstr(ip, "NOT_FOUND")) return 0;
    return 1;
}

int lookup_name_in_subnet(char *nm, char *ip)
{
    FILE *pipe;
    long got_line=0;
    char *sptr, buf[1000], ping_result[500], *p;
    int in_type, cnt=0, return_code = 3;

    //sprintf(buf,"/smg/bin/lookup_name_in_subnet.sh %s", nm);

    sprintf(buf, "grep [[:space:]]%s$ /smg/cfg/ping/* 2>/dev/null | cut -f2 -d':' | cut -f1 -d' '", nm);

    slog("attempting popen(%s)\n", buf);
    if(!(pipe = popen(buf,"r"))) {
        return 0;
    }

    while (fgets(buf, 4999, pipe)) {
        ++cnt; 
        if(sptr=strchr(buf,'\n')) *sptr = '\0';
        strcpy(ip, buf);
        slog("lookup_name_in_subnet(%s) returns >%s<\n", nm, ip);
    }
    
    pclose(pipe);

    if(cnt != 1 || strstr(ip, "NOT_FOUND")) {
        slog("SUBNET LOOKUP ERROR: >%s< ", buf);
        if(nslookup(nm, ip) != NSLOOKUP_SUCCESS)
            return 0;
    }
    return 1;
}


#define NAME 1
#define IP 2


int nslookup_noping(char *in_nm_or_ip, char *out_nm_or_ip)
{
    FILE *pipe;
    long got_line=0;
    char *sptr, buf[1000], ping_result[500], *p;
    int in_type, return_code = 3;

    sprintf(buf,"nslookup %s", in_nm_or_ip);
    slog("attempting popen(%s)\n", buf);
    if(!(pipe = popen(buf,"r"))) {
        return 1;
    }

    // is it a name, or an ip address?
    for (p = in_nm_or_ip; *p; ++p) if(!isdigit(*p) && *p != '.') break;
    in_type = *p ? NAME : IP;

    while (fgets(buf, 4999, pipe)) {
        //slog(" in nslookup.. got >%s<\n", buf);
        if(!got_line++) return_code = 2;
        if(in_type == IP) {
            if(found_in_etc_hosts(in_nm_or_ip, out_nm_or_ip, in_type)) {
#ifdef DEBUG
slog("Found match for >%s< being >%s< in /etc/hosts\n", in_nm_or_ip, out_nm_or_ip);
#endif
                return_code =   NSLOOKUP_SUCCESS;
                break;
            }
            strcpy(out_nm_or_ip,"IP NOT IN DNS");
            if(sptr=strstr(buf,"name =")) {
                sptr+=7;
                strcpy(out_nm_or_ip,sptr);
                if(sptr=strchr(out_nm_or_ip, '\n'))
                     *(--sptr) = '\0'; // get rid of '.' and \n at end of name
                return_code =  NSLOOKUP_SUCCESS;
                break;
            }
        } else if(in_type == NAME) {
            if(found_in_etc_hosts(in_nm_or_ip, out_nm_or_ip, in_type)) {
#ifdef DEBUG
slog("Found match for >%s< being >%s< in /etc/hosts\n", in_nm_or_ip, out_nm_or_ip);
#endif
                return_code =   NSLOOKUP_SUCCESS;
                break;
            }
            strcpy(out_nm_or_ip,"NAME NOT IN DNS");
            if(!strncmp(buf,"Name:", 5)) {
                fgets(buf, 4999, pipe);
                if(!strncmp(buf,"Address:", 8)) {
                    strcpy(out_nm_or_ip,&buf[9]);
                    REMOVE_NEWLINE(out_nm_or_ip);
                    slog("nslookup: found address >%s<. NOT Testing ping.",
                             out_nm_or_ip);
//                    if(!myping(out_nm_or_ip, ping_result)) {
//                        printf("FAILED\n");
//                        slog("PING FAILED on %s: results >%s<\n",
//                           out_nm_or_ip,buf);
//                        continue;
//                    } printf("SUCCESS\n", out_nm_or_ip);
                    return_code =  NSLOOKUP_SUCCESS;
                    break;
                } else {
                    printf("ERROR.. Address: does not follow Name:\n");
                    return_code =  4;
                    break;
                }
            }
        } else printf("Unknown IN_TYPE = %d\n",in_type);
    }
    pclose(pipe);

    return return_code;
}


int nslookup(char *in_nm_or_ip, char *out_nm_or_ip)
{
    FILE *pipe;
    long got_line=0;
    char *sptr, buf[1000], ping_result[500], *p;
    int in_type, return_code = 3;

    sprintf(buf,"nslookup %s", in_nm_or_ip);
    slog("attempting popen(%s)\n", buf);
    if(!(pipe = popen(buf,"r"))) {
        return 1;
    }

    // is it a name, or an ip address?
    for (p = in_nm_or_ip; *p; ++p) if(!isdigit(*p) && *p != '.') break;
    in_type = *p ? NAME : IP;

    while (fgets(buf, 4999, pipe)) {
        //slog(" in nslookup.. got >%s<\n", buf);
        if(!got_line++) return_code = 2;
        if(in_type == IP) {
            if(found_in_etc_hosts(in_nm_or_ip, out_nm_or_ip, in_type)) {
#ifdef DEBUG
slog("Found match for >%s< being >%s< in /etc/hosts\n", in_nm_or_ip, out_nm_or_ip);
#endif
                return_code =   NSLOOKUP_SUCCESS;
                break;
            }
            strcpy(out_nm_or_ip,"IP NOT IN DNS");
            if(sptr=strstr(buf,"name =")) {
                sptr+=7;
                strcpy(out_nm_or_ip,sptr);
                if(sptr=strchr(out_nm_or_ip, '\n'))
                     *(--sptr) = '\0'; // get rid of '.' and \n at end of name
                return_code =  NSLOOKUP_SUCCESS;
                break;
            }
        } else if(in_type == NAME) {
            if(found_in_etc_hosts(in_nm_or_ip, out_nm_or_ip, in_type)) {
#ifdef DEBUG
slog("Found match for >%s< being >%s< in /etc/hosts\n", in_nm_or_ip, out_nm_or_ip);
#endif
                return_code =   NSLOOKUP_SUCCESS;
                break;
            }
            strcpy(out_nm_or_ip,"NAME NOT IN DNS");
            if(!strncmp(buf,"Name:", 5)) {
                fgets(buf, 4999, pipe);
                if(!strncmp(buf,"Address:", 8)) {
                    strcpy(out_nm_or_ip,&buf[9]);
                    REMOVE_NEWLINE(out_nm_or_ip);
                    printf("nslookup: found address >%s<. Testing ping...",
                             out_nm_or_ip);
                    if(!myping(out_nm_or_ip, ping_result)) {
                        printf("FAILED\n");
                        slog("PING FAILED on %s: results >%s<\n",
                           out_nm_or_ip,buf);
                        continue;
                    } printf("SUCCESS for %s\n", out_nm_or_ip);
                    return_code =  NSLOOKUP_SUCCESS;
                    break;
                } else {
                    printf("ERROR.. Address: does not follow Name:\n");
                    return_code =  4;
                    break;
                }
            }
        } else printf("Unknown IN_TYPE = %d\n",in_type);
    }
    pclose(pipe);

    return return_code;
}

int found_in_etc_hosts(char *in_nm_or_ip, char *out_nm_or_ip, int in_type)
{
    char *sptr, ip[200], name[200], alias[200], buf[1000];
    FILE *pipe;

    if(!(pipe = popen("cat /etc/hosts","r"))) {
        slog("can NOT open pipe >cat /etc/hosts<\n");
        return 1;
    }
    if(in_type == IP) {
        while (fgets(buf, 4999, pipe)) {
            sscanf(buf,"%s %s %s", ip, name, alias);
            REMOVE_NEWLINE(alias);
            if(!strcmp(in_nm_or_ip, ip)) {
               strcpy(out_nm_or_ip, name);
               pclose(pipe);
               return 1;
           }
        }
        pclose(pipe);
        return 0;
    }
    // else, in_type is NAME
//slog("found_in_etc: ready to get..\n");
    while (fgets(buf, 4999, pipe)) {
        sscanf(buf,"%s %s %s", ip, name, alias);
        REMOVE_NEWLINE(alias);
//slog("found_in_etc: check for match: >%s< >%s<\n", in_nm_or_ip, name);
        if(!strcmp(in_nm_or_ip, name)) {
           strcpy(out_nm_or_ip, ip);
           pclose(pipe);
           return 1;
        }
        if(!strcmp(in_nm_or_ip, alias)) {
           strcpy(out_nm_or_ip, ip);
           pclose(pipe);
           return 1;
        }
    }
    pclose(pipe);
    return 0;
}


char *upper_case(char *buf)
{
    char *ptr, *sptr, *buf1;
    
    buf1 = malloc(100);
    ptr = buf1;
    
    for(; *buf; ++buf, ++buf1) *buf1 = toupper(*buf);
    *buf1='\0';

    return (ptr);
}


char *lower_case(char *buf)
{
    int i, len;
    char tmpbuf[2001]; 
    char *ptr;

    //tmpbuf = (char *) malloc(2000);

    len=strlen(buf);

    for (i=0; i<len && buf[i]; i++) { tmpbuf[i] = tolower(buf[i]); }
    tmpbuf[i] = '\0';
    strcpy(buf,tmpbuf);
    ptr=buf;
    return (ptr);
}

char *rm_cntrl_chars(char *buf)
{
    char *ptr;
    for(ptr=buf; *ptr; ++ptr) if(iscntrl(*ptr)) *ptr = ' ';
    return buf;
}

char *rm_cntrl_chars_and_stuff(char *buf)
{
    char *ptr,  tmpbuf[5000], *tptr;
    tptr=tmpbuf;
    ptr=buf;

    for( ; *ptr; ++ptr, ++tptr) {

        if(iscntrl(*ptr) || *ptr == '\'' || *ptr == '"' || *ptr == '|' ||
              *ptr == '>' || *ptr == '<' || *ptr == '[' || *ptr == ']')
                  *ptr = '_';
       // else *tptr = *ptr;
    }
    //*tptr='\0';
//    strcpy(buf, tmpbuf);

    return buf;
}

void remove_trailing_spaces(char *buf)
{
    char *sptr;
    
    sptr=strchr(buf,'\0');
    while(*(--sptr) == ' ')  *(sptr)='\0';
}

void remove_spaces(char *buf)
{
    char *sptr, tmpbuf[50000];

    sptr=buf;
    while(*sptr == ' ') ++sptr;
    strcpy(tmpbuf,sptr);

    
    sptr=strchr(tmpbuf,'\0');
    while(*(--sptr) == ' ')  *(sptr)='\0';
    strcpy(buf,tmpbuf);
}

char * clean_str(char *buf)
{
    char *ptr, *retbuf, *rptr;

    if(!(retbuf=malloc(100))) { exit(2); }

    for(ptr=buf, rptr=retbuf; *ptr; ++ptr) {
          if(iscntrl(*ptr) || *ptr == '\'' || *ptr == '"' || *ptr == '|' ||
               *ptr == '>' || *ptr == '<'  || *ptr == '[' || *ptr == ']')
                  continue;
          //else if(isupper(*ptr)) *(rptr++)=tolower(*ptr);
          else *(rptr++)=*ptr;
    }
    *rptr='\0';
    remove_trailing_spaces(retbuf);
    return retbuf;
}


char *rm_cntrl_chars_quotes_and_apos(char *buf)
{
    char *ptr;
    for(ptr=buf; *ptr; ++ptr) 
        if(iscntrl(*ptr) || *ptr == '\'' || *ptr == '"') 
            *ptr = ' ';
    return buf;
}

int days_in_month(int imon) 
{
    int days_in_month[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};
    return (days_in_month[imon]);
}

char *current_date(char *buf, int days_to_subtract)
{
    int days_in_month[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};
    int iyear, imon, iday;
    struct tm *tptr;
    time_t tm;
    
    tm = time(NULL);
    tptr = localtime(&tm);
    
    if(days_to_subtract > 29) {
        sprintf(buf,"%4.4d%2.2d%2.2d", 
           tptr->tm_year, tptr->tm_mon, tptr->tm_mday);
        return buf;
    }

    iyear = tptr->tm_year+1900; 
    imon = tptr->tm_mon+1; 

    if((iday = tptr->tm_mday - days_to_subtract) < 1) {
        if(imon==1) { --iyear; imon=12; iday += days_in_month[imon]; } 
        else { --imon; iday += days_in_month[imon]; }
    }
    
    sprintf(buf,"%4.4d%2.2d%2.2d", iyear, imon, iday);
    return buf;
}

char *current_date_time(char *buf)
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    tptr = localtime(&tm);
    sprintf(buf,"%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", tptr->tm_year+1900, tptr->tm_mon+1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return buf;
}

char *current_date_time_eventlog(char *buf)
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    tptr = localtime(&tm);
    sprintf(buf,"%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d.000000-360", tptr->tm_year+1900, tptr->tm_mon+1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return buf;
}

char *date_stamp1(char *buf)
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    tptr = localtime(&tm);
    sprintf(buf,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", tptr->tm_mon+1, tptr->tm_mday, tptr->tm_year-100, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return buf;
}

char *date_stamp(char *buf)
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    tptr = localtime(&tm);
    sprintf(buf,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d", tptr->tm_mon+1, tptr->tm_mday,  tptr->tm_year+1900, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return buf;
}

int epoch()
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    return (int)tm;
}

int to_num(char *messy_num)
{
    char num_buf[30], *new_num;

    new_num = num_buf;
    while(*messy_num != '\0') {
        if(isdigit(*messy_num)) *(new_num++) = *(messy_num++);
        else ++messy_num;
    }
    *new_num = '\0';
    return atoi(num_buf);
}

int current_time_greater_or_equal_to_last_date_processed(char *sEventTime, char *last_date_str)
{
    /*2009-09-23 18:08:34*/
    char cur_date[30], last_date[30], cur_time[30], last_time[30];

    strncpy(cur_date, sEventTime, 10); cur_date[10] = '\0';
    strncpy(last_date, last_date_str, 10); last_date[10] = '\0';
    if(to_num(cur_date) > to_num(last_date)) return 1;
    if(to_num(cur_date) < to_num(last_date)) return 0;
    strncpy(cur_time, &sEventTime[11], 8); cur_time[8] = '\0';
    strncpy(last_time, &last_date_str[11], 8); last_time[8] = '\0';
    if(to_num(cur_time) >= to_num(last_time)) return 1;
    return 0;
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
 
char *ts_ddhhmm()
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
    sprintf(buf,"%02d%02d%02d", tptr->tm_mday, tptr->tm_hour, tptr->tm_min);
    return (buf);
}   
 
char *ts_mmddhhmmss()
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
    sprintf(buf,"%02d%02d%02d%02d%02d", tptr->tm_mon+1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return (buf);
}   
 

char *ts_yymmddhhmm()
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
    sprintf(buf,"%02d%02d%02d%02d%02d", tptr->tm_year-100, tptr->tm_mon+1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min);
    return (buf);
}


int  get_name_pass(char *file, char *name, char *pass)
{
  static char *key="^#*$@", *sptr, string[200];
  int i=0, slen=0,klen=0,x=0;
  FILE *fp;

  klen = strlen(key);

  REMOVE_NEWLINE(file);

  if(!(fp = fopen(file, "r"))) {
        slog("get_name_pass: FAILED: can not open >%s< to read\n", file );
        exit(0);
  } 

  fgets(string,199,fp);
  slen = strlen(string)-1;

  for(x = 0, i = x % klen; x < slen; x++, i = x % klen) 
     string[x]=string[x]^key[i];

  sscanf(string,"%s %s", name, pass);
  return 1;
}

#define MAX_CHARS_IN_SERVICE_NAME 60
char * norm(char *buf)
{
    char  *rbuf, *rbp, *ibp;
    int modified=0,i=0;

    if(!(rbuf=malloc(200))) { slog("ABORTING: fail MALLOC 200\n"); exit(2); }
    rbp=rbuf; ibp=buf;

    for(; *buf && i < MAX_CHARS_IN_SERVICE_NAME; buf++, rbuf++)
    {   /* replace any wierd char OR space with an underscore */
        if(strchr("`#~!$%^&*\"|'<>@#?,()=", *buf) || *buf <= 32) {
            *rbuf='_';
            modified=1;
        }
        else *rbuf = *buf;
    }

    *rbuf = '\0';
    if(modified) slog("normalize_value: Modified >%s< to >%s<\n", ibp, rbp);
    return rbp;
}
int files_are_different(char *file1, char *file2)
{
    FILE *fpp;
    char   buf[5000], field[1000], p21[10], p22[10], p11[10], p12[10];
    char p1_txt[100], p2_txt[100];

    sprintf(p1_txt,"sum %s", file1);
    sprintf(p2_txt,"sum %s", file2);
    if(!(fpp = popen(p1_txt, "r"))) {
        //slog("Can NOT popen >%s<\n", p1_txt);
        return -1;
    } fgets(buf,100,fpp); sscanf(buf,"%s %s %s", p21, p22, field);
    pclose(fpp);

    if(!(fpp = popen(p2_txt, "r"))) {
        //slog("Can NOT popen >%s<\n", p2_txt);
        return -1;
    } fgets(buf,100,fpp); sscanf(buf,"%s %s %s", p11, p12, field);
    pclose(fpp);

    if(!strcmp(p11,p21) && !strcmp(p12,p22)) {
         slog("FILES are the same: >%s< >%s< \n",file1,file2);
         return 0;
    }
    slog("FILES are different: >%s< >%s< \n",file1,file2);
    //system(" cp /home/u0363077/demnINFO.xml /home/u0363077/bk/demnINFO.xml.`date '+%m%d%y-%H:%M:%S'`");
    return 1;
}

char *current_date_time1(char *buf)
{
    struct tm *tptr;
    time_t tm;

    tm = time(NULL);
    tptr = localtime(&tm);
    sprintf(buf,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", tptr->tm_mon+1, tptr->tm_mday, tptr->tm_year-100, tptr->tm_hour, tptr->tm_min, tptr->tm_sec);
    return buf;
}


int lower_case_and_warn(char *buf, char *warning)
{
    int i, len, done=0;
    char tmpbuf[2001];
    char *ptr;

    //tmpbuf = (char *) malloc(2000);

    len=strlen(buf);

    for (i=0; i<len && buf[i]; i++) {
        if(buf[i] > 64 && buf[i] < 91) {  // wez gots us a cappy
            if(!done) {
              printf("%s",warning);
              printf("buf >%s< char >%c<\n", buf, buf[i]);
              done = 1;
            }
            tmpbuf[i] = tolower(buf[i]);
        } else tmpbuf[i] = buf[i];
    }
    tmpbuf[i] = '\0';
    strcpy(buf,tmpbuf);
    return done;
}

int myping(char *ip, char *result_text)
{
    FILE *pp;
    char *sptr, *tptr, host[200], cmd[150], buf[200];
    char rtab[20], pct_lossb[20], *rta, *pct_loss;

    rta=rtab; pct_loss=pct_lossb;

    //sprintf(cmd,"%s/check_icmp %s 72.20.132.78 -w800,80% -c1000,100%", BIN_PATH, ip);

    //if(hospital_node(ip))
        sprintf(cmd,"ping -w1 %s", ip);
    //else
    //    sprintf(cmd,"ssh nagadm@nagapp2.it.utah.edu ping -w1 %s", ip);
#ifdef DEBUG1
    slog("trying >%s<\n", cmd);
#endif
    if(!(pp=popen(cmd,"r"))) {
        sprintf(result_text, "cant open pipe >%s<\n",cmd);
        slog("ping(): cant open pipe  >%s<\n",cmd);
        return PING_PIPE_FAILED;
    }
    while(fgets(buf, 200, pp)) {
#ifdef DEBUG1
    slog("myping: got >%s<\n", buf);
#endif
        if(strstr(buf,"packet loss,")) {
            if(strstr(buf,"100% packet loss")) {
                slog("PING FAILED >%s<\n", buf);
                return 0;
            }
            slog("PING SUCCESS >%s<\n", buf);
            return 1;
        }
    }
    slog("PING FAILED: Never saw 'packet loss,' in >%s<\n", buf);
    pclose(pp);
    return 0;
}

rm_path(char *filepath, char *file)
{
    char *sptr, *last;

    sptr=filepath;
    if(sptr=strchr(filepath, '/')) {
        while(sptr=strchr(sptr,'/')) last = ++sptr;
        strcpy(file, last);
    } else  strcpy(file, filepath);
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

#include <regex.h>

int string_contains_pat(char *str, char *pat, int *start, int *end)
{
regex_t regex;
int status;
regmatch_t pmatch[1];
int reti;
char msgbuf[100];

/* Compile regular expression */
    reti = regcomp(&regex, pat, 0);
    if( reti ){ fprintf(stderr, "Could not compile regex\n"); exit(1); }

/* Execute regular expression */
    reti = regexec(&regex, str, 1, pmatch, 0);
    if( !reti ){
        *start = pmatch[0].rm_so ; *end = pmatch[0].rm_eo;
        return 1;
    }
    else if( reti == REG_NOMATCH ){
        return 0;
    }
    else{
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        return 0;
    }
}

int string_contains_pat_looking_at_substrings_of_the_structure(char *str, char *pat)
{
regex_t regex;
int status;
regmatch_t pmatch[1];
int reti;
char msgbuf[1000];
char substr[200][1000];
int i=0, cnt=0;
/* Compile regular expression */
    reti = regcomp(&regex, pat, 0);
    if( reti ){ fprintf(stderr, "Could not compile regex\n"); exit(1); }

/* Execute regular expression */
cnt = get_substr_ary(str, substr);
for(i=0; i<cnt; i++) {
    reti = regexec(&regex, substr[i], 1, pmatch, 0);
    if( !reti ){
        //*start = pmatch[0].rm_so ; *end = pmatch[0].rm_eo;
        return 1;
    }
    else if( reti == REG_NOMATCH ){
        continue;
        //return 0;
    }
    else{
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        return 0;
    }
}
return 0;
}

int get_substr_ary(char *str, char substr[][1000])
{
    int idx=0, num_flds=0;

    while(*str) {
        substr[num_flds][idx] = *(str);
        if(*str == '\n')  { 
            substr[num_flds][idx] = '\0';  idx=0;
            //printf("substr[%d] = %s\n", num_flds, substr[num_flds] );
            num_flds++; 
        } else idx++;

        ++str;
    }
    substr[num_flds][idx] = '\0';

    return num_flds;
}


int replace_pat(char *string, char *replace_str, int start, int end)
{
    char buf[5000];
    strncpy(buf, string, start); buf[start] = '\0';
    strcat(buf, replace_str);
    strcat(buf, &string[end]);
    strcpy(string, buf);
    return 1;
}

int add_line(char *struc, char *data_type, char *data)
{
    char buf[5000], catbuf[5000], *sptr;
    int size, start,end;

    strcpy(buf, struc); 
    sptr = strchr(buf,'}');
    *sptr = '\0';
    if(strlen(data_type) < 7)
         sprintf(catbuf,"\t%s\t\t\t\t%s\n}", data_type, data);
    else if(strlen(data_type) > 23)
         sprintf(catbuf,"\t%s\t%s\n}", data_type, data);
    else if(strlen(data_type) > 15)
         sprintf(catbuf,"\t%s\t\t%s\n}", data_type, data);
    else sprintf(catbuf,"\t%s\t\t\t%s\n}", data_type, data);
    strcat(buf,catbuf);
    strcpy(struc,buf);
    return 1;
}

