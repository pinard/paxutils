#! /bin/sh
set -x
cd /tmp
for suffix in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; do
  mkdir abracadabra-$suffix
  cd abracadabra-$suffix
done
echo test >test1
ln test1 /tmp/abracadabra-A/test2
cd /tmp
tar cfvv test.tar abracadabra-A
tar dfvv test.tar abracadabra-A
rm -r abracadabra-A
tar xfvv test.tar
