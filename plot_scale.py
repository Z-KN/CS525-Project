# plot a bar plot, using matplotlib
# 9 values in the x-axis, and the y-axis is the number of times each value was committed
import matplotlib.pyplot as plt
import numpy as np

# Values for the x-axis
x_0 = np.array([5, 8, 10, 13, 15, 18, 20])
# Values for the y-axis
y_0 = np.array([1.5024, 1.5696, 1.5441, 1.7375, 1.7951, 2.25600, 1.9053])
std_y_0 = np.array([0.0381, 0.0529, 0.0554, 0.2407, 0.1649, 0.5511, 0.0077])

y_3 = np.array([1.8045, 3.4394, 3.1651, 3.0758, 3.8311, 4.0524, 4.5773])
std_y_3 = np.array([0.3411, 1.3583, 0.8453, 0.5551, 1.3557, 1.2537, 0.9155])

y_5 = np.array([2.6671, 3.5783, 3.4726, 3.457, 4.2185, 4.3383, 3.9284])
std_y_5 = np.array([0.8287, 0.9695, 0.9056, 0.9221, 1.1717, 0.854, 0.8799])

# Plot the bar plot with error bar
# Plot x_0, y_3,y_5 in a bar plot, sharing the x_0 axis
bar_width = 0.4
plt.bar(x_0-bar_width, y_0, yerr=std_y_0, capsize=3, color='green', width=bar_width, label='lambda = 0')
plt.bar(x_0, y_3, yerr=std_y_3, capsize=3, color='red', width=bar_width, label='lambda = 0.3')
plt.bar(x_0+bar_width, y_5, yerr=std_y_5, capsize=3, color='blue', width=bar_width, label='lambda = 0.5')

# let x_0 = np.array([5, 8, 10, 13, 15, 18, 20]) be the ticks
plt.xticks(x_0, x_0)
# plt.errorbar(x_0, y_0, yerr=std_y_0, label='lambda = 0')
# plt.errorbar(x_0, y_3, yerr=std_y_3, label='lambda = 0.3')
# plt.errorbar(x_0, y_5, yerr=std_y_5, label='lambda = 0.5')
plt.xlabel('Number of nodes in the network')
# also plot the standard deviation
# plt.errorbar(x, y, yerr=std_y, fmt='o')
plt.ylabel('Time to consensus (s)')
plt.legend(loc='upper left', prop={'size': 9})
# plt.title('Number of times each node committed')
plt.tight_layout()
plt.savefig('consensus-time.png')
plt.show()
