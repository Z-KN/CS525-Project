# node.py

import argparse
import socket
import threading
from concurrent import futures
import time
import pathlib
import random
import os
import sys
import psutil
from enum import Enum

import commitlog
import consensusobject

import utility_functions
from ..raft import RaftNode

# each device needs a way to write and read application state
#   functions for writing application state
#       update state
#       update peers

#   functions for reading state
#       local agreement (includes local consensus and local agreement)


# for raft, we need a pretty big mean time between failures. for that reason we can try to avoid churn by getting only bluetooth-nearby nodes to initiate
# a process that is followed up with something more resilient.
# we will also need a two-phase election process, once we see how often churn can happen
# we can also develop snapshotting in raft

# we are trying to reach consensus on one byte
# nodes set individual bits by index
# i think these operations should be atomic, but here I make the locking explicit
# i think the best data structure to test on would be a graph, but we can expand this as necessary and refactor
class RaftState(Enum):
    FOLLOWER = 0
    CANDIDATE = 1
    LEADER = 2

MAX_BLUETOOTH_LISTENERS = 2
MAX_HIGH_POWER_LISTENERS = 2

# bluetooth allows intervals between 20 ms and about 10 seconds in 625 us increments.
# its recommend to use the longest interval possible
BLUETOOTH_ADVERTISEMENT_INTERVAL = 5e8 # in nanoseconds
BLUETOOTH_HEARTBEAT_TIMER = 10*BLUETOOTH_ADVERTISEMENT_INTERVAL

EDGE_ADVERTISEMENT_INTERVAL = 1e9
EDGE_HEARTBEAT_TIMER = 10*EDGE_ADVERTISEMENT_INTERVAL

# this is for the broadcast node abstraction
BROADCAST_NODE_ADDRESS = '172.22.151.214'

# bluetooth ports (meant to be a separate communication channel)
BLUETOOTH_TX_PORT = 59021
BLUETOOTH_RX_PORT = 59031

# edge server broadcast
EDGE_BROADCAST_RX_PORT = 63031

# edge server ports (meant to be a separate communication channel)
EDGE_UNICAST_TX_PORT = 61021
EDGE_UNICAST_RX_PORT = 61031

global edge_server_address_time
edge_server_address_time = (None, None)

# consensus ports (meant to be a separate communication channel)
HIGH_POWER_TX_PORT = 60021
HIGH_POWER_RX_PORT = 60031

parser=argparse.ArgumentParser()
parser.add_argument('-n', '--name', help='enter integer for node index', required=True)

args = parser.parse_args()
NODE_INDEX = int(args.name)

NETWORK_INTERFACE = "ens33"

# RAFT PARAMETERS
class Raft(RaftNode):
    def __init__(self, node_id):
        # all nodes start as followers
        self.state = RaftState.FOLLOWER

        # from paper page 4
        # PERSISTENT STATE
        # the current election term, and the id of the node voted for. right now ids are assigned at runtime, but these can be mac addresses
        self.current_term = 1
        # initialize Commit Log
        self.commit_log = commitlog.CommitLog(file=f"commit-log-{node_id}.txt")
        commit_logfile = pathlib.Path(self.commit_log.file)
        commit_logfile.touch(exist_ok=True)

        # id of current leader, useful in case this node hears of another election but knows of a valid leader
        # it can also pass its unicast information
        self.leader_id = -1

        # VOLATILE STATE
        # most recent state change in log
        self.commit_index = 0
        # last applied point in log
        self.last_applied = 0
 
        # STATE FOR LEADERS/CANDIDATES
        # total set of members, collected by bluetooth advertisements
        # for each member, their entries should contain nextIndex and matchIndex information
        # also should contain a marker on whether or not they voted for this node, if it's a candidate
        self.local_group = dict()

        # STATE FOR FOLLOWERS 
        # id of node that this follower voted for
        self.voted_for = -1
        self.election_timeout = -1

        # OTHER PARAMETERS
        # nodes have random election timeouts in order to avoid simultaneous elections. these parameters will have to be tuned
        self.election_period_ms = random.randint(1000, 5000)
        # because the nodes are not unicasting, we do a probabilistic broadcast to avoid interference as mentioned in one of our references
        # right now its much faster than the advertisement interval
        self.broadcast_timer = random.randint(50, 100)

    # if you want to begin consensus, unicast a message to all peers in your local group. then set your state to a follower
    # if you receive a consensus message, unicast a message to all peers in your local group containing information about the peers you're already connected to. then set your state to a follower
    # either way, begin this program
    def begin_consensus(self):
        # elapse your timer
        # if your timer elapses first, then start an election with request_vote
        # if you hear that another node started an election, run handle_request_vote
        return

    # invoked by leader to replicate log entries
    # doesn't need to function as a heartbeat, since the bluetooth performs that same role
    def append_entries(self):
        return

    def handle_append_entries(self):
        return

    def request_vote(self):
        # send your current term
        # your candidate id
        # the index of your last log entry
        # and the term of your last log entry
        return

    def handle_request_vote(self):
        # if the term in request_vote is less than the actual term, let that node know
        # otherwise, if you haven't already voted for this term, and their log is up to date, give them a vote
        return


