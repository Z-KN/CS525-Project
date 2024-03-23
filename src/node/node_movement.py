import ns.core
import ns.mobility
import ns.network

class CustomMobilityModel(ns.mobility.MobilityModel):
    def __init__(self, velocity):
        super().__init__()
        self.position = ns.core.Vector(0.0, 0.0, 0.0) 
        self.velocity = ns.core.Vector(velocity, 0.0, 0.0) # -> linear movement

    def DoGetPosition(self):
        return self.position

    def DoSetPosition(self, position):
        self.position = position

    def DoGetVelocity(self):
        return ns.core.Vector(1.0, 0.0, 0.0)

    def DoUpdateLocation(self):
        # Update node position based on velocity and time
        if ns.core.Simulator.Now().GetSeconds() < 100 and ns.core.Simulator.Now().GetSeconds() > 50:
            velocity = self.DoGetVelocity()
            displacement = velocity * ns.core.Simulator.Now().GetSeconds()  # Assume constant velocity
            self.DoSetPosition(self.DoGetPosition() + displacement)


    