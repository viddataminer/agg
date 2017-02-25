//int try_snmp_system(char *node_ip, char *os_type, char *hostname,char *commstr);
//int try_wmic(char *node, char *domain, char *hostname);
//int try_ssh(char *node_ip, char *os_type, char *hostname);
//int try_nmap(char *node_ip, char *os_type);
int remove_prens(char *dirty, char *clean);
#define PING_FAILED 2
#define NO_OS_TYPE 3
#define FOUND_OS_TYPE 4
int g_no_nmap=1;
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
int g_use_command_line_comm_strs = 0;
char *g_comm_strs[10]={"1xR\\$bluE", "99U#u#U\\!x", "oc12fast", "public", ""};

#define OSTYPE_NOT_RECOGNIZED (!strcmp(os_type,"sunos") ||  strcmp(os_type,"aix")) ||  strcmp(os_type,"linux")) 

int main(int argc, char **argv)
{   
    char  ver[10]="v2.0";
    char junk[2], *sptr, file1[100], file[100], cfg_file_with_path[200];
    char dotfile[200], dummy[100], buf[1000], node_name[100];
    int num_ips=0, done_base=0, result, num_connected=0, cur=0;
    int ret, n1_idx, n2_idx, j, i, no_access=0, last_idx=0, ser_found=0;
    char node[200], domain[100], n1[500], n2[500], node_ip[500]; 
    char cmd[200],ip_to_check[100], subnet[100], flag[2];
    char output_file_subnet[200];
        char an_ip[50], os_type[100];
    int num_ips_on_subnet=0, acct, snmp;
    float f1, f2;
    FILE *fp_cfg, *pptr, *fpw_subnet;

    if(argc != 2) { 
       printf("USAGE: box_info node\n"); exit(0);
    }
    
    open_log_file(argv[0]);

    slog("----------------STARTING-------------------%s----\n",ver);
    strcpy(domain,"domain");
    slog("argv1 is (%s)\n", argv[1]);
    //strcpy(current_node, argv[1]);
    if(!its_an_ip(argv[1])) {
         nslookup(argv[1], an_ip);
         slog("nslookup returns ip >%s< for name >%s<\n", an_ip, argv[1]);
    } else {
       strcpy(an_ip, argv[1]);
    }
    if(argc == 3) strcpy(domain, argv[2]);    
    else memset(domain,0,sizeof(domain));

    ret = get_box_info(an_ip, os_type, domain, &acct, &snmp);
    if(!(ret == PING_FAILED))
        printf("os_type >%s<, domain >%s<, acct = %d, snmp = %d\n",
          os_type, domain, acct, snmp);
    slog("----------------completed-------------------%s---\n",ver);
}
#else 
    extern char g_comm_strs[10][100];
    extern int g_num_comms, g_num_domains, g_use_command_line_comm_strs;
#endif

