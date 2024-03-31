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
             "02": {"ip": "172.22.153.5", "port": "5567"},
             "03": {"ip": "172.22.154.232", "port": "5567"},
             "04": {"ip": "172.22.151.210", "port": "5567"},
             "05": {"ip": "172.22.151.97", "port": "5567"},
             "06": {"ip": "172.22.153.6", "port": "5567"},
             "07": {"ip": "172.22.154.233", "port": "5567"},
             "08": {"ip": "172.22.151.211", "port": "5567"}#,
             #"09": {"ip": "172.22.151.98", "port": "5567"},
             #"10": {"ip": "172.22.153.7", "port": "5567"},
             #"11": {"ip": "172.22.154.234", "port": "5567"}, 
             #"12": {"ip": "172.22.151.212", "port": "5567"},
             #"13": {"ip": "172.22.151.99", "port": "5567"},
             #"14": {"ip": "172.22.153.8", "port": "5567"},
             #"15": {"ip": "172.22.154.235", "port": "5567"},
             #"16": {"ip": "172.22.151.213", "port": "5567"},
             #"17": {"ip": "172.22.151.100", "port": "5567"},
             #"18": {"ip": "172.22.153.9", "port": "5567"},
             #"19": {"ip": "172.22.154.236", "port": "5567"},
             #"20": {"ip": "172.22.151.214", "port": "5567"}
             }

log_file=open("log.txt", "a")
# print current time to log file
print(get_system_time(), file=log_file)
print(node_name, file=log_file)

# Start a few nodes
self = RaftNode(comm_dict, node_name)
self.start()

# Let a leader emerge
#time.sleep(2)

# Make some requests
self.client_request({'val': node_name})

#time.sleep(2)
last_entry = self.check_committed_entry
print(self.check_committed_entry(), file=log_file)
print(get_system_time(), file=log_file)
# Check and see what the most recent entry is
while(1):
    time.sleep(0.01)
    try:
        if self.check_committed_entry() is not last_entry:
            last_entry = self.check_committed_entry
            print(self.check_committed_entry(), file=log_file)
            print(get_system_time(), file=log_file)
    except KeyboardInterrupt:
        self.stop()
        break
    
