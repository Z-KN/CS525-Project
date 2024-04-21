#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;

class ServerApplication : public ns3::Application {
public:
    ServerApplication() {}

    virtual void StartApplication() {
        Ptr<Node> node = GetNode(); 


        serverSocket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 61021));
        serverSocket->SetRecvCallback(MakeCallback(&ServerApplication::ReceiveData, this));

        clientSocket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());

        sendEvent = Simulator::Schedule(Seconds(1), &ServerApplication::SendData, this);
        updateKeysEvent = Simulator::Schedule(Seconds(1), &ServerApplication::UpdateKeys, this);
    }

    virtual void StopApplication() {}

private:
    void ReceiveData(Ptr<Socket> socket) {
        // Ptr<Packet> packet = socket->Recv();
        // uint32_t dataSize = packet->GetSize();
        // uint8_t buffer[dataSize];
        // packet->CopyData(buffer, dataSize);
        // std::string data(reinterpret_cast<char*>(buffer), dataSize);

        // Address senderAddress;
        // socket->GetPeerName(senderAddress);
        // Ipv4Address senderIpv4Address = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
        // std::cout << "Received data from " << senderIpv4Address << ": " << data << std::endl;

        // std::vector<std::string> tokens = SplitString(data, ',');
        // knownNodes[senderIpv4Address] = Simulator::Now();
    }

    void SendData() {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
        std::string message = std::to_string(address.Get());
        clientSocket->SendTo(Ptr<Packet>(Create<Packet>((const uint8_t*)message.c_str(), message.size())), 0, InetSocketAddress(Ipv4Address::GetAny(), 63031));
    }

    void UpdateKeys() {
        Time currentTime = Simulator::Now();
        for (auto it = knownNodes.begin(); it != knownNodes.end();) {
            if (currentTime - it->second >= Seconds(10)) {
                it = knownNodes.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    Ptr<Socket> serverSocket;
    Ptr<Socket> clientSocket;
    EventId sendEvent;
    EventId updateKeysEvent;
    std::map<Ipv4Address, Time> knownNodes;
};


// TODO: split the classes into multiple files

class ClientApplication : public ns3::Application {
public:
    ClientApplication() {}


    // All the setup for the ports and sockets done here
    virtual void StartApplication() {
        Ptr<Node> node = GetNode();

        clientSocket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        clientSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 63031));
        clientSocket->SetRecvCallback(MakeCallback(&ClientApplication::ReceiveData, this));

        sendEvent = Simulator::Schedule(Seconds(1), &ClientApplication::SendData, this);
    }

    virtual void StopApplication() {}

private:

    // Hook to whenever data is received
    void ReceiveData(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        uint32_t dataSize = packet->GetSize();
        uint8_t buffer[dataSize];
        packet->CopyData(buffer, dataSize);
        std::string data(reinterpret_cast<char*>(buffer), dataSize);

        Address senderAddress;
        socket->GetPeerName(senderAddress);
        Ipv4Address senderIpv4Address = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
        std::cout << "Received data from " << senderIpv4Address << ": " << data << std::endl;
    }

    // note on advertisements:
    //  advertisements should contain a list of elements a node is close to, as well as the causal timestamp and round timestamp each node has for that element
    //  if a node moves sufficiently far away from the coordinates of an element, it should not advertise that element anymore

    // the big idea with this model is to try to get a loose majority on what the state is meant to look like. we cant establish quorum, and we don't have the time to enforce consensus, so instead nodes should eventually converge. also, nodes can have the same "version" of a node but different visualizations. this is what this aims to solve
    
    // on some timer, call GetPosition from the mobility class. if the device is near a element in the list it was initialized with, run this. also, run    it if a neighbor's causal timestamp or round number is higher than yours
    //
    // void BeginAgreement() {
    // recall this data structure: (causal timestamp, round timestamp, fine-grained position (0), cartesian coordinates of features, synchronized set)
    //  causal timestamp: this is the version of the element this node sees
    //  round timestamp: communication between nodes happens in rounds. every time a node gets an ACK from a new node requesting data, it increases its round timer.
    //  the positions should be intuitive
    //  the synchronized set is the set of nodes that the node believes are on the same page regarding membership for the past time horizon
    //      this is unbounded memory, i still need to think of a way around this
    //
    //  if edge server exists{
    //      contact the edge server and ask for its data structure. the edge server likely has a timestamp that will beat yours. if it doesn't, that means you have new information for the edge server retry, and if this fails, go down to the next if block
    //  }
    //  if local group has members{
    //      if there exists a node with a timestamp or round number greater than yours:
    //          (this either means you are new to the element, or you have fallen out of sync)
    //
    //          request the data structure from a neighbor with the biggest timestamp in their advertisements for that element
    //              send them a message that makes them run ReceiveDataRequest()
    //              pick one of these leader nodes at random if there are multiple
    //
    //      if not, but there exists a node with a higher round number than you, 
    //          request the data structure from a neighbor with the biggest round number in their advertisements for that element
    //              send them a message that makes them run ReceiveDataRequest()
    //              pick one of these leader nodes at random if there are multiple
    //
    //          put this on a timeout. if no response is received, go down to the else block
    //      
    //      if no nodes has higher values, do nothing. this means you have a version that beats out everyone else's
    //  }
    //  else {
    //      increment causal timestamp if it was at 0
    //      copy cartesian coordinates of features into into the fine-grained position field
    //      merge this nodes id with the empty set
    //  }
    // }
    
    // void ReceiveDataRequest() {
    //  if you receive this message, that means a node thought you had the most advanced timestamp or round number
    //  send that node your data structure. tell them if they are in your synchronization group
    //  }
 
    // void ReceiveData() {
    //  parse the data that was just received from the node with the higher timestamp or round number
    //  merge yourself with the participating group field, . raise your round number to theirs. if they tell you that you were not in their synchronization group, then increment your round number as well
    //
    
    // void ReceiveDataACK() {
    //  iif they were not in your synchronization group before, add them to your synchronization group and increment your round number. otherwise, they were in your synchronization group, and are just updating stale records instead of bootstrapping
    // }

    // This should be triggered by adding it to the simulator schedule as in line 23
    void SendData() {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
        std::string message = std::to_string(address.Get());
        clientSocket->SendTo(Ptr<Packet>(Create<Packet>((const uint8_t*)message.c_str(), message.size())), 0, InetSocketAddress(Ipv4Address::GetAny(), 61021));
    }

    Ptr<Socket> clientSocket;
    EventId sendEvent;
}; // This is a copy of the server application, but needs to be edited to replicate the other behavior


int main(int argc, char* argv[]) {
    ns3::NodeContainer nodes;
    nodes.Create(2); // Assuming only one node for simplicity

    //initialize list of features here, and pass these to each client and server on initialization:
    //(causal timestamp, 0, cartesian coordinates of features, empty set)
    //when each client is initialized with this data structure, have each node randomize the cartesian coordinates

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    ns3::InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    Ptr<ServerApplication> serverApp = CreateObject<ServerApplication>();
    nodes.Get(0)->AddApplication(serverApp);
    serverApp->SetStartTime(Seconds(0));
    serverApp->SetStopTime(Seconds(10)); 

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    return 0;
}
