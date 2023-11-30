#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <number_of_clients>"
    exit 1
fi

# Number of total clients
total_clients=$1

# Initialize variables
total_response_time=0

# Loop through the clients and calculate response time
for ((i=1; i<=$total_clients; i++)); do
    #response_time=$(./response.sh)
    response_time=$(./loadtest.sh| grep -oP 'Response Time: \K\d+' | awk '{print $1}')
    total_response_time=$((total_response_time + response_time))
done

# Calculate throughput
throughput=$(echo "scale=2; $total_clients / $total_response_time" | bc)

echo "Total Clients: $total_clients"
echo "Total Response Time: $total_response_time seconds"
echo "Throughput: $throughput requests/second"
