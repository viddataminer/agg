#include <stdio.h>
#include <stdlib.h>
#include "log1.h"
#define NSYNC_RESUB_BUF "/smg/tmp/nsync_buf"
#define NAGIOS_PIPE "/usr/local/nagios/var/rw/nagios.cmd"
#define ALERT_LOG "/smg/log/alert.log"
int g_seconds_to_sleep=40;

main(int argc, char **argv)
{
    int i,done=0, num_lines=0, num_written=0;
    FILE *fpr, *fpw;
    char rm_cmd[1000], cmd[10000], buf[10000], tmp_resub_file[200], *sptr, *ver="ver1.3";
    char log_info[1000], buf_ary[10000][300];
    time_t start_time;
   
    open_log_file(argv[0]);
    slog("-----------------STARTING---------------%s--\n",ver);

    if(argc==2) g_seconds_to_sleep = atoi(argv[1]);

    time(&start_time);

    sprintf(tmp_resub_file, "/tmp/resub_nsync_buf.%ld", start_time);
    sprintf(rm_cmd,"rm %s 2>&1 > /dev/null", tmp_resub_file);
    slog("resub file is >%s<\n", tmp_resub_file);
    sprintf(cmd, "mv %s %s 2>&1 > /dev/null", NSYNC_RESUB_BUF, tmp_resub_file);
    system(cmd);

    if(!(fpr=fopen(tmp_resub_file, "r"))) { 
        slog("No new service alerts this minute....\n"); 
        slog("-----------------FINISHED---------------%s--\n", ver);
        system(rm_cmd);
        exit(0); 
    }
    slog("nsync_resub_buf contains:\n");
    while(fgets(buf,1000,fpr)) {
        strcpy(buf_ary[num_lines++],buf);
        //fprintf(fpw,"%s",buf);
        slog("%s", buf);
    }
    slog("FOUND %d NEW SERVICES: Calling reload_nagios...\n", num_lines);

    reload_nagios();

    if(!(fpw=fopen(NAGIOS_PIPE, "w"))) {
       slog("ERROR nagios.cmd: Could NOT open pipe to write!!\n");
       sprintf(log_info,"echo \"`date '+%%m-%%y %%H:%%M:%%S'` ERROR CMD PIPE: NO OPEN %s\n\" >> %s", NSYNC_RESUB_BUF, ALERT_LOG);
       system(log_info);
       exit(8);
    }
   
    slog("WRITTING TO PIPE:\n");
    while(num_written < num_lines) 
        fprintf(fpw,"%s", buf_ary[num_written++]);
    fclose(fpr);
    fclose(fpw);

/*
    while(fgets(buf,1000,fpr)) {
        fprintf(fpw,"%s",buf);
        slog("%s", buf);
    }
*/
    system(rm_cmd);
    slog("-----------------FINISHED---------------%s--\n", ver);
}

int reload_nagios()
{
    FILE *ptr;
    char sleep_cmd[200], buf[1000], cmd[200];
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
    pclose(ptr);
    sprintf(sleep_cmd,"sleep %d", g_seconds_to_sleep);
    slog("system(%s)\n", sleep_cmd);
    system(sleep_cmd);
    if(pipe_returned_nothing) {
        slog("Nagios restart pipe returned nothing!\n");
        return 0;
    }
    return 1;
}

