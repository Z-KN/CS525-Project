#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Peers");

typedef enum : uint8_t {
    ADVERT,
    REQUEST_DATA,
    SEND_DATA,
    ACK,

} MessageType;

// elementId_t and causalNum_t should be the same
typedef uint8_t elementId_t;
typedef uint8_t causalNum_t;
typedef uint32_t nodeId_t;
typedef float location_t;

// EVAN Addition >>>
class AgreementInformation {
public:
    causalNum_t versionNumber, roundNumber;
    // i picked float because they're 32 bit
    // no coarse location, as our system uses the same data type for both fine and coarse locations
    std::pair<location_t, location_t> fineLocation;
    std::set<nodeId_t> agreeingNodes;
    
    AgreementInformation(std::pair<location_t, location_t>* cl, nodeId_t id): versionNumber(0), roundNumber(0) {
        fineLocation = *cl;
        agreeingNodes.insert(id);
    }

    friend std::ostream& operator<< (std::ostream &os, AgreementInformation &a) {
        // os << "Version: " << unsigned(a.versionNumber) << "\nRound: " << unsigned(a.roundNumber)  << std::endl;
        // os << "Location: (" << a.fineLocation.first << ", " << a.fineLocation.second << ")\n";
        // os << "Agreements:\n";
        // for(nodeId_t elem : a.agreeingNodes) {
        //     os << elem << std::endl;
        // }

        return os << std::endl;
    }
private:
};

class nodeEntry {
public:
    Ipv4Address address;
    int64_t recentTime;
    std::vector<std::pair<causalNum_t, causalNum_t>> versionRoundNumbers;

    nodeEntry(Ipv4Address ad, int64_t time): address(ad), recentTime(time), versionRoundNumbers(5, std::pair<causalNum_t, causalNum_t>{0, 0}) {
    }

    friend std::ostream& operator<< (std::ostream &os, nodeEntry &n) {
        os << "Address: " << n.address << "\nHeartbeat Time: " << n.recentTime  << std::endl;
        for(size_t idx = 0; idx < n.versionRoundNumbers.size(); idx++) {
            os << "Element: " << unsigned(idx) << " Version: " << unsigned(n.versionRoundNumbers.at(idx).first) << " Round: " << unsigned(n.versionRoundNumbers.at(idx).first) << std::endl;
        }
        return os;
    } 
private:
};

// undoing transformation, so endianness should not matter
std::vector<uint8_t> fourByteToOne(uint32_t c) {
    std::vector<uint8_t> r(4);
    r.at(3) = (c >> 24) & 0xFF;
    r.at(2) = (c >> 16) & 0xFF;
    r.at(1) = (c >> 8) & 0xFF;
    r.at(0) = (c) & 0xFF;
    return r;
}

uint32_t oneByteToFour(std::vector<uint8_t> *c) {
    uint32_t r = 0;
    r += (uint32_t)(c->at(0));
    r += (uint32_t)(c->at(1)) << 8;
    r += (uint32_t)(c->at(2)) << 16;
    r += (uint32_t)(c->at(3)) << 24;
    return r;
}

/* FORMAT
 * | type   | nodeid |
 * |element |version | round  | ...
 *
 * | 8 bit  | 32 bit |
 * | 8 bit  | 8 bit  |  8 bit | ...
 *
 */
class ADVERT_Message {
public:
    nodeId_t senderId;
    uint32_t ipv4Address;
    std::vector<elementId_t> elements;
    std::vector<causalNum_t> versions;
    std::vector<causalNum_t> rounds;

    ADVERT_Message(nodeId_t id, uint32_t ad, std::set<elementId_t> *nearbyElements, std::vector<AgreementInformation> *elementInfo): 
        senderId(id), ipv4Address(ad) 
    { 
        // initialize the lists of information that are propagated with the advertisement
        elements.reserve(nearbyElements->size());
        versions.reserve(nearbyElements->size());
        rounds.reserve(nearbyElements->size());
        for(elementId_t element : *nearbyElements) {
            elements.push_back(element);
            versions.push_back(elementInfo->at(element).versionNumber);
            rounds.push_back(elementInfo->at(element).roundNumber);
        }     
    }

