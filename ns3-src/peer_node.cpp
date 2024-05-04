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


InetSocketAddress BeaconBroadcastAddress = InetSocketAddress(Ipv4Address::GetBroadcast(), 80);

// EVAN Addition >>>
class AgreementInformation {
public: 
    AgreementInformation(std::pair<location_t, location_t>* cl, nodeId_t id): versionNumber(0), roundNumber(0) {
        fineLocation = *cl;
        agreeingNodes.insert(id);
    }

    void set_version(causalNum_t v) { versionNumber = v; }
    causalNum_t get_version() { return versionNumber; }

    void set_round(causalNum_t r) { roundNumber = r; }
    causalNum_t get_round() { return roundNumber; }

    void set_fineLocation(std::pair<location_t, location_t> *fl) {
        fineLocation.first = fl->first;
        fineLocation.second = fl->second;
    }
    std::pair<location_t, location_t> get_fineLocation() { return fineLocation; } 


    bool add_agreeingNodes(nodeId_t n) { return agreeingNodes.insert(n).second; }
    void set_agreeingNodes(std::set<nodeId_t> *ag) { 
        agreeingNodes.clear();
        agreeingNodes.insert(ag->begin(), ag->end());
    }
    bool count_agreeingNodes(nodeId_t n) { return agreeingNodes.count(n) == 1; }
    std::set<nodeId_t> get_agreeingNodes() { return agreeingNodes; }
    void clear_agreeingNodes() { agreeingNodes.clear(); }

    friend std::ostream& operator<< (std::ostream &os, AgreementInformation &a) {
        os << "Version: " << unsigned(a.versionNumber) << "\nRound: " << unsigned(a.roundNumber)  << std::endl;
        os << "Location: (" << a.fineLocation.first << ", " << a.fineLocation.second << ")\n";
        os << "Synchronized Nodes: ";
        for(nodeId_t elem : a.agreeingNodes) {
            os << elem << " ";
        }

        return os;
    }
private:
    causalNum_t versionNumber, roundNumber;
    // no coarse location, as our system uses the same data type for both fine and coarse locations
    std::pair<location_t, location_t> fineLocation;
    std::set<nodeId_t> agreeingNodes;
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
            versions.push_back(elementInfo->at(element).get_version());
            rounds.push_back(elementInfo->at(element).get_round());
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
    
    friend std::ostream& operator<< (std::ostream &os, REQUEST_DATA_Message &r) {
        os << "Id: " << unsigned(r.senderId) << "\nElement: " << unsigned(r.elementId)  << std::endl;
        return os;
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
        mesg.insert(mesg.end(), packed_y.begin(), packed_y.end());
  
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
        uint32_t packed_y = oneByteToFour(&py);
  
        location_t xpos = *reinterpret_cast<location_t*>(&packed_x);
        location_t ypos = *reinterpret_cast<location_t*>(&packed_y);
  
        causalNum_t vers = buf->at(14);
        causalNum_t rnd = buf->at(15);
  
        std::set<nodeId_t> sync_group;
        for(size_t idx = 16; idx < buf->size(); idx+=4) {
            std::vector<uint8_t> syn(buf->begin() + idx, buf->begin() + idx + 4);
            nodeId_t sync_nodeId = oneByteToFour(&syn);
            sync_group.insert(sync_nodeId);
        }
        return SEND_DATA_Message(otherId, elid, xpos, ypos, vers, rnd, &sync_group);
    }
    
    friend std::ostream& operator<< (std::ostream &os, SEND_DATA_Message &s) {
            os << "Id: " << unsigned(s.senderId) << "\nElement: " << unsigned(s.elementId)  << std::endl;
            os << "Location: (" << s.xpos << ", " << s.ypos << ")\n";
            os << "Version: " << unsigned(s.version) << " Round: " << unsigned(s.round) << std::endl;
            for(nodeId_t elem : s.sync_group) {
                os << "Node: " << elem << std::endl;
            }
            return os;
        }
};

class ACK_Message {
public:
    nodeId_t senderId;
    elementId_t elementId;
  
    ACK_Message(nodeId_t nid, elementId_t eid): senderId(nid), elementId(eid) {
    }
  
