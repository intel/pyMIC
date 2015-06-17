#!/bin/bash

HERE=$(cd $(dirname $0); pwd -P)

FILE=copy.pdf
if [[ "" != "$1" ]] ; then
  FILE=$1
  shift
fi

if [[ -f /cygdrive/c/Program\ Files/gnuplot/bin/wgnuplot ]] ; then
  GNUPLOT=/cygdrive/c/Program\ Files/gnuplot/bin/wgnuplot
elif [[ -f /cygdrive/c/Program\ Files\ \(x86\)/gnuplot/bin/wgnuplot ]] ; then
  GNUPLOT=/cygdrive/c/Program\ Files\ \(x86\)/gnuplot/bin/wgnuplot
else
  GNUPLOT=$(which gnuplot 2> /dev/null)
fi

if [[ "" != "${GNUPLOT}" ]] ; then
  env \
    GDFONTPATH=/cygdrive/c/Windows/Fonts \
    FILENAME=${FILE} \
  "${GNUPLOT}" copy.plt
fi

