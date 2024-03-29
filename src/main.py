import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point
import node_application
import node_movement


sim = ns.core.Simulation()

nodes = ns.network.NodeContainer()
nodes.Create(2)

# Create point-to-point links
pointToPoint = ns.point_to_point.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

devices = pointToPoint.Install(nodes)

# Install internet stack
stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

# Assign IP addresses
address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("10.1.1.0"), ns.network.Ipv4Mask("255.255.255.0"))
interfaces = address.Assign(devices)

# Create custom mobility model
mobility_group_1= ns.mobility.MobilityHelper(-1)
mobility_group_1.Install(nodes.Get(0)) # TODO: increase to 3 nodes

mobility_group2 = ns.mobility.MobilityHelper(1)
mobility_group2.Install(nodes.Get(1))

# Install custom application
customApp = node_application.NodeApp()
customApp.Install(nodes.Get(0))

# Run the simulation
sim.Stop(ns.core.Seconds(10))
sim.Run()
sim.Destroy()