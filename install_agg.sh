#set -x
#ver 1.2
do_ubuntu_install() 
{
    package=${1}
    echo "Installing package ${package}.  Output being sent to /tmp/${package}"
    echo 'y' | apt-get install $package > /tmp/${package}.log
    if [ $? -ne 0 ]; then
        echo -e "Package $package FAILED TO INSTALL SUCCESSFULLY.  Check /tmp/${package}\nfor details." 
    fi
}

homedir=`pwd`

me=`whoami`
cat /proc/version | grep Ubuntu
if [ $? -eq 0 ]; then
   ROOT_CRON_FILE="/var/spool/cron/crontabs/root";
else
   ROOT_CRON_FILE="/var/spool/cron/root";
fi

if [ "${me}" != "root" ]; then
    echo "[31mPlease run this script as root. Good Bye.[0m"
    exit 0
fi

echo ""
echo ""
echo -e "Installing the Core Nagios Product... Be Patient Please."
#echo -e "Install the Core Nagios Product? (y/n):\c"
#read answer
#if [ $answer = 'y' -o "${answer}" = "yes" -o $answer = 'Y' ]; then

    apt-get update  2>&1 > /tmp/update.log
    echo 'y' | apt-get upgrade
#    echo 'y' | apt-get install apache2
    do_ubuntu_install apache2
    echo 'y' | apt-get install libapache2-mod-php5
    echo 'y' | apt-get install build-essential
    echo 'y' | apt-get install libgd2-xpm-dev
    echo 'y' | apt-get install apache2-utils
    echo ""
    #echo -e "\nUse of nagios requires a nagios user.\n"
    echo "Adding user nagios"
    useradd -m nagios
    echo ""
    while :; do
        echo -e "\nPlease enter the password for user nagios\n"
        passwd nagios
        if [ $? -eq 0 ]; then
            break
        fi
        echo -e "\nThe passwords did NOT match meathead... try again\n"
    done

    groupadd nagcmd
    usermod -a -G nagcmd nagios
    usermod -a -G nagcmd www-data
    echo "Issue Cmd: wget http://prdownloads.sourceforge.net/sourceforge/nagios/nagios-4.2.4.tar.gz"
    wget http://prdownloads.sourceforge.net/sourceforge/nagios/nagios-4.2.4.tar.gz
    tar xzf nagios-4.2.4.tar.gz
    cd nagios-4.2.4
    ./configure --with-command-group=nagcmd
    echo "Got Nagios and ready to compile... One moment please"
    echo "For Build details, please check /tmp/nagios_build.log"
    echo "Calling make all..."
    make all > /tmp/nagios_build.log
    make install >> /tmp/nagios_build.log
    echo "Calling make install-init..."
    make install-init >> nagios_build.log
    echo "Calling make install-config..."
    make install-config >> nagios_build.log
    echo "Calling make install-commandmode..."
    make install-commandmode >> nagios_build.log
    echo "Calling make install-webconf..."
    make install-webconf >> nagios_build.log
    echo "The administrator account for Web access is nagiosadmin"
    echo "It is required to set up a password for administrator account nagiosadmin"
    while :; do
        echo -e "\nPlease enter the password for user 'nagiosadmin'\n"
        htpasswd -c /usr/local/nagios/etc/htpasswd.users nagiosadmin
        if [ $? -eq 0 ]; then
            break
        fi
        echo -e "\nThe passwords did NOT match meathead... try again\n"
    done
    /etc/init.d/apache2 reload
    cd $homedir
    ln -s /etc/init.d/nagios /etc/rcS.d/S99nagios
    chmod 777 /usr/local/nagios/*
    mkdir -p /usr/local/nagios/var/spool
    chown nagios:nagios /usr/local/nagios/var/spool
    mkdir -p /usr/local/nagios/var/spool/checkresults
    chown nagios:nagios /usr/local/nagios/var/spool/checkresults
    chown nagios:nagios /usr/local/nagios/var
#    cp /usr/local/nagios/etc/nagios.cfg /smg/bk

    cp commands.cfg hostgroups.cfg hosts.cfg host_template.cfg localhost.cfg services.cfg services_template.cfg stateless_services.cfg  /usr/local/nagios/etc/objects/

#    if [ "x${1}x" != "xvanillax" ]; then
#        cp nagios.cfg /usr/local/nagios/etc/
#    else 
#        cp localhost.vanilla.cfg /usr/local/nagios/etc/objects/localhost.cfg
#    fi

    chown nagios.nagios /usr/local/nagios/etc/objects/*.cfg
    chown nagios.nagios /usr/local/nagios/etc/nagios.cfg
    /usr/local/nagios/bin/nagios -v /usr/local/nagios/etc/nagios.cfg
    /etc/init.d/nagios start
#fi

cd $homedir
echo "Hit return to continue...\n"
read junk
clear
echo ""
echo ""
#wget http://prdownloads.sourceforge.net/sourceforge/nagios/nsca-2.7.2.tar.gz

#echo "Copying modified nsca.c to 2.7.2 source base and REBUILDING nsca..."

#tar -zxvf nsca-2.7.2.tar.gz
#cd nsca-2.7.2/
#./configure
#make all
##cp src/nsca.c /smg/bk
#cp ${homedir}/nsca.c src
#make nsca
#cp src/nsca /usr/local/nagios/bin/
## OKt  Lets start moving some files around. Issue the 3 commands:
#cp sample-config/nsca.cfg /usr/local/nagios/etc/
chown nagios.nagios /usr/local/nagios/etc/nsca.cfg
#chmod g+r /usr/local/nagios/etc/nsca.cfg
cd $homedir

# Now try to fire that baby up. Issue:
#/usr/local/nagios/bin/nsca -c /usr/local/nagios/etc/nsca.cfg

#if [ $? != 0 ]; then
#    echo -e "\n[31mNSCA ERROR: Check nsca Build results[0m" 
#else
#    echo -e "\n[32mnsca was successfully started.[0m"
#fi

cp ${homedir}/main.php /usr/local/nagios/share/

echo "Nagios has been successfully setup."
#echo -e "\nHit return to continue\c"
#read junk
#if [ "x${1}" = "xnagios" ]; then 
#   echo "Nagios Setup Completed Successfully."
#   exit 3
#fi
#if [ "x${1}" = "xvanilla" ]; then 
#   echo "Vanilla Nagios Setup Completed Successfully."
#   exit 3
#fi

cd $homedir
echo ""
echo "Building Aggregator biniaries."
echo ""
make clean
make all
#mkdir /smg 2>&1 > /dev/null
mkdir -p /smg/bin 2>&1 > /dev/null
cp nsca_monitor.sh nagios_monitor.sh nagios_create_monitor.sh restart_nagios_create_due_to_log_rotation.sh /smg/bin
cp  nagios_create ntail nagios_search viz event_queue calc_uptime /smg/bin
if [ $? -ne 0 ]; then
    echo ""
    echo "Aggregator /smg/bin biniaries FAILED to build."
    echo ""
else
    echo "" 
    echo "Aggregator /smg/bin biniaries built successfully."
    echo ""
fi
cp restart_nagios_create start_new_nagios_create start_nagios_create stop_nagios_create add_mssql_host ntail viz /smg/bin
if [ $? -ne 0 ]; then
    echo ""
    echo "Aggregator admin biniaries FAILED to build."
    echo ""
else
    echo ""
    echo "Aggregator admin biniaries built successfully."
    echo ""
fi
echo ""
echo "Scripts are now in place which will monitor basic Nagios functionality."
echo "Should Nagios Fail, Alerts will be sent via mailx/myairmail.com to a pager."
echo -e "Please enter your 10 digit pager number(no dashes or crap):\c"
read group_pager_number
clear
echo "The Root Crontable will now be appended with the following: ."
echo ""
echo "##########################################################################" 
echo "#####                         Aggregator                            ######" 
echo "##########################################################################" 
echo "* * * * * /smg/bin/process_resub_buf > /dev/null 2>&1" 
echo "2 0 * * * if [ -x /usr/local/nagios ]; then /smg/bin/restart_nsync_due_to_log_rotation.sh; fi > /dev/null 2>&1" 
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nsca_monitor.sh $group_pager_number; fi > /dev/null 2>&1" 
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nagios_monitor.sh $group_pager_number ; fi > /dev/null 2>&1" 
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nsync_monitor.sh $group_pager_number ; fi > /dev/null 2>&1" 
echo ""
#echo -e "Modify the Crontable (y/n):\c"
#read junk
#if [ $junk != 'y' -a $junk != 'Y' ]; then
#    echo "Enjoy!"
#    exit
#fi

echo "##########################################################################" >> $ROOT_CRON_FILE
echo "#####                         Aggregator                            ######" >> $ROOT_CRON_FILE
echo "##########################################################################" >> $ROOT_CRON_FILE
echo "* * * * * /smg/bin/process_resub_buf > /dev/null 2>&1" >> $ROOT_CRON_FILE
echo "2 0 * * * if [ -x /usr/local/nagios ]; then /smg/bin/restart_nsync_due_to_log_rotation; fi > /dev/null 2>&1" >> $ROOT_CRON_FILE
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nsca_monitor.sh $group_pager_number ; fi > /dev/null 2>&1" >> $ROOT_CRON_FILE
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nagios_monitor.sh $group_pager_number ; fi > /dev/null 2>&1" >> $ROOT_CRON_FILE
echo "* * * * * if [ -x /usr/local/nagios ]; then /smg/bin/nsync_monitor.sh $group_pager_number ; fi > /dev/null 2>&1" >> $ROOT_CRON_FILE


echo nohup /smg/bin/nagios_create /usr/local/nagios/var/nagios.log &
nohup /smg/bin/nagios_create /usr/local/nagios/var/nagios.log &
echo ""
echo ""
echo "This completes the Aggregator Installation.  Hit any key to continue"
read junk
