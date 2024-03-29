#!/usr/bin/env python
import time
from raft import RaftNode

ip_addr = "127.0.0.1"
# Create the intercommunication json 
comm_dict = {"node0": {"ip": ip_addr, "port": "5567"}, 
             "node1": {"ip": ip_addr, "port": "5566"}, 
             "node2": {"ip": ip_addr, "port": "5565"},
             "node3": {"ip": ip_addr, "port": "5564"}}

# Start a few nodes
nodes = []
for name, address in comm_dict.items():
    nodes.append(RaftNode(comm_dict, name))
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