    // alternative constructor for deserialization
    ADVERT_Message(nodeId_t id, uint32_t ad, std::vector<elementId_t> *elems, std::vector<causalNum_t> *vers, std::vector<causalNum_t> *rnds): 
        senderId(id), ipv4Address(ad) 
    {
        elements = *elems;
        versions = *vers;
        rounds = *rnds;
    }
    std::vector<uint8_t> serialize() {
        std::vector<uint8_t> mesg{ADVERT};
        std::vector<uint8_t> id_array = fourByteToOne(this->senderId);
        mesg.insert(mesg.end(), id_array.begin(), id_array.end());
        std::vector<uint8_t> ip_array = fourByteToOne(this->ipv4Address);
        mesg.insert(mesg.end(), ip_array.begin(), ip_array.end());
        
        for(size_t idx = 0; idx < elements.size(); idx++) {
            mesg.push_back(this->elements.at(idx));
            mesg.push_back(this->versions.at(idx));
            mesg.push_back(this->rounds.at(idx));
        }

        return mesg;
    }

    static ADVERT_Message deserialize(std::vector<uint8_t> *buf) {
        std::vector<uint8_t> oid(buf->begin() + 1, buf->begin() + 5);
        nodeId_t otherId = oneByteToFour(&oid);
        std::vector<uint8_t> oip(buf->begin() + 5, buf->begin() + 9);
        // if we had two interfaces, these steps would be useful. for now, though, they're just here
        uint32_t otherIP = oneByteToFour(&oip);
        std::vector<elementId_t> otherElements;
        otherElements.reserve((buf->size()-9)/3);
        std::vector<elementId_t> otherVersions;
        otherVersions.reserve(otherElements.size());
        std::vector<elementId_t> otherRounds;
        otherRounds.reserve(otherElements.size());
        for(size_t idx = 9; idx < buf->size(); idx+=3) {
            // recall that the message is structured as:
            // element id - version number - round number
            otherElements.push_back(buf->at(idx));
            otherVersions.push_back(buf->at(idx+1));
            otherRounds.push_back(buf->at(idx+2));
        }
        return ADVERT_Message(otherId, otherIP, &otherElements, &otherVersions, &otherRounds);
    }

    friend std::ostream& operator<< (std::ostream &os, ADVERT_Message &a) {
        os << "Id: " << unsigned(a.senderId) << "\nAddress: " << unsigned(a.ipv4Address)  << std::endl;
        for(size_t idx = 0; idx < a.elements.size(); idx++) {
            os << "Element: " << unsigned(a.elements.at(idx)) << " Version: " << unsigned(a.versions.at(idx)) << " Round: " << unsigned(a.rounds.at(idx)) << std::endl;
        }
        return os;
    }

private:

};

/* FORMAT
 * | type   | nodeid |element |
 *
 * | 8 bit  | 32 bit | 8 bit  |
 *
 */
class REQUEST_DATA_Message {
public:
    nodeId_t senderId;
    elementId_t elementId;

    REQUEST_DATA_Message(nodeId_t nid, elementId_t eid): senderId(nid), elementId(eid) {
    }

    std::vector<uint8_t> serialize() { 
        std::vector<uint8_t> oid = fourByteToOne(this->senderId);
        std::vector<uint8_t> mesg{REQUEST_DATA};
        mesg.insert(mesg.end(), oid.begin(), oid.end());
        mesg.push_back(this->elementId);
        return mesg;
    } 

    static REQUEST_DATA_Message deserialize(std::vector<uint8_t> *buf) { 
        std::vector<uint8_t> oid(buf->begin() + 1, buf->begin() + 5);
        nodeId_t otherId = oneByteToFour(&oid);
        elementId_t elid = buf->at(5);
        return REQUEST_DATA_Message(otherId, elid);
    }
};

/* FORMAT
 * |type    |nodeid  |element |
 * |x pos   |y pos   |
 * |version |round   |
 * |members |...
 *
 * |8 bit   |32 bit  |8 bit   |
 * |32 bit  |32 bit  |
 * |8 bit   |8 bit   |
 * |32 bit  |...
 *
 */
