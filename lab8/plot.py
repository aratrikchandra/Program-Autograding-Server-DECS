import matplotlib.pyplot as plt
import pandas as pd

# Load data from the CSV file
df = pd.read_csv("loadtest_results.csv")

# Extract columns
num_clients = df['NumClients']
avg_response_time = df['AvgResponseTime']
avg_throughput = df['AvgThroughput']

# Plot Avg_Response_Time_vs_Clients
plt.figure(figsize=(10, 6))
plt.plot(num_clients, avg_response_time, marker='o', label='Avg Response Time')
plt.title('Average Response Time vs Number of Clients')
plt.xlabel('Number of Clients')
plt.ylabel('Average Response Time (seconds)')
plt.legend()
plt.grid(True)
plt.savefig('Avg_Response_Time_vs_Clients.png')

# Plot Overall_Throughput_vs_Clients
plt.figure(figsize=(10, 6))
plt.plot(num_clients, avg_throughput, marker='o', label='Overall Throughput')
plt.title('Overall Throughput vs Number of Clients')
plt.xlabel('Number of Clients')
plt.ylabel('Overall Throughput (Requests per second)')
plt.legend()
plt.grid(True)
plt.savefig('Overall_Throughput_vs_Clients.png')
