import numpy as np

class Node:
    def __init__(self, x, y, z, id, energy, range):
        self.x = x
        self.y = y
        self.z = z
        self.id = id
        self.energy = energy
        self.range = range

    def calc_distance(self, node):
        return np.sqrt((self.x - node.x)**2 + (self.y - node.y)**2 + (self.z - node.z)**2)

    def is_in_range(self, node):
        return self.calc_distance(node) <= self.range

    def __str__(self):
        return f"Node {self.id} ({self.x}, {self.y}, {self.z})"
    
    def __repr__(self):
        return f"Node {self.id} ({self.x}, {self.y}, {self.z})"
    
    def __eq__(self, other):
        return self.id == other.id
    
def signal_strength(node, distance):
    # TODO: this is a simple model, we should use a more complex model
    #       things that we can model:
    #           fading (included here, roughly)
    #           latency (may be complex, as we need to factor in interference and other nearby nodes)
    return 1 / (1 + distance**2)

# create 10 nodes
nodes = [Node(np.random.randint(0, 100), np.random.randint(0, 100), 0, i, 100, 10) for i in range(10)]