def main():
    """
    main
        starts threads that listen and advertise on the bluetooth low energy channel
        binds device to its IP address and sets it to listen on a higher-power channel
    """
    global edge_server_address_time
    # socket to communicate to other nodes
    bluetooth_tx_socket = utility_functions.bind_socket('0.0.0.0', BLUETOOTH_TX_PORT, socket.SOCK_DGRAM)
    bluetooth_rx_socket = utility_functions.bind_socket('0.0.0.0', BLUETOOTH_RX_PORT, socket.SOCK_DGRAM)

    # socket to reach consensus with other nodes
    high_power_tx_socket = utility_functions.bind_socket('0.0.0.0', HIGH_POWER_TX_PORT, socket.SOCK_STREAM)
    high_power_rx_socket = utility_functions.bind_socket('0.0.0.0', HIGH_POWER_RX_PORT, socket.SOCK_STREAM)

    # socket to unicast to edge server
    edge_unicast_tx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_UNICAST_TX_PORT, socket.SOCK_STREAM)
    edge_unicast_rx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_UNICAST_RX_PORT, socket.SOCK_STREAM)
    
    # socket to receive broadcasts from edge server
    edge_broadcast_rx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_BROADCAST_RX_PORT, socket.SOCK_DGRAM)

    raft = Raft(NODE_INDEX)

    bluetooth_listen_thread = threading.Thread(target=bluetooth_listener, args=(bluetooth_rx_socket,raft))
    bluetooth_listen_thread.start()

    edge_broadcast_listen_thread = threading.Thread(target=edge_broadcast_listener, args=(edge_broadcast_rx_socket, edge_unicast_tx_socket))
    edge_broadcast_listen_thread.start()

    systime = utility_functions.get_system_time()
    while(1):
        if utility_functions.get_system_time() - systime >= BLUETOOTH_ADVERTISEMENT_INTERVAL:
            with threading.Lock():
                systime = utility_functions.get_system_time()
                bluetooth_sender(bluetooth_tx_socket)
                
                print(edge_server_address_time)
                keylist = list(raft.local_group.keys())[:]
                print(keylist)
                if edge_server_address_time[1] is not None:
                    if systime - edge_server_address_time[1] >= EDGE_HEARTBEAT_TIMER:
                        edge_server_address_time = (None, None)
                for key in keylist:
                    if systime - raft.local_group[key][1] >= BLUETOOTH_HEARTBEAT_TIMER:
                        del raft.local_group[key]

"""
    BLUETOOTH FUNCTIONS
"""
def bluetooth_listener(bluetooth_rx_socket, raft):
    """
    bluetooth_listener

        listen for bluetooth low energy advertisements from other devices on the appropriate channel
            people have implemented bluetooth low energy for ns3: https://gitlab.com/Stijng/ns3-ble-module/-/tree/master/ble/examples?ref_type=heads
            but we can start by modeling a low throughput, high fading channel in ns3 instead of implementing bluetooth-specific work 
        these advertisement we can use are defined as follows (taken from https://www.bluetooth.com/bluetooth-resources/intro-to-bluetooth-advertisements/):
            
        these advertisements will communicate which nodes are nearby, and therefore the nodes that actually need to reach consensus
            we can maintain a small membership list in memory, consisting of MAC addresses or some other identifier
    """

    # this doesn't have to be multithreaded when we're working with the dedicated broadcast node, but I choose to do it in case NS3 requires it
    bluetooth_listener_pool = futures.ThreadPoolExecutor(max_workers=MAX_BLUETOOTH_LISTENERS)
    while(1):
        tx_data, tx_address = bluetooth_rx_socket.recvfrom(1024)
        # block until this finishes running
        futures.wait([bluetooth_listener_pool.submit(bluetooth_process_advertisement, tx_data, tx_address, raft)])

def bluetooth_process_advertisement(data, address, raft):
    with threading.Lock():
        message = data.decode()
        [unicast_address, node_id] = message.split(",")
        raft.local_group[node_id] = (unicast_address, utility_functions.get_system_time())

"""
Advertisement Structure
| 4 byte IPv4 Address | 1 byte ID|

right now it's just strings, for simplicity
"""
def bluetooth_sender(bluetooth_tx_socket):
    """
    bluetooth_sender

        propagate bluetooth low energy advertisements on the appropriate channel
    """
    broadcast_node_address = (BROADCAST_NODE_ADDRESS, BLUETOOTH_TX_PORT)  
    bluetooth_tx_socket.connect(broadcast_node_address)
    address = utility_functions.get_address_on_interface(NETWORK_INTERFACE)
    message = f"{address},{NODE_INDEX}"
    bluetooth_tx_socket.send(message.encode())



