ps -eaf | grep -v grep | grep "tail -f /usr/local/nagios/var/nagios.log" | 
while read line
do 
  kill -9 `echo $line | cut -f2 -d' '` 
done
killall nsync
nohup /usr/local/bin/nsync /usr/local/nagios/var/nagios.log &
