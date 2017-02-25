#echo "smgnag1;nsca_monitor;0;OK" | /smg/bin/send_nsca -H ${1} -d ';' -c /etc/send_nsca.cfg
## if nsca_monitor failed, $? will be set to a NON-ZERO value, so record and page out..
if [ $? -ne 0 ]; then
    echo "`date +'%y%m%d%H%M'` $?" >> /smg/log/nsca_monitor.log
    echo "NSCA is not running on `hostname` ! Restarting..." | mailx ${2}@myairmail.com
    /etc/init.d/nsca restart
else 
    echo "`date +'%y%m%d%H%M'` $?" >> /smg/log/nsca_monitor.log
fi
