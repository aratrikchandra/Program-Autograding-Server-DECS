#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <sourceCodeFileTobeGraded> <serverIP:port> <max_clients> <sleep_time_between_checks>"
    exit 1
fi

source_code="$1"
max_clients="$3"
sleep_time="$4"
server_address="$2"

results_csv="loadtest_results.csv"

# Ensure the CSV file has the header
echo "NumClients,AvgResponseTime,AvgThroughput" > "$results_csv"

for ((clients = 10; clients <= max_clients; clients += 10)); do
    echo "Running loadtest.sh with $clients clients..."
    
    # Run loadtest.sh and capture output
    output=$(./loadtest.sh "$source_code" "$server_address" "$clients" "$sleep_time")

    # Extract AvgResponseTime and AvgThroughput from the output
    avg_response_time=$(echo "$output" | awk '/Average Response Time/{print $4}')
    avg_throughput=$(echo "$output" | awk '/Overall Throughput/{print $3}')

    # Append results to CSV file
    echo "$clients,$avg_response_time,$avg_throughput" >> "$results_csv"

    echo "----------------------------------------"
done
python3 plot.py
echo "The Results are saved."
