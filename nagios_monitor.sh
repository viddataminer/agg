nagios_mon=0
nagios_mon=`ps -eaf | grep /usr/local/nagios/bin/nagios | grep -v grep | wc -l`
echo nagios_mon is ${nagios_mon}
if [ ${nagios_mon} -eq 0 ]; then
    echo "Nagios is not running on smgnagios!!!" | mailx ${1}@myairmail.com
#    /etc/init.d/nagios start
fi
