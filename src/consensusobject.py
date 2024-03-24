class LockedConsensusObject:
    def __init__(self):
        self.consensus_object = [0]*8
        self.lock = threading.Lock()

    def set_value(self, index, value):
        with self.lock:
            self.consensus_object[index] = value