class SEND_DATA_Message {
public:
    nodeId_t senderId;
    elementId_t elementId;
    location_t xpos, ypos;
    causalNum_t version, round;

    std::vector<nodeId_t> sync_group;

    SEND_DATA_Message(nodeId_t nid, 
            elementId_t eid, 
            location_t x, 
            location_t y, 
            causalNum_t ver,
            causalNum_t rnd,
            std::set<nodeId_t> *sg): 
        senderId(nid), elementId(eid), xpos(x), ypos(y), version(ver), round(rnd), sync_group(sg->size())
    {
        sync_group.assign(sg->begin(), sg->end());
    }

    std::vector<uint8_t> serialize() { 
        std::vector<uint8_t> mesg{SEND_DATA};
        std::vector<uint8_t> oid = fourByteToOne(this->senderId);
        mesg.insert(mesg.end(), oid.begin(), oid.end());
        mesg.push_back(this->elementId);
        
        uint32_t serialized_xpos = *reinterpret_cast<uint32_t*>(&this->xpos);
        uint32_t serialized_ypos = *reinterpret_cast<uint32_t*>(&this->ypos);
        
        std::vector<uint8_t> packed_x = fourByteToOne(serialized_xpos);
        std::vector<uint8_t> packed_y = fourByteToOne(serialized_ypos);

        mesg.insert(mesg.end(), packed_x.begin(), packed_x.end());
        mesg.insert(mesg.end(), packed_y.begin(), packed_y.begin());

        mesg.push_back(this->version);
        mesg.push_back(this->round);

        for(nodeId_t nodeId : this->sync_group) {
            std::vector<uint8_t> id = fourByteToOne(nodeId);
            mesg.insert(mesg.end(), id.begin(), id.end());
        }
            
        return mesg;
    } 

    static SEND_DATA_Message deserialize(std::vector<uint8_t> *buf) {  
        std::vector<uint8_t> oid(buf->begin() + 1, buf->begin() + 5);
        nodeId_t otherId = oneByteToFour(&oid);
        
        elementId_t elid = buf->at(5);

        std::vector<uint8_t> px(buf->begin() + 6, buf->begin() + 10);
        std::vector<uint8_t> py(buf->begin() + 10, buf->begin() + 14);
        
        uint32_t packed_x = oneByteToFour(&px);
        uint32_t packed_y = oneByteToFour(&px);

        location_t xpos = *reinterpret_cast<location_t*>(&packed_x);
        location_t ypos = *reinterpret_cast<location_t*>(&packed_y);

        causalNum_t vers = buf->at(14);
        causalNum_t rnd = buf->at(15);

        std::set<nodeId_t> sync_group;
        for(size_t idx = 16; idx < buf->size(); idx+=4) {
            std::vector<uint8_t> syn(buf->begin() + idx, buf->begin() + idx + 3);
            nodeId_t sync_nodeId = oneByteToFour(&syn);
            sync_group.insert(sync_nodeId);
        }

        return SEND_DATA_Message(otherId, elid, xpos, ypos, vers, rnd, &sync_group);
    }
};

class EdgeAwareClientApplication : public ns3::Application {
public:
    EdgeAwareClientApplication() {}
   
    uint32_t heartbeatTimeout = 10000;

    void Setup(Ptr<Socket> socket, Ptr<Socket> recvSocket_,  Address address) {
        clientSocket = socket;
        destination = address;
        recvSocket = recvSocket_;
        // clientSocket -> Bind();

        // NS_LOG_INFO(clientSocket->Connect(address));
    }

    void triggerReceive() {
        recvSocket->SetRecvCallback(MakeCallback(&EdgeAwareClientApplication::ReceiveData, this));
    }

    void onAccept(Ptr<Socket> s, const Address& from) {
        NS_LOG_INFO("Accepted connection");
        s->SetRecvCallback(MakeCallback(&EdgeAwareClientApplication::ReceiveData, this));
    }