int get_box_info(char *node_ip, char *r_os_type, char *r_domain, 
     int *acct, int *snmp)
{
    FILE *cfg_ptr, *fp, *fpt;
 
    char *sptr, nd[1000], buf[4000], process[1000];
    char event_txt[1000],  *hp, host[1000], *pat, state[200];
    char *ppt, pport[200], curstate[200], service[200];
    char drive,  pat_array[10][200]={0}, name[200], pass[200];
    char from_port[200], to_port[200], from_ip[200], to_ip[200];
    char to_name[200], from_name[200], tmpbuf[1000]; 
    char *port, pt[100], cmd[1000], domain[100], dom[100];
    char cmd2[1000], acct_file[200], nd_ip[20], 
	    fq_to_name[500], fq_from_name[500], dummy[100];
    char logbuf[2000], file[200], node_name[500];//, num_nodes=0;
    char node_pair[200], node_pair_rev[200], prev_to_name[500];
    char contact[200], location[200], tmpcmd[1000];
    int first_run=0, l_ips_got=0, got_in=0, use_snmp=0;
    int finished=0, num_logs=0, found_tail=0, num_pats=0, found=0, 
	cur_state_ok=0, to_ret_val=0, from_ret_val=0;
    char msg_txt[5000], to_service_name[500], to_service_text[1000];
    char from_service_name[500], from_service_text[1000], os_type[500];
    int snmp_available=0, ssh_available=0, wmic_available=0;
    char os_details[200], mac_address[200]; 
    char unique_connections[1000][200], host_name[100];
    char dns_name[200], snmp_os_type[100];
    char  nmap_os_type[100], comm_str[100];
    int num_unique_connections=0, pingable = 0, is_windows=0, num_ser_found=0;
    int nmap_available=0;

   // if(sent_appinfo_event_due_to_failed_ping(nd, nd_ip, "sd", ""))
   //        continue;
    ///nslookup(node_ip, node_name);
    //sprintf(cmd,"/smg/bin/sshpass -p %s ssh %s@%s netstat -a", pass, name, node);
    memset(host_name, 0, sizeof(host_name));
    memset(dns_name, 0, sizeof(dns_name));
    memset(os_type, 0, sizeof(os_type));
    memset(domain, 0, sizeof(domain));
    pingable=wmic_available=ssh_available=snmp_available=num_ser_found=0;

    if(!myping(node_ip, buf)) {
       printf("Can NOT ping %s\n", node_ip);
       slog("PING FAILED: results >%s<\n", buf);
       pingable=0;
       return PING_FAILED;
    } else pingable=1;
    printf("Acquiring information for %s.....\n", node_ip);

    nslookup(node_ip, node_name);
    //nmap(node_ip, os_type);
    //printf("checking %s %s\n", buf, node_ip);
     //5038076 

    if(!try_nmap(node_ip,os_type)) {
        slog("nmap FAILED to return os for node ip >%s<\n", node_ip);
        nmap_available=0;
        if(!try_snmp_system(node_ip,os_type,host_name, comm_str)){
            slog("SNMP FAILED for node ip >%s<\n", node_ip);
            snmp_available=0;
            slog("Neither snmp NOR nmap is returning os\n");
            slog("doing brut force wmic/ssh\n");
        }
    } else nmap_available=1;

    if(*os_type) {
        slog("Found ostype of >%s<\n", os_type);
        if(strstr(os_type, "indows")) {
            if(try_wmic(node_ip, domain, host_name)) {
                slog("windows domain is >%s<  for host >%s(%s)< \n", domain, node_ip, host_name);
                wmic_available=1;
            }
        } else if(strstr(os_type, "Foundry Networks") || 
            strstr(os_type, "isco")) {
            slog("OS type is >%s<\n", os_type);
            if(!try_snmp_system(node_ip,os_type,host_name, comm_str)) {
                slog("snmp failed for %s\n", node_ip);
            }
        } else if(strstr(os_type, "inux") || strstr(os_type, "aix") || 
                  strstr(os_type,"sun")) {
            slog("Looks like linux/unix/sun/aix \n");
            if(try_ssh(node_ip, os_type, host_name)) ssh_available = 1;
            else slog("ssh failed...\n");
        }
    } else {
        slog("Cant get ostype from nmap NOR snmp.  Trying brut force..\n");
        if(try_wmic(node_ip, domain,  host_name)) {
            wmic_available = 1;
            strcpy(os_type, "windows");
        } else if(try_ssh(node_ip, os_type, host_name)) ssh_available = 1;
    }

    /* check for access */



    REMOVE_NEWLINE(host_name);
    nslookup(node_ip, dns_name);
    REMOVE_NEWLINE(os_type);
    
#ifndef TESTMAIN
    if(!*os_type) { slog("os_type is null. returning\n"); return NO_OS_TYPE; }
        //strcpy(os_type, rm_cntrl_chars_quotes_and_apos(snmp_os_type));
#else
    if(!*os_type) strcpy(os_type,"unknown");
    printf("%s|%s|%s|%s|%d|%d|%d|%d|%s|%s",  node_ip, host_name, dns_name, 
        os_type, pingable, snmp_available, ssh_available, 
        wmic_available, comm_str, domain);
#endif
    strcpy(r_os_type, os_type);
    strcpy(r_domain, domain);
    *snmp=snmp_available; 
    *acct = (ssh_available || wmic_available);
    return FOUND_OS_TYPE;  
}


