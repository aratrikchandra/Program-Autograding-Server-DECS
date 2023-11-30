#!/bin/bash

# Check if the correct number of arguments is provided

gcc -o gradingclient gradingclient.c
# Start the program and capture the response
start_time=$(date +%s)
response=$(./gradingclient new localhost:8080 ten.c)

# Extract the request ID from the response
request_id=$(echo "$response" | awk -F ': ' '/Your RequestID is/{print $2}')
#echo "Request ID: $request_id"
# Check if request ID is obtained
if [ -z "$request_id" ]; then
    echo "Error: Failed to get Request ID"
    exit 1
fi
request_id=$(echo "$request_id" | awk '{gsub(/^ +| +$/,"")} {print}')
echo "Request ID: $request_id"

# Initialize start time
#start_time=$(date +%s)

# Check the status until "Processing is Done" is received
while true; do
    status=$(./gradingclient status localhost:8080 "$request_id")

    # Check if "Processing is Done" is received
    if [[ "$status" == "Your grading request ID $request_id processing is done, here are the results:Program ran successfull" ]]; then
        break
    fi

    # Sleep for a while before checking again (adjust the sleep time as needed)
    sleep 5
done

# Calculate response time
end_time=$(date +%s)
response_time=$((end_time - start_time))

echo "Processing is Done. Response Time: $response_time seconds"
