import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
data = pd.read_csv('result/results.csv')

# Plot Throughput vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['Throughput'], marker='o')
plt.title('Throughput vs Clients')
plt.xlabel('Clients')
plt.ylabel('Throughput')
plt.grid(True)
plt.savefig('result/Throughput_vs_Clients.pdf')

# Plot Response Time vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['ResponseTime'], marker='o', color='r')
plt.title('Response Time vs Clients')
plt.xlabel('Clients')
plt.ylabel('Response Time')
plt.grid(True)
plt.savefig('result/ResponseTime_vs_Clients.pdf')

print("The plots have been saved as 'Throughput_vs_Clients.pdf' and 'ResponseTime_vs_Clients.pdf'.")

