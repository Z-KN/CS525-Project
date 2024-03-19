# this is an abstraction that we need while testing this code, in the absence of real broadcasts

# the broadcast node takes an input from a node and sends it to all other nodes
# no power modeling at the moment

import argparse
import socket
from concurrent import futures
import threading

global BLUETOOTH_PORT
BLUETOOTH_PORT = 9035

parser=argparse.ArgumentParser()
parser.add_argument('-a', '--addresses', nargs='+', help='enter addresses of nodes to be broadcasted to', required=True)

args = parser.parse_args()
global BROADCAST_RECIPIENTS
BROADCAST_RECIPIENTS = args.addresses

MAX_BLUETOOTH_LISTENERS = len(BROADCAST_RECIPIENTS)

def main():
    bluetooth_socket = bind_socket('0.0.0.0', BLUETOOTH_PORT)
    #high_power_socket = bind_socket('0.0.0.0', HIGH_POWER_PORT)

    listen_thread = threading.Thread(target=bluetooth_listener, args=(bluetooth_socket,))
    listen_thread.start()
    listen_thread.join()

def bluetooth_listener(bluetooth_socket):
    bluetooth_listener_pool = futures.ThreadPoolExecutor(max_workers=MAX_BLUETOOTH_LISTENERS)
    while(1):
        # this future probably needs to be executed, and that's why things aren't working now. fix later
        tx_data, tx_address = bluetooth_socket.recvfrom(1024)
        bluetooth_listener_pool.submit(bluetooth_process_advertisement, args=(tx_data, tx_address))


def bluetooth_process_advertisement(data, address):
    print("got packet")
    message = client_socket.recv(1024).decode()
    print(message)

def bind_socket(host, port):
    while True:
        try:
            my_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            my_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            my_socket.bind((host, port))
            break  # Successfully bound the socket
        except OSError as e:
            if e.errno == 98:  # Address already in use
                print("Address in use. Retrying in a moment...")
                time.sleep(1)
            else:
                raise

    return my_socket

if __name__ == "__main__":
    main()

