#!/bin/bash

HERE=$(cd $(dirname $0); pwd -P)
NAME=$(basename ${HERE})

if [[ "-mic" != "$1" ]] ; then
  if [[ "$1" == "o"* ]] ; then
    FILE=copyout.dat
  else
    FILE=copyin.dat
  fi
  env \
    KMP_AFFINITY=scatter,granularity=fine \
    OFFLOAD_INIT=on_start \
    MIC_ENV_PREFIX=MIC \
    MIC_KMP_AFFINITY=scatter,granularity=fine \
  ${HERE}/${NAME} $* | \
  tee ${FILE}
else
  shift
  if [[ "$1" == "o"* ]] ; then
    FILE=copyout.dat
  else
    FILE=copyin.dat
  fi
  env \
    SINK_LD_LIBRARY_PATH=$MIC_LD_LIBRARY_PATH \
  micnativeloadex \
    ${HERE}/${NAME} -a "$*" \
    -e "KMP_AFFINITY=scatter,granularity=fine" | \
  tee ${FILE}
fi

