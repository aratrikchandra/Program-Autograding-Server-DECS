#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <sourceCodeFileTobeGraded> <number_of_clients> <sleep_time_between_checks>"
    exit 1
fi

source_code="$1"
num_clients="$2"
sleep_time="$3"

# Function to simulate a client using throughput.sh
simulate_client() {
    local source_code="$1"
    local sleep_time="$2"
    
    ./throughput.sh "$source_code" "$sleep_time" > "client_$3.log" 2>&1
}

# Run clients in parallel
for ((i = 1; i <= num_clients; i++)); do
    simulate_client "$source_code" "$sleep_time" "$i" &
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
echo "Average Throughput: $average_throughput req/sec"