    // All the setup for the ports and sockets done here
    virtual void StartApplication() {
        Ptr<Node> node = GetNode();
        // EVAN Addition >>>
        node_id = node->GetId();
        
        NS_LOG_INFO(node_id);

        // uniform noise generator, seeded on node id
        RngSeedManager::SetSeed(node_id + 1);
        
        auto unif_rv = CreateObject<UniformRandomVariable>(); 
        unif_rv->SetAttribute("Min", DoubleValue(-1.0));
        unif_rv->SetAttribute("Max", DoubleValue(1.0));
 
        // array of element positions, indexed by element id.
        // the magic numbers aren't great, but we can focus on cleaning that up later
        // they can be changed to be reasonable regarding how the mobility client organizes
        // (via setPositionAllocator)

        std::vector<std::pair<location_t, location_t>> baseElementLocations{
            {5.0f, 3.0f},
            {2.0f, 8.0f},
            {6.0f, 13.0f},
            {10.0f, 18.0f},
            {12.0f, 5.0f}
        };
        
        // each node intitializes their element table, along with noisy versions of the positions
        for(std::pair<location_t, location_t> &element : baseElementLocations) {
            AgreementInformation_vec.push_back(AgreementInformation(&element, node_id));
        } 
        // EVAN Addition <<<

        recvSocket->Listen();
        recvSocket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeCallback (&EdgeAwareClientApplication::onAccept, this));

        // clientSocket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        // clientSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 63031));
        // clientSocket->Bind();
        clientSocket->Bind();
        clientSocket->Connect(destination);
        

        sendEvent = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::SendAdvertisement, this);
        // EVAN: things we need to schedule
        //pruneLocalGroup = Simulator::Schedule(Seconds(3), &EdgeAwareClientApplication::CheckHeartbeats, this);
        //checkNearbyElements = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::GetNearbyElements, this);   
    }

    virtual void StopApplication() {}

