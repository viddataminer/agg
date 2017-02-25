#include<syslog.h>
#include <stdlib.h>
#define RESUB_BUF "/smg/tmp/resub_buf"
#define ALERT_LOG "/smg/log/alert.log"
#define NAGIOS_PIPE "/usr/local/nagios/var/rw/nagios.cmd"

//extern char *g_tag, *g_service, *g_ip;

//int replace_cntrl_newlines_with_escape_newlines(cmd);
char *rm_cntrl_chars(char *);
char *rm_cntrl_chars_and_stuff(char *buf);
char *replace_backslash_for_forwardslash(char *buf) { 
    char *ptr=buf; for(;*ptr;ptr++) if(*ptr == '\\') *ptr='/';  return buf; 
}


int send_nsca(char *node, char *service, int severity, char *msg_text, char *agg_ip)
{
    char cmd[5000], *sptr, buf[5000], logbuf[1000], hostname[200];
    char the_real_agg[200], the_real_node[200], syslog_msg[10000];
    char sync_cmd[1000], tcmd[1000], log_info[1000];
    int do_system_call=0;
    time_t long_time;
    FILE *pp, *fp, *pptr;

#ifdef SYNC_TESTING
    sprintf(sync_cmd,"echo \"%ld %s %s %d\" >> %s", long_time, node, service, severity, "/smg/out/state_sync.lst");
#endif

    // SHG 01-17-17 no longer report appinfo against localhost, but report
    // against the host running the current process.
    if(!strcmp(node,"localhost")) { 
        pptr=popen("hostname -A","r");
        fgets(buf,100,pptr);
        strcpy(node, buf);
        REMOVE_NEWLINE(node);
        pclose(pptr);
    }

//slog("aggip is >%s<\n", agg_ip);

//    if(strstr(service,"memory")) replace_cntrl_newlines_with_escape_newlines(msg_text);

if(!strcmp(agg_ip,"localhost")) {
    time(&long_time); 
    //sprintf(cmd,"echo '[%ld] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s'  > %s", long_time, node, service, severity, msg_text, "/usr/local/nagios/var/rw/nagios.cmd");
    sprintf(tcmd,"[%ld] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s", long_time, node, service, severity, msg_text);
    if(!(fp=fopen(NAGIOS_PIPE, "w"))) {
       sprintf(cmd,"echo %s >> %s", tcmd, RESUB_BUF);
       slog("ERROR nagios.cmd: Could NOT open pipe to write!! Sending to /smg/tmp/resub_buf.\n");
       do_system_call=1;
       sprintf(log_info,"echo \"`date '+%%m-%%y %%H:%%M:%%S'` ERROR CMD PIPE: NO OPEN %s: %s.\" >> %s", RESUB_BUF, tcmd, ALERT_LOG);
       system(log_info); 
    } else {
        fprintf(fp,"%s\n", tcmd); 
        fclose(fp);
        slog("send_nsca EVENT ALERT SENT >%s<\n", tcmd);
    }
} else if(!strcmp(agg_ip,"send_nsca")) {
    strcpy(the_real_agg, agg_ip);
    slog("APPLIANCE - Using send_nsca: agg is %s, node is %s\n", agg_ip, node);
    sprintf(cmd,"echo '%s;%s;%d;%s' | /usr/sbin/send_nsca -H %s -d ';' -c /etc/send_nsca.cfg", node, service, severity, rm_cntrl_chars_and_stuff(msg_text), the_real_agg);
    do_system_call=1;
} else if(strstr(agg_ip,"resub_buf")) {
    time(&long_time); 
    //sprintf(cmd,"echo '[%ld] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s'  >> %s", long_time, node, service, severity,msg_text, "/smg/out/process_resub_buf.txt");
    // this will never happen, as node should never be passed in a 'localhost'
    if(strstr(node,"resub_buf")) strcpy(node,"localhost");

    sprintf(cmd,"echo '[%ld] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s'  >> %s", long_time, node, service, severity,msg_text, RESUB_BUF);
    slog("send_nsca: doing >%s<\n", cmd);

//    if(strstr(service,"memory")) 
//        replace_cntrl_newlines_with_escape_newlines(cmd);
    do_system_call=1;
} else slog("AGGIP ERROR: No match for aggip >%s<\n", agg_ip);

#ifdef DEBUG
    slog("DEBUG: NOT system(%s)\n", cmd);
#else
    if(do_system_call) {
        slog("send_nsca EVENT ALERT: system(%s)\n", cmd);
        system(cmd);
        slog("back from system\n");
    }
#endif
#ifdef sync_testing
    system(sync_cmd);
#endif
    slog("send_nsca complete\n");
//#ifdef NEEDED
// DO SYSLOG STUFF
    //openlog("Logs", LOG_CONS, LOG_USER);
#ifndef DONT_SEND_SMG_EVENT_TO_SYSLOG
    //slog("Compiled with NO_SMG_EVENT defined - not sending smgEvent\n");
#else 
    openlog("cmd_prsr", LOG_CONS, LOG_USER);

    if(*g_tag) sprintf(syslog_msg,"smgEvent|%ld|%s|%s|%d|%s|%s|%s", 
      long_time, node, g_ip, severity, g_service, g_tag, msg_text);
    else sprintf(syslog_msg,"echo \"smgEvent|%ld|%s|%s|%d|%s|%s|%s", 
      long_time, node, g_ip, severity, g_service, g_service, msg_text);

    if(severity==2) syslog(LOG_CRIT, syslog_msg);
    else if(severity==1) syslog(LOG_WARNING, syslog_msg);
    else syslog(LOG_INFO, syslog_msg);

    closelog();
#endif
//#endif
}


int send_nsca_host_check(char *node, int severity, char *msg_text, char *agg_ip)
{
    char cmd[5000], *sptr, buf[5000], logbuf[1000];
    FILE *pp;
    time_t long_time_no_c;
    time(&long_time_no_c); 

slog("in send_nsca_host_check: aggip is >%s<\n", agg_ip);

if(!strcmp(agg_ip,"send_nsca")) {
    sprintf(logbuf,"SEND NSCA: >%s<",cmd);
    sprintf(cmd,"echo \"%s;%d;'%s'\" | %s/send_nsca -H %s -d ';' -c %s/send_nsca.cfg", node, severity, rm_cntrl_chars_and_stuff(msg_text), BIN_PATH, agg_ip, CFG_PATH);

} else if(!strcmp(agg_ip,"localhost")) {
    sprintf(cmd,"echo \"[%ld] PROCESS_HOST_CHECK_RESULT;%s;%d;%s\" > %s",
      long_time_no_c, node, severity, msg_text, "/usr/local/nagios/var/rw/nagios.cmd");
    sprintf(logbuf,"SEND PIPE: >%s<",cmd);
} else if(strstr(agg_ip,"resub_buf")) {
  sprintf(cmd,"echo \"[%ld] PROCESS_HOST_CHECK_RESULT;%s;%d;%s\" >> %s",
    long_time_no_c, node, severity, msg_text, RESUB_BUF);
}


#ifdef DEBUG
    slog("DEBUG ON: NOT SENDING NSCA >%s<\n",cmd);
    slog("in send_nsca_host_check: node >%s< sev >%d< msg>%s< aggip >%s<\n",    
                node, severity, msg_text, agg_ip);
#else
    slog("%s\n",logbuf);
    system(cmd);
#endif

    slog("send_nsca complete\n");
}

