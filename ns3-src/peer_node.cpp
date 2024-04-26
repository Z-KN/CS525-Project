#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Peers");

// EVAN Addition >>>
class AgreementInformation {
public:
    uint16_t versionNumber, roundNumber;
    std::pair<double, double> fineLocation, coarseLocation;
    std::set<uint32_t> agreeingNodes;
    
    AgreementInformation(std::pair<double, double>* cl, uint32_t id) {
        versionNumber = 0;
        roundNumber = 0;
        fineLocation = *cl;
        coarseLocation = *cl;
        agreeingNodes.insert(id);
    }
private:
};
// EVAN Addition <<<

class EdgeAwareClientApplication : public ns3::Application {
public:
    EdgeAwareClientApplication() {}

    // All the setup for the ports and sockets done here
    virtual void StartApplication() {
        Ptr<Node> node = GetNode();
        // EVAN Addition >>>
        const uint32_t node_id = node->GetId();
        
        NS_LOG_INFO(node_id);
        // if two nodes are within this euclidean distance, then they can receive
        // each others advertisements
        // double distance = 5.0f;

        // uniform noise generator, seeded on node id
        RngSeedManager::SetSeed(node_id);
        
        auto unif_rv = CreateObject<UniformRandomVariable>(); 
        unif_rv->SetAttribute("Min", DoubleValue(-1.0));
        unif_rv->SetAttribute("Max", DoubleValue(1.0));
 
        // array of element positions, indexed by element id.
        // the magic numbers aren't great, but we can focus on cleaning that up later
        // they can be changed to be reasonable regarding how the mobility client organizes
        // (via setPositionAllocator)

        std::vector<std::pair<double, double>> baseElementLocations{
            {5.0f, 3.0f},
            {2.0f, 8.0f},
            {6.0f, 13.0f},
            {10.0f, 18.0f},
            {12.0f, 5.0f}
        };
        
        // each node intitializes their element table, along with noisy versions of the positions
        std::vector<AgreementInformation> AgreementInformation_vec;
        for(std::pair<double, double> &element : baseElementLocations) {
            AgreementInformation_vec.push_back(AgreementInformation(&element, node_id));
        }
        
        std::set<uint32_t> nearbyElements;

        std::map<uint32_t, std::pair<std::string, uint32_t>> localGroup;
        // EVAN Addition <<<

        clientSocket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        clientSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 63031));
        clientSocket->SetRecvCallback(MakeCallback(&EdgeAwareClientApplication::ReceiveData, this));

        sendEvent = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::SendAdvertisement, this);
        // EVAN: things we need to schedule
        //pruneLocalGroup = Simulator::Schedule(Seconds(3), &EdgeAwareClientApplication::CheckHeartbeats, this);
        //checkNearbyElements = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::GetNearbyElements, this);   
    }

    virtual void StopApplication() {}

private:
    
    void GetNearbyElements() {
        //iterate through the elements in the table, see which ones youre close to (according to AgreementInformation_vec.get(i).fineLocation), add them to nearbyElements
        //if you are far from an element, attempt to remove it from nearbyElements
    }

    // Hook to whenever data is received
    void ReceiveData(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        uint32_t dataSize = packet->GetSize();
        uint8_t buffer[dataSize];
        packet->CopyData(buffer, dataSize);
        std::string data(reinterpret_cast<char*>(buffer), dataSize);

        // first, parse the stringified packet
        // if the first part of the string is "advert":
        //      get node id and ip address
        //      take system time
        //      localGroup.insert(id, std::pair<std::string, uint32_t>(ip address, system_time)
        //      i believe "insert" updates the data structure with the new value
        //      
        //      if, after this node parses the sender nodes ip address, it sees that there is a list of elements (as discussed in SendData), then
        //      it checks which of those elements are in its nearby set
        //              then, if the version or round number it just received is higher than what it has in AgreementInformation_vec.get(that element)
        //                      it runs the code i mentioned in the notes before
        //
        
        // if it's something else, you can refer to the notes i pushed earlier for how to handle those

        Address senderAddress;
        socket->GetPeerName(senderAddress);
        Ipv4Address senderIpv4Address = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
        std::cout << "Received data from " << senderIpv4Address << ": " << data << std::endl;
    }

    // this function runs every five seconds, and iterates over the list of nodes
    // it prunes the nodes from the local group that have not been responsive for 10 seconds
    void CheckHeartbeats() {
        // get the list of keys for the local group
        // uint32_t systemtime = current system time
        // for key in keys
        //      if current system time - localgroup.get(key).get(1) > 10 seconds
        //              remove node from local group
    }

    // What to send and when to send 

    // This should be triggered by adding it to the simulator schedule as in line 23
    // EVAN:
    // Input:
    // node_id (const, added to state from start application)
    // ipv4 address (on another interface, if we can implement another interface. refer to the tutorial)
    // nearbyElements
    // list of elements in the space (can be global state)
    void SendAdvertisement() {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
        // if we are using one interface, this message should start with a header, so that ReceiveData knows how to parse it. since my old computer is down, I can't
        // refer to our old 438 code, but we can use something like that. what is here now isn't a great delimiter
        std::string header = "advert";
        // we also need to include the stringified node_id here
        std::string message = header+std::to_string(address.Get());
        //for each entry in AgreementInformation_vec
        //      if its in nearby elements
        //              add {element index i, AgreementInformation_vec.get(i).version_number, AgreementInformation_vec.get(i).round_number}\0 to the message
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

    // EVAN: you don't have to iterate over everything yourself, the helpers can take a lost of nodes and install
    // things for you
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