int get_state(char *logbuf, char *curstate)
{
char tp[100], *type, *ptr;
        type = tp;

ptr=logbuf;
ptr=strchr(ptr, ':');
++ptr;
ptr=strchr(ptr, ':');
++ptr;

        while(isalnum(*ptr)) ++ptr;
while(isspace(*ptr)) ++ptr;
while(isalnum(*ptr))  *(type++) = *(ptr++);
        *type = '\0';
strcpy(curstate, tp);
return 1;
}

int send_event_complete(char *node)
{
    char cmd[5000], *sptr, buf[5000];
    int i=0, wseverity;
    FILE *pp;


    sprintf(cmd,"echo %s;logck_%s;%d;\"%s\" | /smg/bin/send_nsca", "smgappliance1", node, 0, "Process Completed Successfully");
    slog("SENT NSCA: >%s<\n",cmd);
    if(!(pp=popen(cmd,"r"))) { 
        slog("cant open >%s<",cmd); 
        return 1; 
    }
    while(fgets(buf,100,pp)) slog(buf);
    pclose(pp);
}

char *get_port(char *logbuf, char *port)
{
    char pport[200], *ppt, *sptr;

    if(!(sptr=strchr(logbuf,':')) || logbuf[0] == '#') return "";
    ppt = pport; 
    ++sptr;
    while(!isspace(*sptr)) *(ppt++) = *(sptr++);
    *ppt='\0';
    strcpy(port, pport);
    return port;
}

#define ColSV "%[^:]%*[:]  %[^:]%*[:]"
#define PSV6 "%[^.]%*[.]  %[^.]%*[.]  %[^.]%*[.]  %[^.]%*[.]  %[^.]%*[.]  %[^.]%*[.]"
#define PSV2 "%[^.]%*[.]  %[^.]%*[.]"


int try_ssh(char *node_ip, char *os_type, char *hostname)
{
    FILE *fpr;
    char *sptr, buf[1001], cmd[1000], name[200], pass[200];
    int num=0;

    slog("in try_ssh...\n");

    REMOVE_NEWLINE(node_ip);

    
    if(*g_acct_file) {
        get_name_pass(g_acct_file, name, pass);
        slog("ACCT info file is >%s<, name >%s<, pass >%s<\n", g_acct_file, name, pass);
    } else {
        slog("NO g_acct_file >%s<\n", g_acct_file);
        get_name_pass("/smg/cfg/nagadm_acct_info.dat", name, pass);
    }
    sprintf(cmd,"/smg/bin/sshlogin1.exp %s %s %s uname -a 2>&1", pass, node_ip, name);
/*
    //sprintf(cmd,"/smg/bin/sshpass -p N@g105yz ssh nagadm@%s hostname",
   //        node_ip);
    //sprintf(cmd,"/smg/bin/sshpass -p N@g105yz ssh nagadm@%s uname", node_ip);
    sprintf(cmd,"/smg/bin/sshlogin1.exp N@g105yz %s nagadm uname -a 2>&1", node_ip);
    //sprintf(cmd,"2>&1 /smg/bin/sshpass -p %s ssh %s@%s uname", "N@g105yz", "nagadm", node);

/*
    sprintf(cmd2,"%s > /tmp/get_os.out", cmd);
    system(cmd2);
    if(!(fpr = popen("/tmp/get_os.out","r")))  
*/
    slog("popen >%s<\n", cmd);
    if(!(fpr = popen(cmd,"r"))) { 
        slog("Cant open >%s<\n", cmd);
        return 0;
    }

    while(fgets(buf, 1000, fpr)) {
        REMOVE_NEWLINE(buf);
        if(strstr(buf,"spawn") || 
           strstr(buf,"assword") || 
           strstr(buf,"set locale") ||
           strstr(buf,"key fingerprin") ||
           strstr(buf, "The authenticity")) { 
           slog("fgets returned >%s< .. getting another line\n");
           continue;
        } 
        if(strstr(buf,"Connection refused") ||
             strstr(buf,"Permission denied") ||
             strstr(buf,"can't be established") )
        {
            slog("fgets returned >%s< .. returning to calling function\n");
            fclose(fpr);
            return 0;
        }
        if(buf[0] == '\n' || buf[0] == '\0') continue;
        
        num=sscanf(buf,"%s %s %s", os_type, hostname, cmd);

        if(num < 3) continue;

        lower_case(os_type); 
        if(( !strstr(os_type,"sunos") &&  
             !strstr(os_type,"aix")) &&  
             !strstr(os_type,"linux")) 
        { 
            slog("No data in >%s<\n", os_type);  
            continue;
        }
        fclose(fpr);
        slog("try_ssh successful.>%s<..\n", buf);
        return 1;
    }
    fclose(fpr);
    slog("leaveing try_ssy: never found os\n");
    return 0;
}

