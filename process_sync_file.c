#include <stdio.h>
#include <stdlib.h>
#include "log1.h"

main(int argc, char **argv)
{
    int i;
    FILE *fp;
    char cmd[10000], buf[10000], tmp_resub_file[200], *sptr, *ver="ver1.3";
    time_t start_time;
   
    open_log_file(argv[0]);
    slog("-----------------STARTING---------------%s--\n",ver);

    time(&start_time);

    sprintf(tmp_resub_file, "/tmp/state_sync.lst.%ld", start_time);
    slog("resub file is >%s<\n", tmp_resub_file);
    sprintf(cmd, "if [ -s /smg/out/state_sync.lst ]; then mv /smg/out/state_sync.lst %s; fi", tmp_resub_file);
    // test -s: file exists and has size > 0
    //system("if [ -s /smg/out/process_resub_buf.txt ]; then mv /smg/out/process_resub_buf.txt /tmp/rsczzx.txt; fi");
    slog("system(%s)\n", cmd);
    system(cmd);

    if(!(fp=fopen(tmp_resub_file, "r"))) { 
        slog("No new service alerts this minute....\n"); 
        slog("-----------------FINISHED---------------%s--\n", ver);
        exit(0); 
    }

    slog("NEW SERVICES: Calling reload_nagios...\n");
    reload_nagios();

    while(fgets(buf,1000,fp)) { REMOVE_NEWLINE(buf); write_to_pipe(&buf[13]); }
    fclose(fp);
#ifdef SUBMIT_TWICE
    system("sleep 5");
    if(!(fp=fopen(tmp_resub_file, "r"))) { 
        slog("fopen FAILED : WIERD - should NEVER happen...\n"); exit(0); 
    }
    while(fgets(buf,1000, fp)) write_to_pipe(&buf[13]);
    fclose(fp);
#endif
    sprintf(cmd,"rm %s 2>&1 > /dev/null", tmp_resub_file);
    //system("rm /tmp/rsczzx.txt 2>&1 > /dev/null");
    system(cmd);
    slog("-----------------FINISHED---------------%s--\n", ver);
}

int write_to_pipe(char *buf)
{
    char cmd[10000];
    time_t long_time;
   

    //slog("Got buf >%s<\n", buf);
    time(&long_time); 
    sprintf(cmd,"echo \"[%ld] %s\"  > /usr/local/nagios/var/rw/nagios.cmd", 
       long_time, buf);
    slog("system(%s)\n",cmd);
    system(cmd);
    slog("send_nsca SUCCESSFUL\n");
}

int reload_nagios()
{
    FILE *ptr;
    char buf[1000], cmd[200];
    int pipe_returned_nothing=1;

    slog("in reload_nagios\n");

    slog("System( /etc/init.d/nagios reload )  Returns:\n");
    ptr=popen("/etc/init.d/nagios reload","r");
    while(fgets(buf,5000,ptr)) {
        slog("%s\n",buf);
        pipe_returned_nothing=0;
        if(strstr(buf,"CONFIG ERROR!")) {
            slog("CONFIG ERROR!\n");
            pclose(ptr);
            return 0;
        }
    }
    slog("Nagios has been reloaded. Sleeping 20...\n");
    pclose(ptr);
    system("sleep 20");
    if(pipe_returned_nothing) {
        slog("Nagios restart pipe returned nothing!\n");
        return 0;
    }
    return 1;
}

