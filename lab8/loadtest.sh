#!/bin/bash

numClients=$6
timeout=$5
loopNum=$3
sleepTimeSeconds=$4
sourceCodeFile=$2
serverIPPort=$1    #IPAddress:PortNumber
# Start the clients in the background

mkdir -p "clientList$numClients"
for ((i=1; i<=numClients; i++))
do
    ./client $serverIPPort $sourceCodeFile $loopNum $sleepTimeSeconds $timeout> "clientList$numClients/client$i.out" &
done

# Wait for all clients to finish
wait

# Calculate overall throughput and average response time
total_throughput=0
total_goodput=0
total_timeout_rate=0
total_error_rate=0
total_response_time=0
total_responses=0
total_requests_sent_rate=0

for ((i=1; i<=numClients; i++))
do
    responses_per_client=$(grep 'Number of successful responses' "clientList$numClients/client$i.out" | awk '{print $5}')
    timeouts_per_client=$(grep 'No. of timeouts' "clientList$numClients/client$i.out" | awk '{print $4}')
    errors_per_client=$(grep 'No. of Erroenous requests responses' "clientList$numClients/client$i.out" | awk '{print $4}')
    avg_response_time_per_client=$(grep 'Average response time' "clientList$numClients/client$i.out" | awk '{print $4}')
    connection_time_per_client=$(grep 'Total time taken to complete the loop' "clientList$numClients/client$i.out" | awk '{print $8}')


# throughput=$(echo "scale=2; 1 / $avg_response_time_per_client" | bc)
# The "scale=2" sets the number of decimal places to 2. Adjust as needed.

# per client basis
goodput_per_client=$(echo "scale=2; 1 / $avg_response_time_per_client" | bc)
throughput=$(echo "scale=2; $responses_per_client / $connection_time_per_client" | bc)
timeout_rate_per_client=$(echo "scale=2; $timeouts_per_client / $connection_time_per_client" | bc)
error_rate_per_client=$(echo "scale=2; $errors_per_client / $connection_time_per_client" | bc)



# overall basis
total_throughput=$(echo "scale=2; $total_throughput + $throughput" | bc)
total_goodput=$(echo "scale=2; $total_goodput + $goodput_per_client" | bc)
total_timeout_rate=$(echo "scale=2; $total_timeout_rate + $timeout_rate_per_client" | bc)
total_error_rate=$(echo "scale=2; $total_error_rate + $error_rate_per_client" | bc)

#total_connection_time=$(echo "scale=2; $total_connection_time + $connection_time_per_client" | bc)

    total_response_time=$(echo "$total_response_time + $avg_response_time_per_client * $responses_per_client" | bc)
    total_responses=$((total_responses + responses_per_client))
done

average_response_time=$(echo "$total_response_time / $total_responses" | bc -l)
# average_throughput=$(echo "$total_throughput / $numClients" | bc -l)
total_requests_sent_rate=$(echo "scale=2; $total_throughput + $total_timeout_rate + $total_error_rate" | bc)

#rm -r "clientList$numClients"

#echo "Overall throughput: $average_throughput"
echo "Overall throughput: $total_throughput"
echo "Overall Successful request rate(goodput): $total_goodput"
echo "Overall timeout rate: $total_timeout_rate"
echo "Overall error rate: $total_error_rate"
echo "Overall request sent rate: $total_requests_sent_rate"
echo "Average response time: $average_response_time"

#echo "Total connection time: $total_connection_time"

