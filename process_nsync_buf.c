#include <stdio.h>
#include <stdlib.h>
#include "log1.h"
#define RESUB_BUF "/smg/tmp/nsync_buf"

main(int argc, char **argv)
{
    int i,done=0;
    FILE *fp;
    char rm_cmd[1000], cmd[10000], buf[10000], tmp_resub_file[200], *sptr, *ver="ver1.3";
    time_t start_time;
   
    open_log_file(argv[0]);
    slog("-----------------STARTING---------------%s--\n",ver);

    time(&start_time);

    sprintf(tmp_resub_file, "/tmp/resub_buf.%ld", start_time);
    sprintf(rm_cmd,"rm %s 2>&1 > /dev/null", tmp_resub_file);
    slog("resub file is >%s<\n", tmp_resub_file);
    sprintf(cmd, "mv %s %s 2>&1 > /dev/null", 
         RESUB_BUF, tmp_resub_file);
    system(cmd);

    if(!(fp=fopen(tmp_resub_file, "r"))) { 
        slog("No new service alerts this minute....\n"); 
        slog("-----------------FINISHED---------------%s--\n", ver);
        exit(0); 
    }

//    slog("NEW SERVICES: Calling reload_nagios...\n");
//    reload_nagios();

    while(fgets(buf,1000,fp))  {
        if(!done) {
            slog("NEW SERVICES: Calling reload_nagios...\n");
            reload_nagios();
            done=1;
        }
        write_to_pipe(&buf[13]); 
    }

    fclose(fp);
    system(rm_cmd);
    slog("-----------------FINISHED---------------%s--\n", ver);
}

int write_to_pipe(char *buf)
{
    char *sptr, cmd[10000];
    time_t long_time;
   

    //slog("Got buf >%s<\n", buf);
    time(&long_time); 
    REMOVE_NEWLINE(buf); 
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

    slog("System( service nagios reload )  Returns:\n");
    ptr=popen("service nagios reload","r");
    while(fgets(buf,5000,ptr)) {
        slog("%s\n",buf);
        pipe_returned_nothing=0;
        if(strstr(buf,"CONFIG ERROR!")) {
            slog("CONFIG ERROR!\n");
            pclose(ptr);
            return 0;
        }
    }
    slog("Nagios has been reloaded. Sleeping 30...\n");
    pclose(ptr);
    system("sleep 30");
    if(pipe_returned_nothing) {
        slog("Nagios restart pipe returned nothing!\n");
        return 0;
    }
    return 1;
}

