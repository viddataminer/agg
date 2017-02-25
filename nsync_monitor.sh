CURRENT_SOFT_COUNT_FILE="/smg/out/nsync_monitor_current_soft_count"
LOG_FILE="/smg/log/nsync_monitor.log"
SOFT_COUNT_TO_ALERT_ON=10
CONFIG_ERR=666
#
#Initialize run count
if [ ! -e $CURRENT_SOFT_COUNT_FILE ]; then
    echo "0" > $CURRENT_SOFT_COUNT_FILE
fi
#
num_nsync_procs=`ps -eaf | grep /usr/local/bin/nsync | grep -v grep | wc -l`
echo " ------ `date +'%y%m%d%H%M'` num nsync procs: $num_nsync_procs ------" >> $LOG_FILE
if [ ${num_nsync_procs} -eq 0 ]; then

    CURRENT_SOFT_COUNT=`cat /smg/out/nsync_monitor_current_soft_count`
    if [ $CURRENT_SOFT_COUNT -eq $CONFIG_ERR ]; then
        echo "CURRENT_SOFT_COUNT = $CONFIG_ERR : Config Error" >> $LOG_FILE
        /usr/local/nagios/bin/nagios -v /usr/local/nagios/etc/nagios.cfg 2>&1 > /dev/null
        # if the config error has been resolved, reset soft count
        # so we can do it all again!
        if [ $? -eq 0 ]; then
            echo " Reseting soft count" >> $LOG_FILE
            echo "0" > $CURRENT_SOFT_COUNT_FILE
        else
            echo " ------ DONE ------" >> $LOG_FILE
            exit 5
        fi
    fi

    /usr/local/nagios/bin/nagios -v /usr/local/nagios/etc/nagios.cfg 2>&1 > /dev/null
    if [ $? -ne 0 ]; then
       page_text=`/usr/local/nagios/bin/nagios -v /usr/local/nagios/etc/nagios.cfg | grep Error:`
       echo "NAGIOS CONFIG ERROR: $page_text " | mailx ${1}@myairmail.com
       echo $CONFIG_ERR > $CURRENT_SOFT_COUNT_FILE
       echo " ------ DONE ------" >> $LOG_FILE
       exit 2
    fi

    CURRENT_SOFT_COUNT=`expr $CURRENT_SOFT_COUNT + 1`
    echo CURRENT_SOFT_COUNT is $CURRENT_SOFT_COUNT >> $LOG_FILE 
    if [ $CURRENT_SOFT_COUNT -eq $SOFT_COUNT_TO_ALERT_ON ]; then
       echo "nsync Not Running for ${CURRENT_SOFT_COUNT} minutes.. Restarting" | mailx ${1}@myairmail.com
       echo SEND PAGE: CURRENT_SOFT_COUNT is $CURRENT_SOFT_COUNT >> $LOG_FILE
       echo "0" > $CURRENT_SOFT_COUNT_FILE
    else
       echo NO PAGE: NSYNC NOT RUNNING: cnt is $CURRENT_SOFT_COUNT >> $LOG_FILE
       echo $CURRENT_SOFT_COUNT > $CURRENT_SOFT_COUNT_FILE
    fi
    /usr/local/bin/restart_nsync.sh
elif [ ${num_nsync_procs} -gt 1 ]; then
    echo "WIERD: number of nsync procs running is $num_nsync_procs. Retarting" >> $LOG_FILE
    /usr/local/bin/restart_nsync.sh
else
    echo OK. num_nsync_procs is $num_nsync_procs >> $LOG_FILE
    echo "0" > $CURRENT_SOFT_COUNT_FILE
fi
echo " ------ DONE ------" >> $LOG_FILE