"""
    EDGE FUNCTIONS
"""
def edge_broadcast_listener(edge_broadcast_rx_socket, edge_unicast_tx_socket):
    """
    edge_broadcast_listener
        this function is meant to run on a thread and listen for edge server broadcasts
        if an edge server is found, it updates the advertisements it sends out to its local group
    """
    edge_listener_pool = futures.ThreadPoolExecutor(max_workers=2)
    while(1):
        tx_data, tx_address = edge_broadcast_rx_socket.recvfrom(1024)
        print("received")
        unicast_address = edge_listener_pool.submit(edge_process_advertisement, tx_data, tx_address).result()
        edge_responder(unicast_address, edge_unicast_tx_socket)

# for now, this is working with the assumption that if you can advertise to a node via bluetooth, both of you have a good quality connection
# to the same edge server
def edge_process_advertisement(data, address):
    """
        edge_process_advertisement
            parse advertisement from edge server, propagate that to peers over bluetooth with heartbeat
    """
    with threading.Lock():
        message = data.decode()
        print(f"found edge at {message}")
        global edge_server_address_time
        edge_server_address_time = (message, utility_functions.get_system_time())
        return message

def edge_responder(edge_unicast_address, edge_unicast_tx_socket):
    """
        edge_responder
            respond to edge server advertisement
    """
    edge_unicast_address_port = (edge_unicast_address, EDGE_UNICAST_TX_PORT)
    # bandaid fix, this might flood the node with spurious setup requests
    try:
        edge_unicast_tx_socket.connect(edge_unicast_address_port)
    except OSError as e:
        if e.errno == 106:
            print("already connected")
        else:
            raise

    address = utility_functions.get_address_on_interface(NETWORK_INTERFACE)
    message = f"{address},{NODE_INDEX}"
    edge_unicast_tx_socket.send(message.encode())

def edge_update_state():
    """
    edge_update_state

        if a user creates a feature in a multi-user environment, it needs to update the edge about that feature. if an edge server is not available, then it needs to log the creation of that feature and add coarse-grained information about that feature to the cloud
        this way, if other users want to interact with that feature, it will still be available to them
        once a feature is logged with nearby edge servers, it can be associated with spatial SLAM mapping data that is more computationally intensive and localized

    """



"""
    CONSENSUS FUNCTIONS
"""
def local_agreement(raft):
    """
    local_agreement

        from this function we can move either to local_consensus or local_dissemination depending on the rules below. if we see issues regarding getting all nodes to run either local_consensus or local_dissemination, then we can try gossiping that decision over bluetooth

        first, check timestamp of last edge server heartbeat. if no such heartbeat exists, or the last edge server heartbeat is above some threshold, thenwe don't know of an edge server. due to the high locality between nodes, we assume that if we don't know of an edge server, our peers don't know of an edge server. we will eventually need to refine how this is handled. maybe if one node gets a value from an edge server, and another node gets a result from an edge server, then the edge-capable node has a modified proposal
            nevertheless, for now, we move into local_consensus

        if the heartbeat is active, then we know of an edge server. if we know of an alive edge server, then our local group likely does as well. we can move to local_dissemination
    """

def local_consensus(raft):
    """
    local_consensus

        we can use peer-to-peer data channels that exist (https://developer.mozilla.org/en-US/docs/Games/Techniques/WebRTC_data_channels) to figure out how we want to model these channels, compared to the bluetooth channels. my idea is that it will probably be higher power and have further range, because consensus involving these channels should be invoked less frequently.
        we can justify this by saying that our big use case (SLAM consensus) can use Kalman filtering or other non-network tracking systems and still maintain good service, according to the Edge-SLAM, etc papers we have available

        over this channel, we can use RAFT. we can try to use a churn-tolerant leader election sceheme, but I think we can analyze whether there's a lot of churn first. I have a suspicion we won't have a lot, because even if users are "churning" in and out of the local group, they will still be reachable by a higher-power channel. they can then reject participation explicitly, or time out. 
    """
    raft_thread = threading.Thread(target=raft.begin_consensus, args=(raft,))
    raft_thread.start()

    
def local_dissemination(raft):
    """
    local_dissemination

        here, the node spawns a request to get state from the edge server
    """

def update_peers():
    """
    update_peers

        if a user creates a feature in a multi-user environment, and that user also has other users nearby, it needs to authoritatively indicate to its local group information about the feature. if no edge server is available, it is not sufficient to wait for that feature to finish uploading to the internet and reach some consistent state to be visible to the users. instead, the creating user can immediately pass information to their local group. this is not consensus because it is authoritatively handled by the feature-creating user
    """

if __name__ == "__main__":
    main()
