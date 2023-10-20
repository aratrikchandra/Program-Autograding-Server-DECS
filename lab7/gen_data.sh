#!/bin/bash

loopNum=$3
sleepTimeSeconds=$4
maxClients=$5
serverIPPort=$1
sourceCodeFile=$2

mkdir -p result
# Create files to store the results
echo "Clients,Throughput,ResponseTime" > "result/results.csv"
# Run the load test for varying number of clients
for ((numClients=1; numClients<=maxClients; numClients++))
do
    echo "Running load test with $numClients clients..."
    output=$(./loadtest.sh $serverIPPort $sourceCodeFile $loopNum $sleepTimeSeconds $numClients)
    throughput=$(echo "$output" | grep 'Overall throughput' | awk '{print $3}')
    response_time=$(echo "$output" | grep 'Average response time' | awk '{print $4}')
    echo "$numClients,$throughput,$response_time" >> "result/results.csv"
done

echo "Done. The results are in 'results.csv'"

python3 plot.py


