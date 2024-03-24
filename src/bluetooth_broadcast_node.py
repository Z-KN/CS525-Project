# this is an abstraction that we need while testing this code, in the absence of real broadcasts

# the broadcast node takes an input from a node and sends it to all other nodes
# no power modeling at the moment

import argparse
import socket
from concurrent import futures
import threading

import utility_functions

BLUETOOTH_TX_PORT = 59031
BLUETOOTH_RX_PORT = 59021

parser=argparse.ArgumentParser()
parser.add_argument('-a', '--addresses', nargs='+', help='enter addresses of nodes to be broadcasted to', required=True)

args = parser.parse_args()
global BROADCAST_RECIPIENTS
BROADCAST_RECIPIENTS = args.addresses

MAX_BLUETOOTH_LISTENERS = len(BROADCAST_RECIPIENTS)

def main():
    bluetooth_tx_socket = utility_functions.bind_socket('0.0.0.0', BLUETOOTH_TX_PORT, socket.SOCK_DGRAM)
    bluetooth_rx_socket = utility_functions.bind_socket('0.0.0.0', BLUETOOTH_RX_PORT, socket.SOCK_DGRAM)
   
    bluetooth_listener(bluetooth_tx_socket, bluetooth_rx_socket)
    #listen_thread = threading.Thread(target=bluetooth_listener, args=(bluetooth_rx_socket, bluetooth_tx_socket))
    #listen_thread.start()
    #listen_thread.join()

def bluetooth_listener(bluetooth_tx_socket, bluetooth_rx_socket):
    bluetooth_listener_pool = futures.ThreadPoolExecutor(max_workers=MAX_BLUETOOTH_LISTENERS)
    while(1):
        tx_data, tx_address = bluetooth_rx_socket.recvfrom(1024)
        # this is a synchronous execution at the moment, i expect broadcasts to be must slower than returns
        bluetooth_listener_pool.submit(bluetooth_process_advertisement, tx_data, tx_address, bluetooth_tx_socket).result()


def bluetooth_process_advertisement(data, address, bluetooth_tx_socket):
    message = data.decode()
    with threading.Lock():
        for listener in BROADCAST_RECIPIENTS:
            # TODO: if the last node doesn't have an bound UDP socket at the right address, the system doesn't work for some reason
            # i think there is some race condition here that is a headache to fix
            # we could potentially just keep the sockets open
            if not listener.__eq__(address[0]):
                receiver_node_address = (listener, BLUETOOTH_TX_PORT)
                try:
                    bluetooth_tx_socket.connect(receiver_node_address)
                    bluetooth_tx_socket.send(message.encode())
                except OSError as e:
                    if e.errno == 111:
                        print(f"listener {listener} not active")
                    else:
                        raise

if __name__ == "__main__":
    main()

