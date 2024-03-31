# plot a bar plot, using matplotlib
# 9 values in the x-axis, and the y-axis is the number of times each value was committed
import matplotlib.pyplot as plt
import numpy as np

# Values for the x-axis
x = np.array(['5', '8', '10', '13', '15', '18', '20'])
# Values for the y-axis
y = np.array([1.5024, 1.5696, 1.5441, 1.7375, 1.7951, 2.25600, 1.9053])
std_y = np.array([0.0381, 0.0529, 0.0554, 0.2407, 0.1649, 0.5511, 0.0077])

# Plot the bar plot with error bar

plt.errorbar(x, y, yerr=std_y)
plt.xlabel('Number of nodes in the network')
# also plot the standard deviation
# plt.errorbar(x, y, yerr=std_y, fmt='o')
plt.ylabel('Time to consensus (s)')
# plt.title('Number of times each node committed')
plt.savefig('consensus-time.png')
plt.show()
