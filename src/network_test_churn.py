#!/usr/bin/env python
import sys
import time
from raft import RaftNode
import argparse
import random
import threading

parser=argparse.ArgumentParser()
parser.add_argument('-n', '--name', help='enter node name for raft', required=True)
parser.add_argument('-p', '--probability', help='enter probablity for churning out, default 0.005', required=False, default=0.005, type=float)

args = parser.parse_args()
node_name = args.name
probability = args.probability

def get_system_time():
    if sys.version_info[0]==3 and sys.version_info[1] < 8:
        return time.perf_counter() * 1e9
    else:
        return time.perf_counter_ns()

# Create the intercommunication json 
comm_dict = {"1": {"ip": "172.22.151.96", "port": "5567"}, 
             "2": {"ip": "172.22.153.5", "port": "5566"},
             "3": {"ip": "172.22.154.232", "port": "5565"},
             "4": {"ip": "172.22.151.210", "port": "5564"}}#,
             #"5": {"ip": "172.22.151.97", "port": "5563"},
             #"6": {"ip": "172.22.153.6", "port": "5562"},
             #"7": {"ip": "172.22.154.233", "port": "5562"},
             #"8": {"ip": "172.22.151.211", "port": "5562"},
             #"9": {"ip": "172.22.151.98", "port": "5562"},
             #"10": {"ip": "172.22.153.7", "port": "5562"},
             #}

log_file=open("log.txt", "w")
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
print(get_system_time()/1e9, file=log_file)
# Check and see what the most recent entry is
while(1):
    try:
        if self.check_committed_entry() is not last_entry:
            last_entry = self.check_committed_entry
            print(self.check_committed_entry(), file=log_file)
            print(get_system_time()/1e9, file=log_file)
        time.sleep(0.01)
        # with some possibility of churning out
        churn = random.uniform(0,1)
        print("Churn: ", churn, probability)
        if churn < probability:
            self.pause()
            time.sleep(0.1)
            self.un_pause()
    except KeyboardInterrupt:
        self.stop()
        print("KeyboardInterrupt", file=log_file)
        break

# Stop all the nodes
