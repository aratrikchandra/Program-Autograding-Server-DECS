#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <sourceCodeFileTobeGraded> <sleepTimeBeforeStatusCheck>"
    exit 1
fi

source_code="$1"
sleep_time="$2"

gcc -o gradingclient gradingclient.c

# Start the program and capture the response
response=$(./gradingclient new localhost:8080 "$source_code")
start_time=$(date +%s)

# Extract the request ID from the response
request_id=$(echo "$response" | awk -F ': ' '/Your RequestID is/{print $2}')

# Check if request ID is obtained
if [ -z "$request_id" ]; then
    echo "Error: Failed to get Request ID"
    exit 1
fi

request_id=$(echo "$request_id" | awk '{gsub(/^ +| +$/,"")} {print}')
echo "Request ID: $request_id"

# Check the status until "Processing is Done" is received
while true; do
    status=$(./gradingclient status localhost:8080 "$request_id" | tr -d '\0')

    # Check if "Processing is Done" is received
    if echo "$status" | grep -q "Your grading request ID $request_id processing is done, here are the results"; then
        break
    fi

    # Sleep for a while before checking again (adjust the sleep time as needed)
    sleep "$sleep_time"
done

# Calculate response time
end_time=$(date +%s)
response_time=$((end_time - start_time))

# Format throughput with leading zero
throughput=$(printf "%.2f" "$(bc -l <<< "1 / $response_time")")
echo "Response Time $response_time seconds"
echo "Throughput $throughput req/sec"