#define NUM_DOMAINS_TO_TRY 3

int try_wmic(char *node, char *domain, char *hostname)
{
    FILE *fpr;
    char cmd[1000], buf[1000], *serial_ptr;
    char header[1000], data[1000];
    char domains_to_try[NUM_DOMAINS_TO_TRY][100]={"srvr", "ad", 0};
    time_t time_then, time_now;
    int i=0;

    slog("incomming try_wmic(node >%s<, domsin>%s<, hostname>%s<)\n", node, domain, hostname);


    if(!*domain) {
        for(i=0; i < NUM_DOMAINS_TO_TRY; i++) {
            time(&time_then);
            printf("Trying domain >%s<\n", domain);
            if(find_windows_domain(node, domains_to_try[i], hostname)) break;
            time(&time_now);
            if(time_now > time_then + 30) {
                slog("Not trying other domains against %s - tooo long a wait\n", 
                         node);
                return 0;
            }
        }
    } else { i = 0; strcpy(domains_to_try[i], domain); }

    if(NUM_DOMAINS_TO_TRY>i) {
        strcpy(domain, domains_to_try[i]);
        if(get_windows_hostname(node, domains_to_try[i], hostname))  {
            slog("FOUND HOSTNAME for %s: %s, domain: %s\n", node, hostname, domain);
            return 1;
        }
    }
    slog("NOT FOUND HOSTNAME for %s: %s, domain: %s\n", node, hostname, domain);
    return 0;
}


int get_windows_hostname(char *node,  char *domain, char *hostname)
{
    char *sptr, cmd[1000], buf[1000], header[1000], data[1000];
    char name[100], pass[100];
    FILE *fpr;

    
    if(*g_acct_file) {
        get_name_pass(g_acct_file, name, pass);
        slog("ACCT info file is >%s<, name >%s<, pass >%s<\n", g_acct_file, name, pass);
    } else {
        slog("NO g_acct_file >%s<\n", g_acct_file);
        get_name_pass("/smg/cfg/acct_info.dat", name, pass);
    }
/*
    if(*domain) sprintf(cmd,"/smg/bin/wmic //%s -U %s/nagiosadmin%%nagios123 \" select name from Win32_ComputerSystem\" 2>/dev/null", node, domain);
     else sprintf(cmd,"/smg/bin/wmic //%s -U nagiosadmin%%nagios123 \" select name from Win32_ComputerSystem\" 2>/dev/null", node);
*/

    if(*domain) sprintf(cmd,"/smg/bin/wmic //%s -U %s/%s%%%s \" select name from Win32_ComputerSystem\" 2>/dev/null", node, domain, name, pass);
     else sprintf(cmd,"/smg/bin/wmic //%s -U %s%%%s \" select name from Win32_ComputerSystem\" 2>/dev/null", node, name, pass);

    slog("trying os cmd: >%s<\n", cmd);


    if(!(fpr=popen(cmd,"r") )) return 0;

    while(fgets(buf, 1000, fpr)) {
#ifdef DEBUG1
       slog("GOT >%s<\n", buf);
#endif
       if(strstr(buf, "ERROR: ")) { pclose(fpr); return 0; }
       if(strstr(buf,"CLASS")) { // should get CLASS: back on success
            slog("os type is WINDOWS with domain >%s<\n", domain);
            fgets(header, 1000, fpr);
            fgets(data, 1000, fpr);
            strcpy(hostname, data);
            REMOVE_NEWLINE(hostname);
            return 1;
       }
    }
    return 0;
}

