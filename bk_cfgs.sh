cp /usr/local/nagios/etc/objects/services.cfg /usr/local/nagios/etc/objects/bk/services.`date '+%y%m%d%H%M'` 2>&1 > /dev/null
cp /usr/local/nagios/etc/objects/hosts.cfg /usr/local/nagios/etc/objects/bk/hosts.`date '+%y%m%d%H%M'` 2>&1 > /dev/null
cp * /usr/local/nagios/etc/objects/bk
