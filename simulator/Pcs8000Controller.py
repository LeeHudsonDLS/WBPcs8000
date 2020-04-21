from State import State
import logging
import xmltodict
import xml.etree.ElementTree as ET

class Pcs8000Controller:
    def __init__(self,slaveNo):
        logging.info(f'Pcs8000Controller::Pcs8000Controller({slaveNo})')
        self.slaveNo = slaveNo
        self.udpRunning = 0
        # List of stream values
        self.streamValues = [0]*16
        # List of streamValues to be streamed via udp
        self.registeredStreams = list()
        self.sysState = State({"Busy":0,"Ready":1})
        self.seqState = State({"Setup":0,"Program":1})

    def startUdp(self):
        logging.info(f'Pcs8000Controller.startUDP() on slave {self.slaveNo}')
        self.udpRunning = 1

    # Function to set sys_state
    def setSysState(self,stateString):
        logging.info(f'Pcs8000Controller::setSysState({stateString}) on slave {self.slaveNo}')
        self.sysState.setState(stateString)

    # Function to set seq_state
    def setSeqState(self,stateString):
        logging.info(f'Pcs8000Controller::setSeqState({stateString}) on slave {self.slaveNo}')
        self.seqState.setState(stateString)

    def registerStream(self,stream):
        assert stream in range(1,16)
        logging.info(f'Pcs8000Controller::registerStream({stream}) on slave {self.slaveNo}')
        self.registeredStreams.append(stream)

    def clearUDP(self):
        logging.info(f'Pcs8000Controller.clearUDP() on slave {self.slaveNo}')
        self.registeredStreams = list()