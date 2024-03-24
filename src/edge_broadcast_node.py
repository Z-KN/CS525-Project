# this is an abstraction that we need while testing this code, in the absence of real broadcasts

# the broadcast node takes an input from a node and sends it to all other nodes
# no power modeling at the moment

import argparse
import socket
from concurrent import futures
import threading

import utility_functions

NODE_TX_PORT = 63031
EDGE_RX_PORT = 63031

parser=argparse.ArgumentParser()
parser.add_argument('-e', '--edge', help='enter address of edge server', required=True)
parser.add_argument('-a', '--addresses', nargs='+', help='enter addresses of nodes to be broadcasted to', required=True)

args = parser.parse_args()
EDGE_SERVER = args.edge
BROADCAST_RECIPIENTS = args.addresses

MAX_EDGE_LISTENERS = len(BROADCAST_RECIPIENTS)

def main():
    node_tx_socket = utility_functions.bind_socket('0.0.0.0', NODE_TX_PORT, socket.SOCK_DGRAM)
    edge_rx_socket = utility_functions.bind_socket('0.0.0.0', EDGE_RX_PORT, socket.SOCK_DGRAM)
    
    edge_listener(node_tx_socket, edge_rx_socket)
    #listen_thread = threading.Thread(target=edge_listener, args=(node_tx_socket, edge_rx_socket))
    #listen_thread.start()
    #listen_thread.join()

def edge_listener(node_tx_socket, edge_rx_socket):
    edge_listener_pool = futures.ThreadPoolExecutor(max_workers=MAX_EDGE_LISTENERS)
    while(1):
        tx_data, tx_address = edge_rx_socket.recvfrom(1024)
        # this is a synchronous execution at the moment, i expect broadcasts to be must slower than returns
        edge_listener_pool.submit(edge_process_advertisement, tx_data, tx_address, node_tx_socket).result()

def edge_process_advertisement(data, address, node_tx_socket):
    message = data.decode()
    with threading.Lock():
        for listener in BROADCAST_RECIPIENTS:
            # TODO: if the last node doesn't have an bound UDP socket at the right address, the system doesn't work for some reason
            # i think there is some race condition here that is a headache to fix
            # we could potentially just keep the sockets open
            if not listener.__eq__(address[0]):
                receiver_node_address = (listener, NODE_TX_PORT)
                try:
                    node_tx_socket.connect(receiver_node_address)
                    node_tx_socket.send(message.encode())
                except OSError as e:
                    if e.errno == 111:
                        print(f"listener {listener} not active")
                    else:
                        raise

if __name__ == "__main__":
    main()

