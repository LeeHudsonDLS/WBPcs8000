from Pcs8000Controller import Pcs8000Controller
import logging
import xmltodict
import xml.etree.ElementTree as ET


class Pcs8000ControllerMaster(Pcs8000Controller):
    def __init__(self):
        Pcs8000Controller.__init__(self,0)
        # Dict containing slaves, slave number is the key
        self.slaves = dict()
        # Set first element to be this object
        self.slaves[0] = self

        # XML roots that precede a setting command, not a program
        self.stateCommands = ['udpxmit','maincontrol','sequencer']

    # Do something useful with an error
    def commandError(self):
        print(f"Error called")

    def addSlave(self,slave):
        self.slaves[int(slave.slaveNo)] = slave

    # Method to replace element written as <xx /> with <xx></xx>
    # This is because the PCS8000 controller doesn't recognise <xx />
    def formatXml(self,xml):

        while xml.find('/>') > -1:
            end = xml.find('/>') + 1
            for char in range(end, end - 20, -1):
                if xml[char] == '<':
                    token = xml[char + 1:end - 2]
                    xml = xml.replace(f'<{token} />', f'<{token}></{token}>')
                    break
        return xml


    # Function to distribute commands to slaves
    def parseCommand(self,command):
        logging.info(f'Pcs8000ControllerMaster::parseCommand()')
        logging.debug(f'Command received:\n {command}')
        commandRoot = ET.fromstring(command)
        addressedSlave = self.slaves[int(commandRoot[0].text)]
        valid = False

        if commandRoot.tag in self.stateCommands:
            action = commandRoot[1].tag

            # Check if the element that contains the value actually has a value, ie, not <start></start>
            hasValue = len(commandRoot[1])
            if hasValue :

                target = commandRoot[1][0].tag
                value = commandRoot[1][0].text.strip('"')

                if target == 'sys_state':
                    assert value in addressedSlave.sysState.states.keys(),self.commandError()
                    addressedSlave.setSysState(value)
                    valid = True
                if target == 'seq_state':
                    assert value in addressedSlave.seqState.states.keys(),self.commandError()
                    addressedSlave.setSeqState(value)
                    valid = True
                if target == 'stream':
                    assert value[:4] == 'phys',self.commandError()
                    assert int(value[4:]) in range(1,16),self.commandError()
                    addressedSlave.registerStream(int(value[4:]))
                    valid = True

            if commandRoot.tag == 'udpxmit' and action == 'start':
                addressedSlave.startUdp()
                valid = True

            if commandRoot.tag == 'udpxmit' and action == 'clear':
                addressedSlave.clearUDP()
                valid = True

        if valid:
            # Append the ackn element to the original command and return it
            ET.SubElement(commandRoot,'ackn').text = "OK"
            logging.debug(f'Pcs8000ControllerMaster Command sent:\n {self.formatXml(ET.tostring(commandRoot).decode())}')
            return self.formatXml(ET.tostring(commandRoot).decode())