int find_windows_domain(char *node, char *try_this_domain, char *hostname)
{
    char cmd[1000], buf[1000], header[1000], data[1000];
    char name[200], pass[200];
    FILE *fpr;

    if(*g_acct_file) get_name_pass(g_acct_file, name, pass);
    else get_name_pass("/smg/cfg/acct_info.dat", name, pass);

    if(*try_this_domain) 
        sprintf(cmd,"/smg/bin/wmic //%s -U %s/%s%%%s \" select SerialNumber from Win32_BIOS\"", node, try_this_domain, name, pass);
    else sprintf(cmd,"/smg/bin/wmic //%s -U %s%%%s \" select SerialNumber from Win32_BIOS\"", node, name, pass);
    slog("popen(%s)\n", cmd);
    if(!(fpr=popen(cmd,"r") )) return 0;

    while(fgets(buf, 1000, fpr)) {
#ifdef DEBUG1
       slog("GOT >%s<\n", buf);
#endif
       if(strstr(buf, "ERROR: ")) { pclose(fpr); return 0; }
       if(strstr(buf,"CLASS")) { // should get CLASS: back on success
            slog("os type is WINDOWS with domain >%s<\n", try_this_domain);
            return 1;
       }
    }
    return 0;
}


#ifdef wont_compile
SNMPv2-MIB::sysDescr.0 = STRING: "Hardware: x86 Family 15 Model 33 Stepping 2 AT/AT COMPATIBLE - Software: Windows Version 5.2 (Build 3790 Uniprocessor Free)"
SNMPv2-MIB::sysObjectID.0 = OID: SNMPv2-SMI::enterprises.311.1.1.3.1.2
DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (1132457866) 131 days, 1:42:58.66
SNMPv2-MIB::sysContact.0 = STRING: "ESM Team"
SNMPv2-MIB::sysName.0 = STRING: "SMGAPPLIANCE1"
SNMPv2-MIB::sysLocation.0 = STRING: "585 Komas Data Center"
SNMPv2-MIB::sysServices.0 = INTEGER: 79

root@voltaire:~# snmpwalk -v 2c -c oc12fast cisivrdb.med.utah.edu system
Timeout: No Response from cisivrdb.med.utah.edu

root@voltaire:~# snmpwalk -v 2c -c oc12fast ov3.med.utah.edu system
SNMPv2-MIB::sysDescr.0 = STRING: "SunOS ov3 5.9 Generic_118558-17 sun4u"
SNMPv2-MIB::sysObjectID.0 = OID: SNMPv2-SMI::enterprises.11.2.3.10.1.2
DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (660134290) 76 days, 9:42:22.90
SNMPv2-MIB::sysContact.0 = ""
SNMPv2-MIB::sysName.0 = STRING: "ov3.med.utah.edu"
SNMPv2-MIB::sysLocation.0 = ""
SNMPv2-MIB::sysServices.0 = INTEGER: 72
SNMPv2-MIB::sysORLastChange.0 = Timeticks: (0) 0:00:00.00
root@voltaire:~# snmpwalk -v 2c -c oc12fast ov3.med.utah.edu sysDescr.0
SNMPv2-MIB::sysDescr.0 = STRING: "SunOS ov3 5.9 Generic_118558-17 sun4u"
#endif