    std::vector<uint8_t> serialize() { 
        std::vector<uint8_t> oid = fourByteToOne(this->senderId);
        std::vector<uint8_t> mesg{ACK};
        mesg.insert(mesg.end(), oid.begin(), oid.end());
        mesg.push_back(this->elementId);
        return mesg;
    } 
  
    static ACK_Message deserialize(std::vector<uint8_t> *buf) { 
        std::vector<uint8_t> oid(buf->begin() + 1, buf->begin() + 5);
        nodeId_t otherId = oneByteToFour(&oid);
        elementId_t elid = buf->at(5);
        return ACK_Message(otherId, elid);
    }
    
    friend std::ostream& operator<< (std::ostream &os, ACK_Message &a) {
        os << "Id: " << unsigned(a.senderId) << "\nElement: " << unsigned(a.elementId)  << std::endl;
        return os;
    }
};

class EdgeAwareClientApplication : public ns3::Application {
public:
    EdgeAwareClientApplication() {}
   
    uint32_t heartbeatTimeout = 10000;

    void Setup(Ptr<Socket> dataSendSocket_, 
                Ptr<Socket> dataRecvSocket_, 
                Ptr<Socket> broadcastSendSocket_,
                Ptr<Socket> broadcastRecvSocket_) 
    {
        dataSendSocket = dataSendSocket_;
        dataRecvSocket = dataRecvSocket_;
        broadcastSendSocket = broadcastSendSocket_;
        broadcastRecvSocket = broadcastRecvSocket_;
        broadcastRecvSocket->SetRecvCallback(MakeCallback(&EdgeAwareClientApplication::ReceiveData, this));    
        dataRecvSocket->SetRecvCallback(MakeCallback(&EdgeAwareClientApplication::ReceiveData, this));
    }

    // All the setup for the ports and sockets done here
    void StartApplication() {
        Ptr<Node> node = GetNode();
        node_id = node->GetId();

        auto unif_rv = CreateObject<UniformRandomVariable>(); 
        unif_rv->SetAttribute("Min", DoubleValue(-0.5));
        unif_rv->SetAttribute("Max", DoubleValue(0.5));
 
        std::vector<std::pair<location_t, location_t>> baseElementLocations{
            {2.5f, 5.5f},
            {1.0f, 4.0f},
            {3.0f, 6.5f},
            {5.0f, 9.0f},
            {6.0f, 2.5f}
        };
        for(auto &elem : baseElementLocations) {
            elem = std::pair<location_t, location_t>(elem.first + unif_rv->GetValue(), elem.second + unif_rv->GetValue());
        }
        
        for(std::pair<location_t, location_t> &element : baseElementLocations) {
            AgreementInformation_vec.push_back(AgreementInformation(&element, node_id));
        } 
 
        NS_LOG_INFO("Starting state for node " << node_id << ": ");
        for(auto elem : AgreementInformation_vec) {
            NS_LOG_INFO(elem);
        }
        NS_LOG_INFO("");

        sendEvent = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::SendAdvertisement, this);
        pruneLocalGroup = Simulator::Schedule(Seconds(3), &EdgeAwareClientApplication::CheckHeartbeats, this);
        checkNearbyElements = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::GetNearbyElements, this);   
    }

    void StopApplication() {
        // we need to unbind things here
        NS_LOG_INFO("Ending state for node " << node_id << ": ");
        for(auto elem : AgreementInformation_vec) {
            NS_LOG_INFO(elem);
        }
        NS_LOG_INFO("");
    }

