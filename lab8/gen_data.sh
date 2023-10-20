#!/bin/bash

loopNum=$3
sleepTimeSeconds=$4
maxClients=$6
serverIPPort=$1
sourceCodeFile=$2
timeout=$5

# Extract the port number from serverIPPort
portNumber=$(echo "$serverIPPort" | cut -d ':' -f 2)


mkdir -p result
# Create files to store the results
echo "Clients,Throughput,ResponseTime,Goodput,TimeoutRate,ErrorRate,RequestSentRate" > "result/results.csv"

# Run the load test for varying numbers of clients
for ((numClients=10; numClients<=maxClients; numClients+=10))
do
    echo "Running load test with $numClients clients..."

    # Start the load test
    output=$(./loadtest.sh $serverIPPort $sourceCodeFile $loopNum $sleepTimeSeconds $timeout $numClients)
	
    throughput=$(echo "$output" | grep 'Overall throughput' | awk '{print $3}')
    response_time=$(echo "$output" | grep 'Average response time' | awk '{print $4}')
    goodput=$(echo "$output" | grep 'Overall Successful request rate(goodput)' | awk '{print $5}')
    timeout=$(echo "$output" | grep 'Overall timeout rate' | awk '{print $4}')
    errorrate=$(echo "$output" | grep 'Overall error rate' | awk '{print $4}')
    sentrate=$(echo "$output" | grep 'Overall request sent rate' | awk '{print $5}')

    
    echo "$numClients,$throughput,$response_time,$goodput,$timeout,$errorrate,$sentrate" >> "result/results.csv"
    
    echo "Finished experiment with m=$numClients"
done

echo "All experiments finished"
echo "Done. The results are in 'results.csv'"

python3 plot.py
