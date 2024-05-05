import numpy as np
import matplotlib.pyplot as plt


convergence_times = np.zeros((12, 5))
convergence_std = np.zeros(12)
convergence_averages = np.zeros(12)
total = 0
for index, x in enumerate(x for x in range(3, 26, 2)):
    for i in range(1, 6):
        with open(f'logs/seed{i}-{x}nodes.out') as f:
            data = f.readlines()
            # print(data[-1])
            last_time = data[-1].split()[0]

            last_time = last_time[1:len(last_time)-1]
            convergence_times[index, i - 1] = float(last_time)
            # print(index, i - 1)
    # print(index, convergence_times[index])
    convergence_std = np.std(convergence_times, axis=1)
    mean = np.mean(convergence_times[index])
    convergence_averages[index] = mean

print(convergence_std)
plt.plot([3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25], convergence_averages)
# x ticks
plt.xticks([3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25])
plt.errorbar([3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25], convergence_averages, yerr=convergence_std, fmt='o')
plt.xlabel('Number of nodes')
plt.ylabel('Time to convergence (s)')
# plt.title('Time to convergence for different numbers of nodes')
# plt.show()
plt.tight_layout()
plt.savefig('local_group.png')