private:
    void GetNearbyElements() {
        Ptr<Node> node = GetNode();
        Vector3D nodePosition = node->GetObject<MobilityModel>()->GetPosition();
        // NS_LOG_INFO(nodePosition);
        location_t distance = 8.0f;
        //iterate through the elements in the table, see which ones youre close to (according to AgreementInformation_vec.get(i).fineLocation), add them to nearbyElements
        //if you are far from an element, attempt to remove it from nearbyElements
        for(size_t idx = 0; idx < AgreementInformation_vec.size(); idx++) {
            location_t xdiff = pow(AgreementInformation_vec.at(idx).get_fineLocation().first - nodePosition.x, 2);
            location_t ydiff = pow(AgreementInformation_vec.at(idx).get_fineLocation().second - nodePosition.y, 2);
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
        checkNearbyElements = Simulator::Schedule(Seconds(1), &EdgeAwareClientApplication::GetNearbyElements, this);   
    }
 
    void BeginAgreement(elementId_t element) {
        causalNum_t ourVersion = AgreementInformation_vec.at(element).get_version();
        causalNum_t ourRound = AgreementInformation_vec.at(element).get_round();
        
        causalNum_t maxVersion = 0;
        causalNum_t maxRound = 0;
        nodeId_t bestNode = UINT32_MAX;
        bool no_version = AgreementInformation_vec.at(element).get_round() == 0;
        NS_LOG_INFO("node " << node_id << " is beginning agreement");
        if(!localGroup.empty()) {
            for(auto node : localGroup) {       
                uint32_t theirVersion = node.second.versionRoundNumbers.at(element).first;
                uint32_t theirRound = node.second.versionRoundNumbers.at(element).second;

                if( theirVersion > maxVersion || (theirVersion == maxVersion && theirRound > maxRound) ) {
                    maxVersion = theirVersion;
                    maxRound = theirRound;
                    bestNode = node.first;
                }
            }
            if(ourVersion < maxVersion || (ourVersion == maxVersion && ourRound < maxRound)) {
                NS_LOG_INFO("replacement found for element " << unsigned(element) << " from node " << bestNode);
                SendDataRequest(localGroup.at(bestNode).address, element);
            }
            else {
                NS_LOG_INFO("replacement not found for element " << unsigned(element));
                if(no_version) {
                    AgreementInformation_vec.at(element).set_round(node_id+1);
                    NS_LOG_INFO("initialized to " << (node_id+1));
                }
            }
        }
        else {
            NS_LOG_INFO("found " << unsigned(element) << " but, empty local group");
            if(no_version) {
                AgreementInformation_vec.at(element).set_round(node_id+1);
                NS_LOG_INFO("initialized to " << (node_id+1));
            }
        }
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
        pruneLocalGroup = Simulator::Schedule(Seconds(3), &EdgeAwareClientApplication::CheckHeartbeats, this);
    }

    void ReceiveData(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        uint32_t dataSize = packet->GetSize();
        uint8_t buffer[dataSize];
        packet->CopyData(buffer, dataSize);
        // NS_LOG_INFO("" << node_id << " received data");

        // because of how we serialize, the first byte is the message type 
        std::vector<uint8_t> vectorized_buffer(buffer, buffer + sizeof(buffer)/sizeof(buffer[0])); 
        if(buffer[0] == ADVERT) {
            //NS_LOG_INFO("Received advertisement");
            ADVERT_Message info = ADVERT_Message::deserialize(&vectorized_buffer);
            //NS_LOG_INFO(info);
            nodeEntry thisNode(Ipv4Address(info.ipv4Address), Simulator::Now().GetMilliSeconds());
            bool queueBeginAgreement = false;
            std::vector<elementId_t> agree;

            for(size_t idx = 0; idx < info.elements.size(); idx++) {
                causalNum_t theirVersion = info.versions.at(idx);
                causalNum_t theirRound = info.rounds.at(idx);

                causalNum_t ourVersion = AgreementInformation_vec.at(info.elements.at(idx)).get_version();
                causalNum_t ourRound = AgreementInformation_vec.at(info.elements.at(idx)).get_round();

                if(nearbyElements.count(info.elements.at(idx))==1) {
                    if(theirVersion > ourVersion || (theirVersion == ourVersion && theirRound > ourRound)) {
                        NS_LOG_INFO("node " << node_id << " saw " << info.senderId << ", a better node for nearby element " << unsigned(info.elements.at(idx)));
                        queueBeginAgreement = true;
                        agree.push_back(info.elements.at(idx));
                    }
                }
                thisNode.versionRoundNumbers.at(info.elements.at(idx)) = std::pair<causalNum_t, causalNum_t>(theirVersion, theirRound);
            }
            localGroup.insert_or_assign(info.senderId, thisNode);
            if(queueBeginAgreement) {
                for(auto elem : agree) {
                    BeginAgreement(elem);
                }
            }
        }
        else if(buffer[0] == REQUEST_DATA) {
            //NS_LOG_INFO("Received data request");
            REQUEST_DATA_Message info = REQUEST_DATA_Message::deserialize(&vectorized_buffer);
            Ipv4Address address = localGroup.at(info.senderId).address;
            SendData(address, info.elementId);
        }
        else if(buffer[0] == SEND_DATA) { 
            //NS_LOG_INFO("Received data");
            SEND_DATA_Message info = SEND_DATA_Message::deserialize(&vectorized_buffer);

            AgreementInformation *elementInfo = &AgreementInformation_vec.at(info.elementId);

            std::set<nodeId_t> agreeset(info.sync_group.begin(), info.sync_group.end());
            elementInfo->set_version(info.version);
            elementInfo->set_round(info.round);
            if(agreeset.count(node_id) == 0) {
                elementInfo->set_round(elementInfo->get_round()+1);
            }
            agreeset.insert(node_id);
            elementInfo->set_agreeingNodes(&agreeset);
            std::pair<location_t, location_t> tmp(info.xpos, info.ypos);
            elementInfo->set_fineLocation(&tmp);
            
            Ipv4Address address = localGroup.at(info.senderId).address;
            SendACK(address, info.elementId);
        }
        else if(buffer[0] == ACK) {
            // NS_LOG_INFO("Received ACK");
            ACK_Message info = ACK_Message::deserialize(&vectorized_buffer);
             
            AgreementInformation *elementInfo = &AgreementInformation_vec.at(info.elementId);
            if(elementInfo->count_agreeingNodes(info.senderId) == false) {
                elementInfo->set_round(elementInfo->get_round()+1);
                elementInfo->add_agreeingNodes(info.senderId);
            }
        }
        else {
            NS_LOG_INFO("INVALID MESSAGE");
        }   
    }

    // This should be triggered by adding it to the simulator schedule as in line 23
    void SendAdvertisement() {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
        uint32_t ipv4_num = address.Get();
        ADVERT_Message adv(node_id, ipv4_num, &nearbyElements, &AgreementInformation_vec);
        
        //NS_LOG_INFO(adv);
        std::vector<uint8_t> mesg = adv.serialize();
        Ptr<Packet> packet = Create<Packet>(mesg.data(), mesg.size());
        broadcastSendSocket->Send(packet);
        sendEvent = Simulator::Schedule(Time("1s"), &EdgeAwareClientApplication::SendAdvertisement, this);
    }
    
    void SendDataRequest(Ipv4Address address, elementId_t elementId) {
        REQUEST_DATA_Message req(node_id, elementId);
        
        std::vector<uint8_t> mesg = req.serialize();
        Ptr<Packet> packet = Create<Packet>(mesg.data(), mesg.size());
        dataSendSocket->SendTo(packet, 0, InetSocketAddress(address, 8080));
        //NS_LOG_INFO(gunk);
    }

    void SendData(Ipv4Address address, elementId_t elementId) {
        AgreementInformation elem = AgreementInformation_vec.at(elementId);
        std::set<nodeId_t> sync = elem.get_agreeingNodes();

        SEND_DATA_Message send(
                node_id,
                elementId,
                elem.get_fineLocation().first,
                elem.get_fineLocation().second,
                elem.get_version(),
                elem.get_round(),
                &sync);
        
        std::vector<uint8_t> mesg = send.serialize();
        Ptr<Packet> packet = Create<Packet>(mesg.data(), mesg.size());
        dataSendSocket->SendTo(packet, 0, InetSocketAddress(address, 8080));
    }

    void SendACK(Ipv4Address address, elementId_t elementId) {
        ACK_Message ack(node_id, elementId);
        
        std::vector<uint8_t> mesg = ack.serialize();
        Ptr<Packet> packet = Create<Packet>((uint8_t*)mesg.data(), sizeof(mesg.data()));
        dataSendSocket->SendTo(packet, 0, InetSocketAddress(address, 8080));
    }

    Ptr<Socket> dataSendSocket, dataRecvSocket, broadcastSendSocket, broadcastRecvSocket;
    EventId sendEvent, checkNearbyElements, pruneLocalGroup;
    std::vector<AgreementInformation> AgreementInformation_vec;
    std::set<elementId_t> nearbyElements;
    std::map<nodeId_t, nodeEntry> localGroup;
    nodeId_t node_id;
}; 