#define FAILURE 666

int try_snmp_system(char *node_ip,char *os_type, char *hostname, char *commstr)
{
    FILE *fp, *fp1, *fpin;
    char *ptr, *sptr, buf[1000], cmd[1000], hardware[200], owner[200];
    char contact[200], location[200], host_name[200], tmpbuf[1000];
    char *comm_strs[10]={"1xR\\$bluE", "99U#u#U\\!x", "oc12fast", "public", ""}, comm_str[100];
    int i, num=0, got_sysdescr=0, got_sysname=0, got_location=0, cmds_to_try=2;

    // system  oid
    memset(os_type, 0, sizeof(os_type));

    for (i=0; *comm_strs[i]; i++) {
            sprintf(cmd,"snmpwalk -c %s -v 2c %s iso.3.6.1.2.1.1.1.0 2>&1",comm_strs[i],node_ip);
        slog("popen >%s<\n", cmd);


        if(!(fp = popen(cmd,"r"))) { slog("no popen >%s<\n", cmd); continue; }

        while(fgets(buf, 1000, fp)) {
#ifdef DEBUG1
           slog("GOT >%s<\n", buf);
#endif
           if(strstr(buf, "No Response") || strstr(buf, "Unknown host") ||
              strstr(buf, "timed out") || strstr(buf, "Unknown Object Identifier")) 
           {
               slog("fgets returns >%s<\n", buf);
               break; 
           }
           if(strstr(buf, "STRING")) {
               strcpy(commstr, comm_strs[i]);

               if(strstr(buf,"Cisco")) strcpy(os_type,"cisco");
               else if(strstr(buf,"AIX")) strcpy(os_type,"aix");
               else if(strstr(buf,"Foundry")) strcpy(os_type,"foundry");
               else if(strstr(buf,"SunOS")) strcpy(os_type,"sunos");
               else if(strstr(buf,"indows")) strcpy(os_type,"windows");
               else if(strstr(buf,"inux")) strcpy(os_type,"linux");
               else strcpy(os_type,"no-os");
               return 1;
           }
        } // while
        fclose(fp);
    } // for
    return 0;
}


int try_nmap(char *node_ip,  char *os_type)
{
    FILE *fp;
    char *ptr, os_details[500], buf[1000], cmd[1000];
    int got_tcp_ms_port=0;


    if(g_no_nmap) return 0;


    sprintf(cmd, "nmap -O %s 2>&1",node_ip);
    slog("popen >%s<\n", cmd);

    if(!(fp = popen(cmd,"r"))) {
        slog("Cant popen >%s<\n", cmd);
        return 0;
    }
     while(fgets(buf, 1000, fp)) {
       // slog("GOT >%s<\n", logbuf);
         if(buf[0] =='\n') continue;

         if(strstr(buf, "No exact OS matches")) slog("NO EXACT >%s<\n", buf);

         if(strstr(buf, "tcp")&& strstr(buf, "open")&& strstr(buf, "icrosoft")){
             slog("using microsoft for os_type from buf>%s<\n", buf);
             strcpy(os_type,"windows");
             got_tcp_ms_port =1;
             continue;
         }

         if(!strncmp(buf, "Running ", 8) || 
             strstr(buf, "Aggressive OS guesses: ") || 
             strstr(buf, "OS details: ")) 
         {
             slog("found os details section in >%s<\n", buf);
             if(get_os_type_from_details(buf, os_type)) {
                 pclose(fp);
                 return 1;
             }
         }
     }
     if(got_tcp_ms_port) return 1;
     return 0;
}
get_os_type_from_details(char *os_details, char *os_type)
{
    if(strstr(os_details, "indows") || 
      strstr(os_details, "icrosoft") ) 
        strcpy(os_type,"windows");
    else if(strstr(os_details, "Apple") || strstr(os_details, "FreeBSD") 
            ||strstr(os_details, "AIX"))
        strcpy(os_type,"aix");
    else if(strstr(os_details, "inux") ) 
        strcpy(os_type,"linux");
    else if(strstr(os_details, "Sun")) 
        strcpy(os_type,"sun");
    else return 0;

    return 1;
}
int parse_snmpwalk_for_mac_addr_and_insert_into_db(node_ip) {

}



