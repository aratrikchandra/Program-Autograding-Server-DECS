#!/bin/bash

rm -rf file* output* compiler* prog* runtime* request_status.csv

numClients=$1
for ((i=1; i<=$numClients; i++)); do

  {
    echo client created: $i
    ./client new localhost:8002 submission.c &
    
  }
done
wait
