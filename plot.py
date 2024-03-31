# plot a bar plot, using matplotlib
# 9 values in the x-axis, and the y-axis is the number of times each value was committed
import matplotlib.pyplot as plt
import numpy as np

# Values for the x-axis
x = np.array(['2', '3', '4', '5', '6', '7', '8', '9', '10'])
# Values for the y-axis
y = np.array([2, 3, 5, 7, 8, 9, 10, 11, 12])
std_y = np.array([0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0])

# Plot the bar plot with error bar

plt.bar(x, y, yerr=std_y, capsize=7, color='green')
plt.xlabel('Number of nodes in the network')
# also plot the standard deviation
# plt.errorbar(x, y, yerr=std_y, fmt='o')
plt.ylabel('Time to consensus (s)')
# plt.title('Number of times each node committed')
plt.savefig('consensus-time.png')
plt.show()
