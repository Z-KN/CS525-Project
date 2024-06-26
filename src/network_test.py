#!/usr/bin/env python
import sys
import time
from raft import RaftNode
import argparse
import random
import threading

parser=argparse.ArgumentParser()
parser.add_argument('-n', '--name', help='enter node name for raft', required=True)

args = parser.parse_args()
node_name = args.name

def get_system_time():
    if sys.version_info[0]==3 and sys.version_info[1] < 8:
        return time.perf_counter() * 1e9
    else:
        return time.perf_counter_ns()

# Create the intercommunication json 
comm_dict = {"01": {"ip": "172.22.151.96", "port": "5567"}, 
             "02": {"ip": "172.22.153.5", "port": "5566"},
             "03": {"ip": "172.22.154.232", "port": "5565"},
             "04": {"ip": "172.22.151.210", "port": "5564"}}#,
             #"5": {"ip": "172.22.151.97", "port": "5563"},
             #"6": {"ip": "172.22.153.6", "port": "5562"},
             #"7": {"ip": "172.22.154.233", "port": "5562"},
             #"8": {"ip": "172.22.151.211", "port": "5562"},
             #"9": {"ip": "172.22.151.98", "port": "5562"},
             #"10": {"ip": "172.22.153.7", "port": "5562"},
             #}

log_file=open("log.txt", "a")
# print current time to log file
print(get_system_time(), file=log_file)
print(node_name, file=log_file)

# Start a few nodes
self = RaftNode(comm_dict, node_name)
self.start()

# Let a leader emerge
time.sleep(2)

# Make some requests
self.client_request({'val': node_name})

#time.sleep(2)
time.sleep(5)

print(self.check_committed_entry(), file=log_file)
print(get_system_time(), file=log_file)

# Stop all the nodes
self.stop()
