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
from raft import RaftNode

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
# i think the best data structure to test on would be a graph, but we can expand this as necessary and refacto

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
    # high_power_tx_socket = utility_functions.bind_socket('0.0.0.0', HIGH_POWER_TX_PORT, socket.SOCK_STREAM)
    # high_power_rx_socket = utility_functions.bind_socket('0.0.0.0', HIGH_POWER_RX_PORT, socket.SOCK_STREAM)

    # socket to unicast to edge server
    edge_unicast_tx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_UNICAST_TX_PORT, socket.SOCK_STREAM)
    edge_unicast_rx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_UNICAST_RX_PORT, socket.SOCK_STREAM)
    
    # socket to receive broadcasts from edge server
    edge_broadcast_rx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_BROADCAST_RX_PORT, socket.SOCK_DGRAM)

    local_group = dict()

    bluetooth_listen_thread = threading.Thread(target=bluetooth_listener, args=(bluetooth_rx_socket,local_group))
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
                keylist = list(local_group.keys())[:]

                # if the length of the keylist is 3, run consensus
                # this is a BAD(!!) trigger but it shows that consensus will run
                # we need to improve this trigger so that we can choose when to run consensus
                if len(keylist) == 3:
                    local_consensus(local_group)

                print(keylist)
                if edge_server_address_time[1] is not None:
                    if systime - edge_server_address_time[1] >= EDGE_HEARTBEAT_TIMER:
                        edge_server_address_time = (None, None)
                for key in keylist:
                    if systime - local_group[key][1] >= BLUETOOTH_HEARTBEAT_TIMER:
                        del local_group[key]

"""
    BLUETOOTH FUNCTIONS
"""
def bluetooth_listener(bluetooth_rx_socket, local_group):
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
        futures.wait([bluetooth_listener_pool.submit(bluetooth_process_advertisement, tx_data, tx_address, local_group)])

def bluetooth_process_advertisement(data, address, local_group):
    with threading.Lock():
        message = data.decode()
        [unicast_address, node_id] = message.split(",")
        local_group[node_id] = (unicast_address, utility_functions.get_system_time())

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

def local_consensus(local_group):
    """
    local_consensus

        we can use peer-to-peer data channels that exist (https://developer.mozilla.org/en-US/docs/Games/Techniques/WebRTC_data_channels) to figure out how we want to model these channels, compared to the bluetooth channels. my idea is that it will probably be higher power and have further range, because consensus involving these channels should be invoked less frequently.
        we can justify this by saying that our big use case (SLAM consensus) can use Kalman filtering or other non-network tracking systems and still maintain good service, according to the Edge-SLAM, etc papers we have available

        over this channel, we can use RAFT. we can try to use a churn-tolerant leader election sceheme, but I think we can analyze whether there's a lot of churn first. I have a suspicion we won't have a lot, because even if users are "churning" in and out of the local group, they will still be reachable by a higher-power channel. they can then reject participation explicitly, or time out. 
    """
    comm_dict = dict()
    for key in local_group.keys():
        comm_dict[key] = {'ip': local_group[key][0], 'port': str(HIGH_POWER_TX_PORT)}

    comm_dict[str(NODE_INDEX)] = {'ip': utility_functions.get_address_on_interface(NETWORK_INTERFACE), 'port': str(HIGH_POWER_TX_PORT)} 

    self = RaftNode(comm_dict, str(NODE_INDEX))
    self.start()

    time.sleep(15)

    # Make some requests
    if str(NODE_INDEX).__eq__("2") or str(NODE_INDEX).__eq__("3"):
        self.client_request({'val': 2})
    else:
        self.client_request({'val': 3})

    time.sleep(5)

    # Check and see what the most recent entry is
    print(self.check_committed_entry())

    # Stop all the nodes
    self.stop()
    
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