void TracePackets(Ptr<const Packet> packet) {
    NS_LOG_INFO("Packet trace - Received packet at node: " << Simulator::Now().GetSeconds());
}

int main(int argc, char *argv[]) {
    LogComponentEnable("Peers", LOG_LEVEL_INFO);
    // CommandLine cmd;
    // cmd.Parse(argc, argv);
    const static int num_nodes = 3;
    NodeContainer nodes;
    nodes.Create(num_nodes); 
   
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    std::string phyMode("DsssRate1Mbps");
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    YansWifiPhyHelper wifiPhy;
    // This is one parameter that matters when using FixedRssLossModel
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set("RxGain", DoubleValue(0));
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // The below FixedRssLossModel will cause the rss to be fixed regardless
    // of the distance between the two stations, and the transmit power
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());
  
    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                   "DataMode",
                                   StringValue(phyMode),
                                   "ControlMode",
                                   StringValue(phyMode));
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
/*
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = p2p.Install(nodes);
    NS_LOG_INFO("Testing");
*/
    InternetStackHelper stack;
    stack.Install(nodes);
 
    // IP address assignment to the above interface
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0"); 
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Temporary way to do it but we can do a loop for each node or we can write a helper
    Ptr<EdgeAwareClientApplication> ClientApps[num_nodes];
    Ptr<Socket> UdpDataSendSockets[num_nodes];
    Ptr<Socket> UdpDataRecvSockets[num_nodes];
    Ptr<Socket> UdpBeaconSinks[num_nodes];
    Ptr<Socket> UdpBeaconSources[num_nodes];

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

    for (int n = 0; n < num_nodes; n++) {
        // each client needs an application
        ClientApps[n] = CreateObject<EdgeAwareClientApplication>();

        UdpDataRecvSockets[n] = Socket::CreateSocket(nodes.Get(n), tid);
        UdpDataRecvSockets[n]->Bind(InetSocketAddress(interfaces.GetAddress(n), 8080));

        // beacon_sink on every node
        UdpBeaconSinks[n] = Socket::CreateSocket(nodes.Get(n), tid);
        UdpBeaconSinks[n]->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));

        UdpDataSendSockets[n] = Socket::CreateSocket(nodes.Get(n), tid);

        // beacon source on every node
        UdpBeaconSources[n] = Socket::CreateSocket(nodes.Get(n), tid);
        UdpBeaconSources[n]->SetAllowBroadcast(true);
        UdpBeaconSources[n]->Connect(BeaconBroadcastAddress);        

        ClientApps[n]->Setup(UdpDataSendSockets[n], UdpDataRecvSockets[n], UdpBeaconSources[n], UdpBeaconSinks[n]);
        auto unif_rv = CreateObject<UniformRandomVariable>();
        unif_rv->SetAttribute("Min", DoubleValue(0));
        unif_rv->SetAttribute("Max", DoubleValue(1));
        ClientApps[n]->SetStartTime(Seconds(unif_rv->GetValue()));
        ClientApps[n]->SetStopTime(Seconds(13.0));
        
        nodes.Get(n)->AddApplication(ClientApps[n]);
    }

    // change the velocity and model:
    // Ptr<ConstantVelocityMobilityModel> nodeMobilityModel = nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    // nodeMobilityModel->SetVelocity(Vector(speed, 0, 0));

    // Ptr<ConstantVelocityMobilityModel> nodeMobilityModel2 = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
    // nodeMobilityModel2->SetVelocity(Vector(-speed, 0, 0));
    
    MobilityHelper mobility;
    auto positions = CreateObject<RandomRectanglePositionAllocator>();
  
    // change this seed when necessary 
    RngSeedManager::SetSeed(1); 
    auto unif_rv = CreateObject<UniformRandomVariable>();
    unif_rv->SetAttribute("Min", DoubleValue(0));
    unif_rv->SetAttribute("Max", DoubleValue(10));

    positions->SetX(unif_rv);
    positions->SetY(unif_rv);

    mobility.SetPositionAllocator(positions);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    
    // simulator now ends late so that the applications can dump state
    Simulator::Stop(Seconds(14.0));
    
    // Enable global static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // p2p.EnablePcapAll("peer-to-peer"); 
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    wifiPhy.EnablePcap("edge-aware", devices);

    // Run simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
