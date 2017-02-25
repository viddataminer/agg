## 
##
##  This script will be called by rm_host to assure all refrences to a host are removed
##  from the *_host_group.cfg files in the /usr/local/nagios/etc/objects directory
num_grps=0
num_found=0
mkdir /tmp/clean 2>/dev/null
for i in /usr/local/nagios/etc/objects/*_group.cfg
do
    num_grps=`expr $num_grps + 1`
    # regex matches 'members' followed by any number of white space (space or tabs), followed by host
    grep "members\s*${1}" ${i} > /dev/null
    if [ $? -ne 0 ]; then
        #echo "pattern not found in $i "
        continue;
    fi
    num_found=`expr $num_found + 1`
    echo "found match in ${i}, cleaning..."
    grep -v "members\s*${1}" ${i} > ${i}.clean
    echo "cp ${i}.clean ${i}"
    cp ${i}.clean ${i}
done 
echo "found $1 in $num_found of $num_grps cheched"
exit $num_found
