# Create a simple application
class NodeApp(ns.applications.Application):
    def StartApplication(self):
        pass        

    def StopApplication(self):
        pass

    def Schedule(self):
        # schedule consensus here
        pass
    
    def HandleRead(self):
        # read state here
        pass

    def HandleWrite(self):
        # write state here
        pass