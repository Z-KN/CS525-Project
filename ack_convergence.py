import numpy as np
import matplotlib.pyplot as plt


convergence_times = np.zeros((9, 5))
convergence_std = np.zeros(9)
convergence_averages = np.zeros(9)
total = 0
for index, x in enumerate(x for x in range(3, 20, 2)):
    for i in range(1, 6):
        with open(f'ack_convergence_logs/seed{i}-{x}nodes.out') as f:
            data = f.readlines()
            last_time = data[-(x + 1)].split()[0]

            last_time = last_time[1:len(last_time)-1]
            convergence_times[index, i - 1] = float(last_time)
    convergence_std = np.std(convergence_times, axis=1)
    mean = np.mean(convergence_times[index])
    convergence_averages[index] = mean

nodes = [3, 5, 7, 9, 11, 13, 15, 17, 19]

number_of_acks = np.zeros(9)
acks_std = np.zeros(9)



# COUNT NUMBER OF ACKS
for index, x in enumerate(y for y in range(3, 20, 2)):
    for i in range(1, 6):
        with open(f'ack_convergence_logs/seed{i}-{x}nodes.out') as f:
            data = f.readlines()
            total = 0.0
            acks = []
            for y in range(x):
                line = data[-(y + 1)].split()[-1]
                total += float(line)
                acks.append(float(line))
            acks_std[index] = np.std(acks)
            number_of_acks[index] = total/x
fig, ax1 = plt.subplots()
# Std deviation error bars here
# plt.errorbar([3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25], convergence_averages, yerr=convergence_std, fmt='o')
plt.title('Time to ACK convergence/Number of ACKs for different numbers of nodes')

color = 'tab:red'
ax1.set_xlabel('Number of nodes')
ax1.set_ylabel('Time to convergence (s)', color=color)
ax1.plot(nodes, convergence_averages, color=color)
ax1.errorbar(nodes, convergence_averages, yerr=convergence_std, fmt='o', color=color)
ax1.tick_params(axis='y', labelcolor=color)

ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis

color = 'tab:blue'
ax2.set_ylabel('Number of ACKS', color=color)  # we already handled the x-label with ax1
ax2.errorbar(nodes, number_of_acks, yerr=acks_std, fmt='o', color=color)
ax2.plot(nodes, number_of_acks, color=color)
ax2.tick_params(axis='y', labelcolor=color)
fig.tight_layout() 
plt.show()
plt.savefig('ack_convergence.png')