// parsing:
// Host hsc126-1.med.utah.edu (155.100.126.1) appears to be up.
/*
#ifdef  youhome
 Linux:
 Starting Nmap 5.21 ( http://nmap.org ) at 2012-02-21 11:07 MST
 Nmap scan report for bahd-d.med.utah.edu (155.100.9.147)
 Host is up (0.0019s latency).
 Nmap scan report for 155.100.9.148
 Host is up (0.0019s latency).
 Nmap scan report for micky-9.med.utah.edu (155.100.9.149)
 Host is up (0.0021s latency).
 Nmap scan report for sarah-9.med.utah.edu (155.100.9.150)
 Host is up (0.0021s latency).
 Nmap done: 16 IP addresses (4 hosts up) scanned in 1.31 seconds
 
 Redhat:
 Starting Nmap 4.11 ( http://www.insecure.org/nmap/ ) at 2012-02-21 11:04 MST
 Host 9-145.med.utah.edu (155.100.9.145) appears to be up.
 Host bahd-d.med.utah.edu (155.100.9.147) appears to be up.
 Host 155.100.9.148 appears to be up.
 Host micky-9.med.utah.edu (155.100.9.149) appears to be up.
 Host sarah-9.med.utah.edu (155.100.9.150) appears to be up.
 Nmap finished: 16 IP addresses (5 hosts up) scanned in 6.989 seconds
#endif
*/
#ifdef NEEDED
int campus_node(char *node_ip) {
    return !(hospital_node(node_ip));
}
int hospital_node(char *node_ip)
{
    if(ip_first_octet_is_155_and_second_octet_is_100(node_ip) ||
       ip_first_octet_is_172(node_ip)) return 1;
    return 0;
}
int ip_first_octet_is_155_and_second_octet_is_100(char *node_ip)
{
    char *ptr, buf[10], *octet;

    if(strncmp(node_ip,"155", 3)) return 0;
    
    octet=buf;
    ptr=node_ip; ptr+=4;

    while(*ptr != '.') *(octet++) = *(ptr++); *octet='\0';
    if(atoi(buf) == 100) return 1;
    
    return 0;
} 

int ip_first_octet_is_172(char *node_ip)
{
    if(!strncmp(node_ip,"172", 3)) return 1;
    return 0;
}
#endif
#ifdef TESTMAIN
int remove_prens(char *dirty, char *clean)
{
    char buf[100], *sptr;
    if(*dirty == '(') {
        strcpy(buf,&dirty[1]);
        if(sptr = strchr(buf,')')) *sptr='\0';
        strcpy(clean, buf);
    } else strcpy(clean, dirty);
}
#endif
#ifdef TESTMAIN
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
#endif

int add_cert(char *host)
{
    char buf[200], cmd[200], name[200], pass[200];
    FILE *fp;

    slog("in add_cert (%s)\n", host);
    if(!get_name_pass("/smg/cfg/nagadm_acct_info.dat", name, pass)) {
        slog("Can not open /smg/cfg/nagadm_acct_info.dat");
        return 0;
    }
    slog("in add_cert: get_name_pass returns name of >%s<\n", name);

    sprintf(cmd,"/smg/bin/setupssh.exp %s %s %s", pass, host, name);
    slog("popen >%s<\n", cmd);
    if(!(fp=popen(cmd,"r"))) {
        slog("can NOT popen >%s<\n", cmd);
        return 0;
    }
    while(fgets(buf,200,fp)) {
        slog("add_cert: got >%s<\n", buf);
    }
    pclose(fp);
    return 1;
    //system(cmd);
}
