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

int main(int argc, char* argv[]) {
    ns3::NodeContainer nodes;
    nodes.Create(2); // Assuming only one node for simplicity


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