private:
    
    void GetNearbyElements() {
        Ptr<Node> node = GetNode();
        Vector3D nodePosition = node->GetObject<MobilityModel>()->GetPosition();
        location_t distance = 5.0f;
        //iterate through the elements in the table, see which ones youre close to (according to AgreementInformation_vec.get(i).fineLocation), add them to nearbyElements
        //if you are far from an element, attempt to remove it from nearbyElements
        for(size_t idx = 0; idx < AgreementInformation_vec.size(); idx++) {
            location_t xdiff = pow(AgreementInformation_vec.at(idx).fineLocation.first - nodePosition.x, 2);
            location_t ydiff = pow(AgreementInformation_vec.at(idx).fineLocation.second - nodePosition.y, 2);
            if(xdiff + ydiff < pow(distance, 2)) {
                if(nearbyElements.count(idx) == 0) {
                    BeginAgreement((elementId_t)idx);
                }
                nearbyElements.insert((elementId_t) idx);   
            }
            else {
                nearbyElements.erase((elementId_t) idx);
            }
        } 
    }

    // Hook to whenever data is received
    // For some reason this callback does not work
    void ReceiveData(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        uint32_t dataSize = packet->GetSize();
        uint8_t buffer[dataSize];
        packet->CopyData(buffer, dataSize);
        // NS_LOG_INFO("" << node_id << " received data");
        // because of how we serialize, the first byte is the message type 
        if(buffer[0] == ADVERT) {
            // NS_LOG_INFO("Received advertisement");
            std::vector<uint8_t> vectorized_buffer(buffer, buffer + sizeof(buffer)/sizeof(buffer[0])); 
            ADVERT_Message info = ADVERT_Message::deserialize(&vectorized_buffer);
            NS_LOG_INFO(info);
            nodeEntry thisNode(Ipv4Address(info.ipv4Address), Simulator::Now().GetMilliSeconds());
            for(size_t idx = 0; idx < info.elements.size(); idx++) {
                if(nearbyElements.count(info.elements.at(idx)) == 1) {
                    uint32_t ourVersion = AgreementInformation_vec.at(buffer[idx]).versionNumber;
                    uint32_t ourRound = AgreementInformation_vec.at(buffer[idx]).roundNumber;
                    
                    uint32_t theirVersion = info.versions.at(idx);
                    uint32_t theirRound = info.rounds.at(idx);
                    if(ourVersion < theirVersion || (ourVersion == theirVersion && ourRound < theirRound)) {
                        thisNode.versionRoundNumbers.at(info.elements.at(idx)) = std::pair<causalNum_t, causalNum_t>(theirVersion, theirRound);
                    }
                }
            }
            localGroup.insert_or_assign(info.senderId, thisNode);
        }
        // SETTLE LOCAL GROUPS FIRST >>>
        else if(buffer[0] == REQUEST_DATA) {
            //
        }
        else if(buffer[0] == SEND_DATA) { 
            //MergeSyncGroups();
            //SendACK(senderIpv4Address);
        }
        else if(buffer[0] == ACK) {
            //MergeSyncGroups();
        }
        else {
            NS_LOG_INFO("INVALID MESSAGE");
        }

        for(auto elem : localGroup) {
            NS_LOG_INFO(elem.second);
        }
        // <<<
       
       /* 
        Address senderAddress;
        socket->GetPeerName(senderAddress);
        Ipv4Address senderIpv4Address = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
        std::cout << "Received data from " << senderIpv4Address << ": " << buffer << std::endl;
        */
    }

    // this function runs every five seconds, and iterates over the list of nodes
    // it prunes the nodes from the local group that have not been responsive for 10 seconds
    void CheckHeartbeats() {
        std::vector<uint32_t> deletionSet;
        deletionSet.reserve(2);
        uint32_t currentTime = Simulator::Now().GetMilliSeconds();
        for(auto localGroupNodePair : localGroup) {
            if(currentTime - localGroupNodePair.second.recentTime >= heartbeatTimeout) {
                deletionSet.push_back(localGroupNodePair.first);
            } 
        }
        for(uint32_t key : deletionSet) {
            localGroup.erase(key);
        }
    }

    // This should be triggered by adding it to the simulator schedule as in line 23
    void SendAdvertisement() {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
        // NS_LOG_INFO("Sending advertisement");
        uint32_t ipv4_num = address.Get();
        ADVERT_Message adv(node_id, ipv4_num, &nearbyElements, &AgreementInformation_vec);
        
        NS_LOG_INFO(adv);
        std::vector<uint8_t> mesg = adv.serialize();

        Ptr<Packet> packet = Create<Packet>(mesg.data(), mesg.size());
        clientSocket->Send(packet);

        sendEvent = Simulator::Schedule(Time("1s"), &EdgeAwareClientApplication::SendAdvertisement, this);
    }

    /* SETTLE LOCAL GROUPS FIRST */
    void BeginAgreement(elementId_t element) {
        //if there is an edge server
        //      SendDataRequest to edge server address
        //      we can model an edge server as a node that doesn't move (similar to the access point). otherwise its application code can be the same
        //if there is no edge server, but there is a local group
        //otherwise
        //      set element round number to 1
    }
    
    void SendDataRequest(Ipv4Address address, elementId_t elementId) {
        REQUEST_DATA_Message req(node_id, elementId);
        
        std::vector<uint8_t> mesg = req.serialize();
        Ptr<Packet> packet = Create<Packet>((uint8_t*)mesg.data(), sizeof(mesg.data()));
        // we need to set up client sockets properly to use the received IP addresses
        // we can setup and teardown a connection immediately in this method if we have the receiver nodes ip address and TCP port (data exchange happens over a reliable channel)
        clientSocket->Send(packet);
    }

    void SendData(Ipv4Address address, elementId_t elementId) {
        AgreementInformation elem = AgreementInformation_vec.at(elementId);
        location_t xpos = elem.fineLocation.first;
        location_t ypos = elem.fineLocation.second;
        causalNum_t version = elem.versionNumber;
        causalNum_t round = elem.roundNumber;
        std::set<nodeId_t> *sync = &elem.agreeingNodes;

        SEND_DATA_Message send(node_id, elementId, xpos, ypos, version, round, sync);
        
        std::vector<uint8_t> mesg = send.serialize();
        Ptr<Packet> packet = Create<Packet>((uint8_t*)mesg.data(), sizeof(mesg.data()));
        // see above
        clientSocket->Send(packet);
    }

    // WIP
    void SendACK(Ipv4Address address) { 
        std::vector<uint8_t> mesg{ACK}; 
        Ptr<Packet> packet = Create<Packet>(mesg.data(), mesg.size());
        // see above
        clientSocket->Send(packet);
    }  

    Ptr<Socket> clientSocket;
    EventId sendEvent;
    Address destination;
    DataRate m_dataRate;
    Ptr<Socket> recvSocket;
    // I'm not too familiar with callbacks in c++, but if possible we should not keep these as global state so that we can handle concurrency better
    std::vector<AgreementInformation> AgreementInformation_vec;
    std::set<elementId_t> nearbyElements;
    // this data structure indexes by node id, and stores IPv4 address (as a string) as well as local time
    // due to how this system works, only the local group entry is modified by heartbeat. therefore it needs to contain a lot of information
    std::map<nodeId_t, nodeEntry> localGroup;
    nodeId_t node_id;
}; // This is a copy of the server application, but needs to be edited to replicate the other behavior


