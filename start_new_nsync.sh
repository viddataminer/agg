ps -eaf | grep -v grep | grep "tail -f /usr/local/nagios/var/nagios.log" | 
while read line
do 
  kill -9 `echo $line | cut -f2 -d' '` 
done
killall nsync
cp /usr/local/bin/nsync /usr/local/bin/nsync.bk
cp /home/u0363077/nsync /usr/local/bin/nsync
nohup /usr/local/bin/nsync /usr/local/nagios/var/nagios.log &
