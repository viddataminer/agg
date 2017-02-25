#set -x
da_cnt=`ps -eaf | grep nsca | grep -v grep | wc -l`

#REMOVE this line after testing
echo "on `date` $da_cnt NSCA daemons are running!" >> /tmp/nsca_daemon_count

if [ -e /tmp/dont_repage_nsca_dcnt -a $da_cnt -lt 10 ]; then
    echo "OK: ${da_cnt} NSCA daemons are running." | mailx ${1}@myairmail.com
    echo "OK: ${da_cnt} NSCA daemons are running." | mailx ${1}@myairmail.com
    rm /tmp/dont_repage_nsca_dcnt
    exit
fi

if [ $da_cnt -ge 10 ]; then
    echo "WOAH: ${da_cnt} NSCA daemons are running on the aggro!!!" | mailx ${1}@myairmail.com
    echo "WOAH: ${da_cnt} NSCA daemons are running on the aggro!!!" | mailx ${1}@myairmail.com
    echo "$da_cnt NSCA daemons are running!" > /tmp/dont_repage_nsca_dcnt
fi
