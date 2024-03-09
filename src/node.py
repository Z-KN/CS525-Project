# node.py

# each device needs a way to write and read application state
#   functions for writing application state
#       update state
#       update peers

#   functions for reading state
#       local agreement (includes local consensus and local agreement)


def main():
    """
    main
        starts threads that listen and advertise on the bluetooth low energy channel
        binds device to its IP address and sets it to listen on a higher-power channel
    """

def bluetooth_listener():
    """
    bluetooth_listener

        listen for bluetooth low energy advertisements from other devices on the appropriate channel
            people have implemented bluetooth low energy for ns3: https://gitlab.com/Stijng/ns3-ble-module/-/tree/master/ble/examples?ref_type=heads
            but we can start by modeling a low throughput, high fading channel in ns3 instead of implementing bluetooth-specific work
        
        these advertisements are defined as follows (taken from https://www.bluetooth.com/bluetooth-resources/intro-to-bluetooth-advertisements/):
            insert definition here
            additionally, they may also advertise the MAC addresses of its neighbors. but we can start with default bluetooth advertisements
            
        these advertisements will communicate which nodes are nearby, and therefore the nodes that actually need to reach consensus
            we can maintain a small membership list in memory, consisting of MAC addresses ideally, since those are consistent even if a user leaves and rejoins the local group
    """

def bluetooth_sender():
    """
    bluetooth_sender

        propagate bluetooth low energy advertisements on the appropriate channel  
    """

def edge_listener():
    """
    edge_listener

        this function is meant to run on a thread and listen for edge server broadcasts
        if an edge server is found, it updates the advertisements it sends out to its local group
    """

def local_agreement():
    """
    local_agreement

        from this function we can move either to local_consensus or local_dissemination depending on the rules below. if we see issues regarding getting all nodes to run either local_consensus or local_dissemination, then we can try gossiping that decision over bluetooth

        first, check timestamp of last edge server heartbeat. if no such heartbeat exists, or the last edge server heartbeat is above some threshold, thenwe don't know of an edge server. due to the high locality between nodes, we assume that if we don't know of an edge server, our peers don't know of an edge server. we will eventually need to refine how this is handled. maybe if one node gets a value from an edge server, and another node gets a result from an edge server, then the edge-capable node has a modified proposal
            nevertheless, for now, we move into local_consensus

        if the heartbeat is active, then we know of an edge server. if we know of an alive edge server, then our local group likely does as well. we can move to local_dissemination
    """

def local_consensus():
    """
    local_consensus

        we can use peer-to-peer data channels that exist (https://developer.mozilla.org/en-US/docs/Games/Techniques/WebRTC_data_channels) to figure out how we want to model these channels, compared to the bluetooth channels. my idea is that it will probably be higher power and have further range, because consensus involving these channels should be invoked less frequently.
        we can justify this by saying that our big use case (SLAM consensus) can use Kalman filtering or other non-network tracking systems and still maintain good service, according to the Edge-SLAM, etc papers we have available

        over this channel, we can use RAFT. we can try to use a churn-tolerant leader election sceheme, but I think we can analyze whether there's a lot of churn first. I have a suspicion we won't have a lot, because even if users are "churning" in and out of the local group, they will still be reachable by a strong channel. they can then reject participation explicitly, or time out. 
    """

def local_dissemination():
    """
    local_dissemination

        here, the node spawns a request to get state from the edge server
    """

def update_state():
    """
    update_state

        if a user creates a feature in a multi-user environment, it needs to update the edge about that feature. if an edge server is not available, then it needs to log the creation of that feature and add coarse-grained information about that feature to the cloud
        this way, if other users want to interact with that feature, it will still be available to them
        once a feature is logged with nearby edge servers, it can be associated with spatial SLAM mapping data that is more computationally intensive and localized

    """

def update_peers():
    """
    update_peers

        if a user creates a feature in a multi-user environment, and that user also has other users nearby, it needs to authoritatively indicate to its local group information about the feature. if no edge server is available, it is not sufficient to wait for that feature to finish uploading to the internet and reach some consistent state to be visible to the users. instead, the creating user can immediately pass information to their local group. this is not consensus because it is authoritatively handled by the feature-creating user
    """


if __name__ == "__main__":
    main()
