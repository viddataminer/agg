# USAGE check_nsync minutes
# calls restart_nsync if last log entry was over <minutes>
old: gn viz process_nsync_buf process_resub_buf nsync ntail
new: process_resub_buf_n nsync_n ntail_n gn_n viz_n process_nsync_buf_n
clean: 
	rm -rf  bin/*

process_resub_buf: process_resub_buf.c 
	gcc -w -o bin/process_resub_buf process_resub_buf.c
process_nsync_buf: process_nsync_buf.c 
	gcc -w -o bin/process_nsync_buf process_nsync_buf.c
nsync: nsync.c 
	gcc -w -o bin/nsync nsync.c
ntail: ntail.c 
	gcc -w -o bin/ntail ntail.c
gn: gn.c 
	gcc -w -o bin/gn gn.c
viz: viz.c 
	gcc -w -o bin/viz viz.c
process_resub_buf_n: process_resub_buf.c 
	gcc -w -o bin/check_4_incomming_alerts process_resub_buf.c
process_nsync_buf_n: process_nsync_buf.c 
	gcc -w -o bin/check_4_nagios_updates process_nsync_buf.c
nsync_n: nsync.c 
	gcc -w -o bin/update_nagios nsync.c
ntail_n: ntail.c 
	gcc -w -o bin/tail_nagios ntail.c
gn_n: gn.c 
	gcc -w -o bin/gn gn.c
viz_n: viz.c 
	gcc -w -o bin/viz viz.c
