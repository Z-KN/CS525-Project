from ns import ns

class ServerApplication(ns.applications.Application):
    def __init__(self):
        ns.applications.Application.__init__(self)

        
        self.server_socket= None
        self.send_event1 = None
        self.update_keys_event = None
        self.known_nodes = dict()

    def receive_data(self, socket):
        packet = socket.Recv(1024)
        data = packet.GetPayload()
        sender_address = packet.GetAddress()
        print(f"Received data from {sender_address}: {data}")

        message = data.decode().split(",")
        known_nodes[sender_address] = ns.core.Simulator.Now()


    def send_data(self):
        ipv4 = node.GetObject(ns.internet.Ipv4.GetTypeId())
        address = ipv4.GetAddress(1)
        message = f"{str(address)}"
        self.client_socket.SendTo(ns.core.Packet(message.encode()), 0, ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 63031)) 

    def updateKeys(self):
        keylist = list(self.known_nodes)[:]
        for key in keylist:
            if ns.core.Simulator.Now() - self.known_nodes[key] >= 10:
                del self.known_nodes[key]

    def StartApplication(self):
        self.server_socket = ns.network.Socket.CreateSocket(self.GetNode(), ns.network.TypeId.LookupByName("ns3::UdpSocketFactory"))
        
        ns.network.MakeCallback
        self.server_socket.SetRecvCallback(self.receive_data)

        # Bind the socket to listen on port 61021
        self.server_socket.Bind(ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 61021))

        # Create a UDP socket for sending
        self.client_socket = ns.network.Socket.CreateSocket(self.GetNode(), ns.network.TypeId.LookupByName("ns3::UdpSocketFactory"))

        self.send_event1 = ns.core.Simulator.Schedule(ns.core.Seconds(1e9), ns.core.MakeCallback(self.send_data))

        self.update_keys_event = ns.core.Simulator.Schedule(ns.core.Seconds(1), ns.core.MakeCallback(self.send_data))

