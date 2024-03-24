#!/usr/bin/env python
import time
from raft import RaftNode
import argparse

parser=argparse.ArgumentParser()
parser.add_argument('-n', '--name', help='enter node name for raft', required=True)

args = parser.parse_args()
node_name = args.name

# Create the intercommunication json 
comm_dict = {"1": {"ip": "172.22.151.96", "port": "5567"}, 
             "2": {"ip": "172.22.153.5", "port": "5566"},
             "3": {"ip": "172.22.154.232", "port": "5565"},
             "4": {"ip": "172.22.151.210", "port": "5564"}}

print(node_name)

# Start a few nodes
self = RaftNode(comm_dict, node_name)
self.start()

# Let a leader emerge
time.sleep(15)

# Make some requests
if node_name.__eq__("2") or node_name.__eq__("3"):
    self.client_request({'val': 2})
else:
    self.client_request({'val': 3})

time.sleep(5)

# Check and see what the most recent entry is
print(self.check_committed_entry())

# Stop all the nodes
self.stop()
