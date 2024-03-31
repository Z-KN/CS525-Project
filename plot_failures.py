# plot a bar plot, using matplotlib
# 9 values in the x-axis, and the y-axis is the number of times each value was committed
import matplotlib.pyplot as plt
import numpy as np

# Values for the x-axis
x_0 = np.array([5, 8, 10, 13, 15, 18, 20])
# Values for the y-axis
y_0 = np.array([0, 0, 0, 0, 0, 0, 0])

y_3 = np.array([.1, .5, 0, .4, .4, .3, .2])

y_5 = np.array([.2, .2, .3, .6, .3, .3, .8])

# Plot the bar plot with error bar

bar_width = 0.4
plt.bar(x_0-bar_width, y_0, width=bar_width, color='green', label='lambda = 0')
plt.bar(x_0, y_3, width=bar_width, color='red', label='lambda = 0.3')
plt.bar(x_0+bar_width, y_5, width=bar_width, color='blue', label='lambda = 0.5')

plt.xticks(x_0, x_0)
# plt.bar(x_0, y_0, label='lambda = 0')
# plt.bar(x_0, y_3, label='lambda = 0.3')
# plt.bar(x_0, y_5, label='lambda = 0.5')
plt.xlabel('Number of nodes in the network')
# also plot the standard deviation
# plt.errorbar(x, y, yerr=std_y, fmt='o')
plt.ylabel('% Failure to agree after 2N entries')
plt.legend(loc='upper left')
# plt.title('Number of times each node committed')
plt.tight_layout()
plt.savefig('failure-rate.png')
plt.show()
