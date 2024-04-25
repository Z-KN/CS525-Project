#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Peers");


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

    // What to send and when to send 

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


void TracePackets(Ptr<const Packet> packet) {
    NS_LOG_INFO("Packet trace - Received packet at node: " << Simulator::Now().GetSeconds());
}

int main(int argc, char *argv[]) {
    LogComponentEnable("Peers", LOG_LEVEL_INFO);
    // CommandLine cmd;
    // cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(10);  

    InternetStackHelper internet;
    internet.Install(nodes);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices;

    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        for (uint32_t j = i + 1; j < nodes.GetN(); ++j) {
            devices.Add(p2p.Install(nodes.Get(i), nodes.Get(j)));
        }
    }
    NS_LOG_INFO("Testing");

    // IP address assignment 
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); 
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    p2p.EnablePcapAll("peer-to-peer"); 

    uint16_t port = 9; 

    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        // Replace the server and clients (one server/client per node for now)
        UdpEchoServerHelper echoServer(port);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(i));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(10.0));

        Ptr<UdpEchoServer> server = DynamicCast<UdpEchoServer>(serverApps.Get(0));
        server->TraceConnectWithoutContext("Rx", MakeCallback(&TracePackets));
        UdpEchoClientHelper echoClient(interfaces.GetAddress((i + 1) % nodes.GetN()), port);
        

        echoClient.SetAttribute("MaxPackets", UintegerValue(1));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        // Start the client application at time 2.0 seconds
        ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(Seconds(10.0));
    }

    // Enable global static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Run simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
