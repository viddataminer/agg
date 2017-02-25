#set -x
diffcnt=0
for i in `ls *.${1}`
do 
   echo checking $i
   diff $i /tmp/agg/${i} > /tmp/${i}.diff 
   if [ $? != 0 ]; then
      echo "## $i is different - diff output in /tmp/${i}.diff ##" 
      diffcnt=`expr $diffcnt + 1`
   fi
done
echo "files different: $diffcnt "
