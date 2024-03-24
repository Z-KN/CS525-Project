# edge.py

import consensusobject
import commitlog
import socket
import threading
from concurrent import futures

import utility_functions

EDGE_ADVERTISEMENT_INTERVAL = 1e9
EDGE_HEARTBEAT_TIMER = 10*EDGE_ADVERTISEMENT_INTERVAL

BROADCAST_NODE_ADDRESS = '172.22.154.236'

known_nodes = dict()

BROADCAST_TX_PORT = 63031

UNICAST_TX_PORT = 61031
UNICAST_RX_PORT = 61021

NETWORK_INTERFACE = "ens33"

def main(): 
    broadcast_tx_socket = utility_functions.bind_socket('0.0.0.0', BROADCAST_TX_PORT, socket.SOCK_DGRAM)

    unicast_tx_socket = utility_functions.bind_socket('0.0.0.0', UNICAST_TX_PORT, socket.SOCK_STREAM)
    unicast_rx_socket = utility_functions.bind_socket('0.0.0.0', UNICAST_RX_PORT, socket.SOCK_STREAM)
    unicast_rx_socket.listen()
    
    listen_thread = threading.Thread(target=parse_advertisement_response, args=(unicast_tx_socket, unicast_rx_socket))
    listen_thread.start()
 
    systime = utility_functions.get_system_time()
    while(1):
        if utility_functions.get_system_time() - systime >= EDGE_ADVERTISEMENT_INTERVAL:
            with threading.Lock():
                systime = utility_functions.get_system_time()
                broadcast_advertisement(broadcast_tx_socket)

                keylist = list(known_nodes)[:]
                print(keylist)
                for key in keylist:
                    if systime - known_nodes[key][1] >= EDGE_HEARTBEAT_TIMER:
                        del known_nodes[key]

def broadcast_advertisement(broadcast_tx_socket):
    """
    broadcast_advertisement
        the edge server broadcasts its existence to any nearby nodes
    """
    broadcast_node_address = (BROADCAST_NODE_ADDRESS, BROADCAST_TX_PORT)  
    broadcast_tx_socket.connect(broadcast_node_address)
    address = utility_functions.get_address_on_interface(NETWORK_INTERFACE)
    message = f"{address}"
    broadcast_tx_socket.send(message.encode())

def parse_advertisement_response(node_tx_socket, node_rx_socket):
    """
    parse_advertisement_response
        add a responsive node to the membership list
    """
    #arbitrary number of max workers
    node_listener_pool = futures.ThreadPoolExecutor(max_workers=10)
    while(1):
        # accept a new connection
        connection, address = node_rx_socket.accept()

        print(f"connected to {address}")
        node_listener_pool.submit(maintain_existing_connection, connection, address)

def maintain_existing_connection(connection, address):
    while(1):
        message = connection.recv(1024).decode().split(",")
        with threading.Lock():
            print(message)
            known_nodes[message[1]] = (message[0], utility_functions.get_system_time())


def reply_to_user():
    """
    reply_to_user

        if a user requests information about a certain feature, the edge server looks at the id of the feature they request and transmit data back 
    """

# the edge server will eventually get information from user feature creation "trickling down" from the server back to the edge
# but we may be able to make this quicker by having nodes directly communicate their raft logs to the edge server
# users have their raft logs which persists state--maybe uploading these to the edge server in a smart way can help reconcile state?
def update_state():
    """
    update_state

        if a user submits a request to create a new feature, the edge server can add it
        potentially the user can log features theyve added in the past but not committed, and once in range of an edge server all of those features can be "logged" with the edge server
    """

if __name__ == "__main__":
    main()
