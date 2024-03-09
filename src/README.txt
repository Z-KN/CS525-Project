
some terminology i use:
    local group: this is the group of users (or, alternatively, its corresponding data structure in memory) that are all interacting over bluetooth low energy. the idea is that bluetooth low energy forces users to be nearby because the power is so limited
    feature: this is anything a user creates, for now, we can represent these as a tuple of (data, id). id denotes metadata about the feature (in use cases that can be something like location), and data communicates the actual information sought.

our system consists of two types of nodes:

node:
    there can be many of these in a network
    these are low power, at-the-moment-homogeneous devices that can communicate on two channels: bluetooth low energy, and cell
    we use bluetooth low energy to organize consensus for the most local participants, and higher-power cell channels to conduct that consensus

edge:
    there's only one of these in a network, for now
    they're modeled as nodes with no power constraints and no movement. the edge server is assumed to be reliable as long as users are in range
    in reality, edge servers are associated with base stations. in our case, they're modeled as wireless nodes that broadcast their existence to anything in range (ideally on a different channel than p2p mobile communications)


test setup:
    we can have users in some limited space moving around randomly to start
    additionally in this space are an edge server and a few features, which for us are just (noisy) coordinate pairs and ids.
    
