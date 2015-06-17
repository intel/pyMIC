#!/bin/bash

export KMP_AFFINITY=scatter
export OFFLOAD_INIT=on_start
export MIC_USE_2MB_BUFFERS=2m
export MIC_ENV_PREFIX=MIC
export MIC_KMP_AFFINITY=balanced,granularity=fine

HERE=$(cd $(dirname $0); pwd -P)

if [[ "-test" == "$1" ]] ; then
  TESTS=( \
    "16 1 2" \
    "19 1 4" \
    "25 1 13" \
    "45 1 4" \
  )
else
  TESTS=( "$*" )
fi

for TEST in "${TESTS[@]}" ; do
  ${HERE}/entropy ${TEST}
done

