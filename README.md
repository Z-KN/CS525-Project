# CS525-Project

## Raft-Lite?
Raft-Lite was written by Nikolas Lamb (original here: https://github.com/nikwl/raft-lite) and is being modified to use Berkeley Sockets instead of 0MQ
For now, though, we can stick to using broadcasts over 0MQ

## How to run the emulation
### First, set up the broadcasting nodes
I usually run bluetooth\_broadcast\_node.py on vm 20
```bash
python3 src/bluetooth_broadcast_node.py -a <ip addresses of non-edge non-broadcast nodes>
```
I usually run edge\_broadcast\_node.py on vm 19
```bash
python3 src/edge_broadcast_node.py -e <ip address of edge node> -a <ip addresses of non-edge non-broadcast nodes>
```
These files are very similar. I can refactor them after the deadline so that we don't need to edit things twice, but at the moment this works.

### Then, set up the edge server
In `edge.py`:
Set BROADCAST\_NODE\_ADDRESS to the address of the node running edge\_broadcast\_node.py
Set NETWORK\_INTERFACE to be the name of the network interface you're using (found with `ip a`)
```python
BROADCAST_NODE_ADDRESS = '<address here>'
NETWORK_INTERFACE '<name here>'
```
Then run it. I usually run it on vm 5
```bash
python3 src/edge.py
```

### Finally, set up your nodes
In `node.py`
Set BROADCAST\_NODE\_ADDRESS to the address of the node running **bluetooth**\_broadcast\_node.py
Set NETWORK\_INTERFACE to be the name of the network interface you're using (found with `ip a`)
```python
BROADCAST_NODE_ADDRESS = '<address here>'
NETWORK_INTERFACE '<name here>'
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
