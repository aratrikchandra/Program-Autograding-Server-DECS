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

#Plot Goodput vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['Goodput'], marker='o', color='g')
plt.title('Goodput vs Clients')
plt.xlabel('Clients')
plt.ylabel('Goodput')
plt.grid(True)
plt.savefig('result/Goodput_vs_Clients.pdf')

#Plot TimeoutRate vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['TimeoutRate'], marker='o', color='b')
plt.title('TimeoutRate vs Clients')
plt.xlabel('Clients')
plt.ylabel('TimeoutRate')
plt.grid(True)
plt.savefig('result/TimeoutRate_vs_Clients.pdf')

#Plot ErrorRate vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['ErrorRate'], marker='o', color='r')
plt.title('ErrorRate vs Clients')
plt.xlabel('Clients')
plt.ylabel('ErrorRate')
plt.grid(True)
plt.savefig('result/ErrorRate_vs_Clients.pdf')

# Plot RequestSentRate vs Clients
plt.figure(figsize=(10, 5))
plt.plot(data['Clients'], data['RequestSentRate'], marker='o', color='g')
plt.title('RequestSentRate vs Clients')
plt.xlabel('Clients')
plt.ylabel('RequestSentRate')
plt.grid(True)
plt.savefig('result/RequestSentRate_vs_Clients.pdf')

print("The plots have been saved as 'results' directory")

