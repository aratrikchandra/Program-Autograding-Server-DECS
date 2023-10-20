#!/bin/bash

numClients=$5
loopNum=$3
sleepTimeSeconds=$4
sourceCodeFile=$2
serverIPPort=$1    #IPAddress:PortNumber
# Start the clients in the background

mkdir -p clientList
for ((i=1; i<=numClients; i++))
do
    ./client $serverIPPort $sourceCodeFile $loopNum $sleepTimeSeconds > "clientList/client$i.out" &
done

# Wait for all clients to finish
wait

# Calculate overall throughput and average response time
total_throughput=0
total_response_time=0
total_responses=0

for ((i=1; i<=numClients; i++))
do
    responses_per_client=$(grep 'Number of successful responses' "clientList/client$i.out" | awk '{print $5}')
    avg_response_time_per_client=$(grep 'Average response time' "clientList/client$i.out" | awk '{print $4}')
    connection_time_per_client=$(grep 'Total time taken to complete the loop' "clientList/client$i.out" | awk '{print $8}')

# throughput=$(echo "scale=2; 1 / $avg_response_time_per_client" | bc)
# The "scale=2" sets the number of decimal places to 2. Adjust as needed.
throughput=$(echo "scale=2; $responses_per_client / $connection_time_per_client" | bc)


total_throughput=$(echo "scale=2; $total_throughput + $throughput" | bc)
#total_connection_time=$(echo "scale=2; $total_connection_time + $connection_time_per_client" | bc)

    total_response_time=$(echo "$total_response_time + $avg_response_time_per_client * $responses_per_client" | bc)
    total_responses=$((total_responses + responses_per_client))
done

average_response_time=$(echo "$total_response_time / $total_responses" | bc -l)
average_throughput=$(echo "$total_throughput / $numClients" | bc -l)

echo "Overall throughput: $total_throughput"
#echo "Overall throughput: $average_throughput"
echo "Average response time: $average_response_time"

#echo "Total connction time: $total_connection_time"

