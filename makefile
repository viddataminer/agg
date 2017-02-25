# USAGE check_nsync minutes
# calls restart_nsync if last log entry was over <minutes>
all: calc_uptime rin nsync ntail gn viz process_resub_buf
clean: 
	rm -rf  nsync ntail gn viz process_resub_buf calc_uptime 

rin: rin.c 
	gcc -o rin rin.c
rm_host: rm_host.c 
	gcc -o rm_host rm_host.c
calc_uptime: calc_uptime.c 
	gcc -o calc_uptime calc_uptime.c
add_mssql_host: add_mssql_host.c 
	gcc -DTESTMAIN -o add_mssql_host add_mssql_host.c
process_resub_buf: process_resub_buf.c 
	gcc -o process_resub_buf process_resub_buf.c
nsync: nsync.c 
	gcc -o nsync nsync.c
ntail: ntail.c 
	gcc -o ntail ntail.c
gn: gn.c 
	gcc -o gn gn.c
viz: viz.c 
	gcc -o viz viz.c
