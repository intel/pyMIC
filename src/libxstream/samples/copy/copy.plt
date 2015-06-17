FILENAME = system("sh -c \"echo ${FILENAME}\"")
if (FILENAME eq "") {
  FILENAME = "copy.pdf"
}
FILEEXT = system("sh -c \"echo ".FILENAME." | sed 's/.\\+\\.\\(.\\+\\)/\\1/'\"")
FILTER = "sed -n 's/\\(.\\+\\) Byte x \\(.\\+\\): \\(.\\+\\) MB\\/s/\\1 \\2 \\3/p'"

set output FILENAME
set terminal FILEEXT
set termoption enhanced
#set termoption font "Times-Roman,7"
save_encoding = GPVAL_ENCODING
set encoding utf8

set grid xtics lc "grey"
set grid ytics lc "grey"
set xlabel "data size [MB]"
set ylabel "bandwidth [MB/s]"
set autoscale fix
set mytics 2
set logscale x
set format x "%g"

plot "<(sh -c \"".FILTER." copyin.dat\")" using ($1/(1024*1024)):3 notitle smooth sbezier lc 1, \
     (1/0) title "copy-in" with points pt 7 ps 1 lc 1, \
     "" using ($1/(1024*1024)):3 notitle with points pt 7 ps 0.3 lc 1, \
     "<(sh -c \"".FILTER." copyout.dat\")" using ($1/(1024*1024)):3 notitle smooth sbezier lc 2, \
     (1/0) title "copy-out" with points pt 7 ps 1 lc 2, \
     "" using ($1/(1024*1024)):3 notitle with points pt 7 ps 0.3 lc 2
