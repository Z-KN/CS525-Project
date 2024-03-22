import threading

class CommitLog:
    def __init__(self, file):
        self.file = file
        self.lock = threading.Lock()
        self.last_term = 0
        self.last_index = -1

    def get_last_index_term(self):
        with self.lock:
            return self.last_index, self.last_term

    # basis of the log is just writing changes
    def log(self, term, command):
        with self.lock:
            with open(self.file, 'a') as f:
                f.write(f"{term}, {command}\n")
                self.last_term = term
                self.last_index += 1

            return self.last_index, self.last_term

    # if there's a view change, the leader can overwrite commit logs
    def log_replace(self, term, commands, start):
        # commands: list of commands committed by the leader to the follower. this is the whole leader log
        # start: index to start overwriting from. everything before start is assumed to be correct

        index = 0
        i = 0
        with self.lock:
            with open(self.file, 'r+') as f:
                # if there are commands
                if len(commands) > 0:
                    while i < len(commands):

                        # 
                        if index >= start:
                            command = commands[i]
                            i += 1
                            f.write(f"{term}, {command}\n")

                            if index > self.last_index:
                                self.last_term = term
                                self.last_index = index

                        index += 1

                    f.truncate()
            return self.last_index, self.last_term

    def read_log(self):
        with self.lock:
            output = []
            with open(self.file, 'r') as f:
                for line in f:
                    term, command = line.strip().split(",")
                    output += [(term, command)]
            
            return output

    def read_logs_start_end(self, start, end=None):
        # Return in memory array of term and command between start and end indices
        with self.lock:
            output = []
            index = 0
            with open(self.file, 'r') as f:
                for line in f:
                    if index >= start:
                        term, command = line.strip().split(",")
                        output += [(term, command)]
                    
                    index += 1
                    if end and index > end:
                        break
            
