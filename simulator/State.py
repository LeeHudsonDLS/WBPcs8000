# Class to store the state of a variable.
# Constructor called with dict of states
# State is set by calling setState with string of state

class State:
    def __init__(self,states):
        self.states = states
        self.state = 0

    def setState(self,stateString):
        if stateString in self.states:
            self.state = self.states[stateString]