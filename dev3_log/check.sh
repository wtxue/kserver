#/bin/bash
ps -eo pid,tty,user,comm,stime,etime | grep -v grep | grep dev3_log
