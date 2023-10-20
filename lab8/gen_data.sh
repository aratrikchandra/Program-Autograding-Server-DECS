#!/bin/bash

loopNum=$3
sleepTimeSeconds=$4
maxClients=$6
serverIPPort=$1
sourceCodeFile=$2
timeout=$5

# Extract the port number from serverIPPort
portNumber=$(echo "$serverIPPort" | cut -d ':' -f 2)


mkdir -p cpu_utilizations
mkdir -p active_threads
rm -f cpu_util_and_threads_stats.txt result/results.csv

mkdir -p result
# Create files to store the results
echo "Clients,Throughput,ResponseTime,Goodput,TimeoutRate,ErrorRate,RequestSentRate" > "result/results.csv"

touch "cpu_util_and_threads_stats.txt"
echo "avg_cpu_utilization_%,avg_active_threads" > ./cpu_util_and_threads_stats.txt
cpu_utilization=0
active_threads=0


# Run the load test for varying numbers of clients
for ((numClients=10; numClients<=maxClients; numClients+=10))
do
    echo "Running load test with $numClients clients..."

    # Start taking CPU utilization snapshots
    $(vmstat -n 1 > "./cpu_utilizations/cpu_utilization_clients_$numClients.txt")& # 2
    pids[0]=$!

    # Start taking active threads snapshots
    rm -f ./active_threads/NLWP_clients_$numClients.txt
    while true; do
        ps -eLf | grep server |tail -n +2 | head -n 1 | awk '{print $6}' >> ./active_threads/NLWP_clients_$numClients.txt
        sleep 1
    done &  # 3
    pids[1]=$!

    # Wait for 10 seconds
    sleep 1
    # sleep 1

    # Start the load test
    output=$(./loadtest.sh $serverIPPort $sourceCodeFile $loopNum $sleepTimeSeconds $timeout $numClients)

    sleep 2
    # Stop taking snapshots
    # kill %2 %3
    for pid in ${pids[*]}; do
        kill -9 $pid
    done
    killall  -9 vmstat 
	
    throughput=$(echo "$output" | grep 'Overall throughput' | awk '{print $3}')
    response_time=$(echo "$output" | grep 'Average response time' | awk '{print $4}')
    goodput=$(echo "$output" | grep 'Overall Successful request rate(goodput)' | awk '{print $5}')
    timeout=$(echo "$output" | grep 'Overall timeout rate' | awk '{print $4}')
    errorrate=$(echo "$output" | grep 'Overall error rate' | awk '{print $4}')
    sentrate=$(echo "$output" | grep 'Overall request sent rate' | awk '{print $5}')

    
    echo "$numClients,$throughput,$response_time,$goodput,$timeout,$errorrate,$sentrate" >> "result/results.csv"
    
        # Calculate avg of cpu util and active threads by reading from respective files
    cpu_utilization=$(cat ./cpu_utilizations/cpu_utilization_clients_$numClients.txt | awk 'BEGIN {sum=0} NR > 2 { sum += (100- $15); n++ } END { if (n > 0) printf "%.2f\n", sum / n }')
    avg_threads=$(cat ./active_threads/NLWP_clients_$numClients.txt | awk 'BEGIN {sum=0} NR > 2 { sum += $1; n++ } END { if (n > 0) printf "%.2f\n", sum / n }')
    
    echo "${cpu_utilization},${avg_threads}" >> ./cpu_util_and_threads_stats.txt

    echo "Finished experiment with m=$numClients"
done

paste -d, "result/results.csv" cpu_util_and_threads_stats.txt > "result/experiment_results.csv"


rm -rf cpu_utilizations active_threads cpu_util_and_threads_stats.txt "result/results.csv"

echo "All experiments finished"
echo "Done. The results are in 'results.csv'"

python3 plot.py
