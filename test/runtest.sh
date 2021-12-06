#!/bin/sh 

#sh test/runtest.sh "./dicheck -h"   "test/testcase"  test/baseta  di
#sh test/runtest.sh "./trimtrailing" "test/testcasex" test/basett-a tt

if [ $# -ne 4 ]
then
  echo "FAIL bad test command $*"
  exit 1
fi
c=$1
f=$2
b=$3
t=$4
n=test/junktx
tto=test/junk.ttorig

if [ x$t = "xtt" ] 
then
  cp $f $tto
  $c $f  
  diff $f $b >junk.difference
  if [ $? -ne 0 ]
  then
    echo "FAIL  $* "
    cat junk.difference
    echo "To update: mv $f $b"
    exit 1
  fi
else
  $c $f >$n  
  diff $b $n >test/junk.difference
  if [ $? -ne 0 ]
  then
    echo "FAIL  $* "
    cat test/junk.difference
    echo "To update: mv $n $b"
    exit 1
  fi
fi
exit 0