void TracePackets(Ptr<const Packet> packet) {
    NS_LOG_INFO("Packet trace - Received packet at node: " << Simulator::Now().GetSeconds());
}

int main(int argc, char *argv[]) {
    LogComponentEnable("Peers", LOG_LEVEL_INFO);
    // CommandLine cmd;
    // cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(2);  

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Setup wifi channels
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());


    // Setup wifi ssid nodes
    WifiMacHelper mac;
    Ssid ssid = Ssid("clients");

    WifiHelper wifi;

    NetDeviceContainer devices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    devices = wifi.Install(phy, mac, nodes);


    InternetStackHelper stack;

    stack.Install(nodes);
    // NetDeviceContainer devices;
    // devices = p2p.Install(nodes);

    // InternetStackHelper internet;
    // internet.Install(nodes);
    
    // IP address assignment 
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); 
    Ipv4InterfaceContainer interfaces = address.Assign(devices);


    // uint16_t port = 9; 

    // EVAN: you don't have to iterate over everything yourself, the helpers can take a lost of nodes and install
    // things for you

    // Temporary way to do it but we can do a loop for each node or we can write a helper
    Ptr<EdgeAwareClientApplication> clientApp1 = CreateObject<EdgeAwareClientApplication>();
    Ptr<Socket> UdpSocket = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());
    Ptr<Socket> UdpSocketRecv = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());

    UdpSocketRecv -> Bind(InetSocketAddress(interfaces.GetAddress(0), 8080));
    
    uint16_t port = 8080;
    Address client1Address(InetSocketAddress(interfaces.GetAddress(0), port));
    Address client2Address(InetSocketAddress(interfaces.GetAddress(1), port));

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes.Get(0));

    MobilityHelper mobility2;

    mobility2.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(-5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility2.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility2.Install(nodes.Get(1));

    
    clientApp1->Setup(UdpSocket, UdpSocketRecv, client2Address); // setup connection to 1st node
    nodes.Get(0)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(1.0));
    clientApp1->SetStopTime(Seconds(10.0));

    clientApp1->triggerReceive();

    Ptr<EdgeAwareClientApplication> clientApp2 = CreateObject<EdgeAwareClientApplication>();
    Ptr<Socket> UdpSocket2 = Socket::CreateSocket(nodes.Get(1), UdpSocketFactory::GetTypeId());
    Ptr<Socket> UdpSocketRecv2 = Socket::CreateSocket(nodes.Get(1), UdpSocketFactory::GetTypeId());
    UdpSocketRecv2 -> Bind(InetSocketAddress(interfaces.GetAddress(1), 8080));


    clientApp2->Setup(UdpSocket2, UdpSocketRecv2, client1Address); // setup connection to 0th node
    nodes.Get(1)->AddApplication(clientApp2);
    clientApp2->SetStartTime(Seconds(1.0));
    clientApp2->SetStopTime(Seconds(10.0));
    Simulator::Stop(Seconds(10.0));
    clientApp2->triggerReceive();
    // Enable global static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.EnablePcap("edge-aware", devices);
    // Run simulation
    Simulator::Run();
    Simulator::Destroy();


    return 0;
}
