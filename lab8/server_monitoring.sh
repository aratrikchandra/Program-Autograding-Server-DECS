#!/bin/bash

mkdir -p ./cpu_utilizations
mkdir -p ./active_threads

m=$1

rm -rf file* output* compiler* prog* runtime* request_status.csv ./cpu_utilizations/cpu_utilization_clients_$m.txt

# Start taking CPU utilization snapshots
#$(top -bn 1 | grep '%Cpu' | awk '{print $2}' > "./cpu_utilizations/cpu_utilization_clients_$m.txt")& # 2

$(vmstat -n 1 > "./cpu_utilizations/cpu_utilization_clients_$m.txt")&
pids[0]=$!



rm -f ./active_threads/NLWP_clients_$m.txt
while true; do
    ps -eLf | grep server |tail -n +2 | head -n 1 | awk '{print $6}' >> ./active_threads/NLWP_clients_$m.txt
    sleep 1

   # top -b -n 1 | grep gradingserver | awk '/^%Cpu'/ {print 2}'

done &  # 3
pids[1]=$!
