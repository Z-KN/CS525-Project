import zmq
import time
import multiprocessing
from concurrent import futures
from queue import Empty
import json
import socket

from .protocol import MessageType

"""
This code converts the existing interface.py to use Berkeley Sockets instead

This has some challenges, because Berkeley Sockets do not support multicast
in our case, publishers are mapped to multiple subscribers which is supported by the TCP client/server model
however subscribers are also mapped to multiple publishers, which is not

the fast and ugly solution that only uses berkeley sockets (with a reliable connection, this can be done with UDP but I don't want to
change raft guarantees) is creating a set of listener sockets, each on their own thread, which listen to different publishers
"""

HIGH_POWER_TX_PORT = 60021
HIGH_POWER_RX_PORT = 60031

class Talker(multiprocessing.Process):
    def __init__(self, port_list, identity):
        super(Talker, self).__init__()
        # no broadcast primitive, so configure address_list
        self.address_list = port_list
        
        # Port to talk from
        (self.address, self.port) = identity['my_id'].split(":")
        print(self.address)
        print(self.port)

        # Backoff amounts
        self.initial_backoff = 1.0
        self.operation_backoff = 0.0001

        # Place to store outgoing messages
        self.messages = multiprocessing.Queue()

        # Signals
        self._ready_event = multiprocessing.Event()
        self._stop_event = multiprocessing.Event()

    def stop(self):
        self._stop_event.set()

    def run(self):
        """
        Here, we have one publisher that listens for multiple subscribers
        Luckily, we know who the subscribers should be based on the local group
        """
        # All of the zmq initialization has to be in the same function for some reason
        # zmq "contexts" are lists of sockets that the node will use
        ## context = zmq.Context()
        # create a publisher socket. this is an abstraction of a berkeley socket that broadcasts to all peers
        ## pub_socket = context.socket(zmq.PUB)
        pub_socket = bind_socket('0.0.0.0', HIGH_POWER_TX_PORT, socket.SOCK_STREAM)
        pub_socket.listen()

        connections = set()
        #while True:
        #    try:
        #        pub_socket.bind("tcp://%s" % self.address)
        #        break
        #    except zmq.ZMQError:
        #        time.sleep(0.1)

        # the original implementation has publishers publish to the same node's subscriber
        while len(connections) < len(self.address_list):
            connection, address = pub_socket.accept()
            print(f"connected to {address}")
            connections.add((connection, address))

        # Need to backoff to give the connections time to initizalize
        time.sleep(self.initial_backoff)

        # Signal that you're ready
        self._ready_event.set()

        while not self._stop_event.is_set():
            try:    
                message = json.dumps(self.messages.get_nowait())
                messagelen = len(message.encode())
                for (connection, address) in connections:
                    #print(f"{messagelen} - {message}")
                    # in order to parse these messages, we need to add in a delimiter
                    finalmessage = messagelen.to_bytes(2, "big") + message.encode()
                    connection.send(finalmessage)
            except Empty:
                try:
                    time.sleep(self.operation_backoff)
                except KeyboardInterrupt:
                    break
            except KeyboardInterrupt:
                break
        
        pub_socket.close()

    def send_message(self, msg):
        self.messages.put(msg)
    
    def wait_until_ready(self):
        while not self._ready_event.is_set():
            time.sleep(0.1)
        return True

class Listener(multiprocessing.Process):
    def __init__(self, port_list, identity):
        super(Listener, self).__init__()

        # List of ports to subscribe to
        self.address_list = port_list
        self.identity = identity

        # Backoff amounts
        self.initial_backoff = 1.0

        # Place to store incoming messages
        self.messages = multiprocessing.Queue()

        # Signals
        self._stop_event = multiprocessing.Event()

    def stop(self):
        self._stop_event.set()
 
    def run(self):
        """
        The listener consists of multiple sockets hooked on consecutive port numbers
        """
        # All of the zmq initialization has to be in the same function for some reason
        ##context = zmq.Context()
        ##sub_sock = context.socket(zmq.SUB)
        ##sub_sock.setsockopt(zmq.SUBSCRIBE, b'')
        sub_sockets = set()

        for i, a in enumerate(self.address_list):
            sub_sock = bind_socket('0.0.0.0', HIGH_POWER_RX_PORT+i, socket.SOCK_STREAM)
            (address, port) = a.split(":")
            for attempt in range(10):
                try:
                    sub_sock.connect((address, HIGH_POWER_TX_PORT))
                    sub_sockets.add(sub_sock)
                    break
                except OSError as e:
                    if e.errno == 111:
                        print("connection failed. retrying...")
                        time.sleep(self.initial_backoff)
                    else:
                        raise

        # Need to backoff to give the connections time to initizalize
        time.sleep(self.initial_backoff)
        
        processing_pool = futures.ThreadPoolExecutor(max_workers = len(self.address_list)+1)
        # in the background, these socket futures are listening and submitting anything they see to the message queue
        socket_futures = [processing_pool.submit(self.handle_socket, a.dup()) for a in sub_sockets]
        while not self._stop_event.is_set():
            try:
                pass
            except KeyboardInterrupt:
                break
       
        futures.wait(socket_futures)
        sub_sock.close()
   
    def handle_socket(self, socket):
        print(socket)
        while not self._stop_event.is_set():
            msglen = int.from_bytes(socket.recv(2), "big")
            msg = socket.recv(msglen).decode()
            try:
                #print(f"received {msg} from {socket}\n")
                msg = json.loads(msg)
                self.messages.put(msg)
            except json.JSONDecodeError as e:
                if e.msg == "Expecting value" and len(e.doc) == 0:
                    pass
                else:
                    #pass
                    print(f"{e.msg} for {e.doc}")

    def get_message(self):
        # If there's nothing in the queue Queue.Empty will be thrown
        try:
            return self.messages.get_nowait()
        except Empty:
            return None

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
