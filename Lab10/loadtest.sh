#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <sourceCodeFileTobeGraded> <serverIP:port> <number_of_clients> <sleep_time_between_checks>"
    exit 1
fi

source_code="$1"
num_clients="$3"
sleep_time="$4"
server_address="$2"

# Function to simulate a client using throughput.sh

# Run clients in parallel
for ((i = 1; i <= num_clients; i++)); do
	./throughput.sh "$source_code" "$server_address" "$sleep_time" > "client_$i.log" &
done

# Wait for all background processes to finish
wait

# Calculate average response time and average throughput
total_response_time=0
total_throughput=0

for ((i = 1; i <= num_clients; i++)); do
    response_time=$(grep "Response Time" "client_$i.log" | awk '{print $3}')
    throughput=$(grep "Throughput" "client_$i.log" | awk '{print $2}')
    
    total_response_time=$((total_response_time + response_time))
    total_throughput=$(bc <<< "$total_throughput + $throughput")
done

average_response_time=$(bc <<< "scale=2; $total_response_time / $num_clients")
average_throughput=$(bc <<< "scale=2; $total_throughput / $num_clients")

echo "Average Response Time: $average_response_time seconds"
echo "Overall Throughput: $total_throughput req/sec"
