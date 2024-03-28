import sys
import os

# Get the absolute path of the parent directory
parent_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

# Add the parent directory to the Python path
sys.path.append(parent_dir)

# Now can import the raft module
import time
from node import Raft

# Create the intercommunication json 
ip_addr_1 = "172.22.151.96"
ip_addr_2 = "172.22.153.5"
ip_addr_3 = "172.22.154.232"
ip_addr_4 = "172.22.151.210"
comm_dict = {"node1": {"ip": ip_addr_1, "port": "5567"}, # what is the port???
             "node2": {"ip": ip_addr_2, "port": "5566"}, 
             "node3": {"ip": ip_addr_3, "port": "5566"}, 
             "node4": {"ip": ip_addr_4, "port": "5565"}}

# Start a few nodes
nodes = []
# for name, address in comm_dict.items():
#     nodes.append(RaftNode(comm_dict, name))
#     nodes[-1].start()

for i in range(4):
    nodes.append(Raft(i))
    nodes[-1].start()

# Let a leader emerge
time.sleep(2)

# Make some requests
for val in range(5):
    nodes[0].client_request({'val': val})
time.sleep(5)

# Check and see what the most recent entry is
for n in nodes:
    print(n.check_committed_entry())

# Stop all the nodes
for n in nodes:
    n.stop()