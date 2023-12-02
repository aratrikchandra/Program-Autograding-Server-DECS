killall -9 vmstat
kill -9 $( ps -ax | grep "/bin/bash ./mointorServer.sh" | head -n 1 | awk '{print $1}')
while [ $? -eq 0 ]; do
  kill -9 $( ps -ax | grep "/bin/bash ./mointorServer.sh" | head -n 1 | awk '{print $1}')
done

touch ./cpu_util_and_threads_stats.txt
m=$1
# Calculate avg of cpu util and active threads by reading from respective files
#cpu_utilization=$(cat ./cpu_utilizations/cpu_utilization_clients_$m.txt | awk 'BEGIN {sum=0} NR > 2 { sum += (100- $15); n++ } END { if (n > 0) printf "%.2f\n", sum / n }')

cpu_utilization=$(cat ./cpu_utilizations/cpu_utilization_clients_$m.txt | awk 'BEGIN {sum=0} NR > 2 { if((100-$15)>sum) sum=(100-$15) } END { printf "%.2f\n", sum }')

avg_threads=$(cat ./active_threads/NLWP_clients_$m.txt | awk 'BEGIN {sum=0} NR > 2 { sum += $1; n++ } END { if (n > 0) printf "%.2f\n", sum / n }')

echo "${m},${cpu_utilization},${avg_threads}" >> ./cpu_util_and_threads_stats.txt

echo "Finished experiment with m=$m"

rm -rf file* output* compiler* prog* runtime* request_status.csv
