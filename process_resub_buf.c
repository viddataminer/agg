#include <stdio.h>
#include <stdlib.h>
#include "log1.h"
#include "path.h"
//#define RESUB_BUF "/smg/tmp/resub_buf"
//#define NAGIOS_PIPE "/usr/local/nagios/var/rw/nagios.cmd"


main(int argc, char **argv)
{
    int i,done=0;
    FILE *fp_buf, *fp_pipe;
    char rm_cmd[1000], cmd[10000], buf[10000], tmp_resub_file[200], *sptr, *ver="ver1.3";
    time_t start_time;
   
    open_log_file(argv[0]);
    slog("-----------------STARTING---------------%s--\n",ver);

    time(&start_time);

    sprintf(tmp_resub_file, "/tmp/resub_buf.%ld", start_time);
    sprintf(rm_cmd,"rm %s 2>&1 > /dev/null", tmp_resub_file);
    slog("resub file is >%s<\n", tmp_resub_file);

    sprintf(cmd, "mv %s %s 2>&1 > /dev/null", RESUB_BUF, tmp_resub_file);
    system(cmd);

    if(!(fp_buf=fopen(tmp_resub_file, "r"))) { 
        slog("No new service alerts this minute....\n"); 
        slog("-----------------FINISHED---------------%s--\n", ver);
        exit(0); 
    }

    if(!(fp_pipe=fopen(NAGIOS_PIPE, "w"))) {
        while(fgets(buf,10000,fp_buf))  {
            sprintf(cmd,"echo %s >> %s", buf, RESUB_BUF);
            slog("nagios.cmd PIPE OPEN ERROR. system >%s<\n", cmd);
            system(cmd);
        }
        fclose(fp_buf);
        exit(1);
    }

    while(fgets(buf,10000,fp_buf))  {
        //REMOVE_NEWLINE(buf);
        if( (buf[0] != '[') || (buf[11] != ']') || (buf[12] != ' ') || 
                 (strncmp(&buf[13],"PROCESS",7)) )
        {
            slog("process_resub_buf BAD FORMAT: >%s<\n",buf); 
            continue;
        }
        slog("writing to pipe >%s<\n", buf);
        fprintf(fp_pipe,"%s", buf);
    }

    fclose(fp_pipe);
    fclose(fp_buf);
    system(rm_cmd);
    slog("-----------------FINISHED---------------%s--\n", ver);
}

int format_is_bad(char *buf)
{
    if(buf[0] != '[') return 1;
    if(buf[11] != ']') return 1;
    if(buf[12] != ' ') return 1;
    if(strncmp(&buf[13],"PROCESS",7)) return 1;
    return 0;
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
    slog("Nagios has been reloaded. Sleeping 15...\n");
    pclose(ptr);
    system("sleep 15");
    if(pipe_returned_nothing) {
        slog("Nagios restart pipe returned nothing!\n");
        return 0;
    }
    return 1;
}

