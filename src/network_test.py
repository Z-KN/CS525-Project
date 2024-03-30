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
             "4": {"ip": "172.22.151.210", "port": "5564"},
             "5": {"ip": "172.22.151.97", "port": "5563"},
             "6": {"ip": "172.22.153.6", "port": "5562"},
             "7": {"ip": "172.22.154.233", "port": "5562"},
             "8": {"ip": "172.22.151.211", "port": "5562"},
             "9": {"ip": "172.22.151.98", "port": "5562"},
             "10": {"ip": "172.22.153.7", "port": "5562"},
             }

log_file=open("log.txt", "w")
# print current time to log file
print(time.ctime(), file=log_file)
print(node_name, file=log_file)

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
print(self.check_committed_entry(), file=log_file)
print(time.ctime(), file=log_file)

# Stop all the nodes
self.stop()
