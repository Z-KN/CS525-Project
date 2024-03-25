# CS525-Project

## Raft-Lite?
Raft-Lite was written by Nikolas Lamb (original here: https://github.com/nikwl/raft-lite) and is being modified to use Berkeley Sockets instead of 0MQ
For now, though, we can stick to using broadcasts over 0MQ

## How to run the emulation
### First, set up the broadcasting nodes
I usually run bluetooth\_broadcast\_node.py on vm 20
```bash
python3 src/bluetooth_broadcast_node.py -a <ip addresses of non-edge non-broadcast nodes>
# if we just run on vm 1,2,3,4
# python3 src/bluetooth_broadcast_node.py -a '172.22.151.96' '172.22.153.5' '172.22.154.232' '172.22.151.210'
```
I usually run edge\_broadcast\_node.py on vm 19
```bash
python3 src/edge_broadcast_node.py -e <ip address of edge node> -a <ip addresses of non-edge non-broadcast nodes>
# python3 src/edge_broadcast_node.py -e '172.22.151.214' -a '172.22.151.96' '172.22.153.5' '172.22.154.232' '172.22.151.210'
```
These files are very similar. I can refactor them after the deadline so that we don't need to edit things twice, but at the moment this works.

### Then, set up the edge server
Set BROADCAST\_NODE\_ADDRESS to the address of the node running edge\_broadcast\_node.py
Set NETWORK\_INTERFACE to be the name of the network interface you're using (found with `ip a`)
```python
BROADCAST_NODE_ADDRESS = '<address here>'
NETWORK_INTERFACE = '<name here>'
```
Then run it. I usually run it on vm 5
```bash
python3 src/edge.py
```

### Finally, set up your nodes
Set BROADCAST\_NODE\_ADDRESS to the address of the node running **bluetooth**\_broadcast\_node.py
Set NETWORK\_INTERFACE to be the name of the network interface you're using (found with `ip a`)
```python
BROADCAST_NODE_ADDRESS = '<address here>'
NETWORK_INTERFACE = '<name here>'
```
Then run it. I usually run this on vms 1-4
```bash
python3 src/node.py -n <node id>
```

## How to run the wireless model
```bash
cd src
python3 wireless_model.py
```
