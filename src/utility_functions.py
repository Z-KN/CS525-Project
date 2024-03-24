import socket
import psutil
import sys
import time

"""
    UTILITY FUNCTIONS
"""
def get_address_on_interface(interface):
    addrs = psutil.net_if_addrs()
    my_address_on_interface = addrs[interface][0][1]
    return my_address_on_interface

def bind_socket(host, port, connection_type):
    while True:
        try:
            my_socket = socket.socket(socket.AF_INET, connection_type)
            my_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            my_socket.bind((host, port))
            break  # Successfully bound the socket
        except OSError as e:
            if e.errno == 98:  # Address already in use. should not appear due to SO_REUSEADDR
                print("Address in use.")
                time.sleep(1)
            else:
                raise

    return my_socket

def get_system_time():
    if sys.version_info[0]==3 and sys.version_info[1] < 8:
        return time.perf_counter() * 1e9
    else:
        return time.perf_counter_ns()
