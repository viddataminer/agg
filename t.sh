for i in `cat /tmp/hosts`
do
echo -e "\ndefine host {\n\thost_name\t\t\t${i}\n\talias    \t\t\t${i}\n\taddress\t\t\t\t${i}\n\tuse\t\t\t\tpassive-host\n}\n\n" >> /tmp/jj35
done
