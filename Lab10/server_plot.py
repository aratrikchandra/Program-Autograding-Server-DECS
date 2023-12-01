import matplotlib.pyplot as plt
import pandas as pd

# Load data from the file
df = pd.read_csv("cpu_util_and_threads_stats.txt")

# Extract columns
clients = df['no of clients']
cpu_utilization = df['avg cpu utilization']
avg_active_threads = df['avg active threads']

# Plot Avg_Number_of_Threads_vs_Clients
plt.figure(figsize=(10, 6))
plt.plot(clients, avg_active_threads, marker='o', label='Avg Number of Threads')
plt.title('Average Number of Threads vs Clients')
plt.xlabel('Number of Clients')
plt.ylabel('Average Number of Threads')
plt.legend()
plt.grid(True)
plt.savefig('Avg_Number_of_Threads_vs_Clients.png')

# Plot Cpu_Utilization_%_vs_Clients
plt.figure(figsize=(10, 6))
plt.plot(clients, cpu_utilization, marker='o', label='CPU Utilization (%)')
plt.title('CPU Utilization vs Clients')
plt.xlabel('Number of Clients')
plt.ylabel('Average CPU Utilization (%)')
plt.legend()
plt.grid(True)
plt.savefig('Cpu_Utilization_%_vs_Clients.png')