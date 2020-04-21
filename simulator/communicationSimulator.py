# This script simulates the laserPuckPointer hardware to allow testing of the support module and
# streamDevice. Run this on a local machine and point the AsynIP port to localhost:8000.
import logging
from Pcs8000Controller import Pcs8000Controller
from Pcs8000ControllerMaster import Pcs8000ControllerMaster
from Pcs8000Connections import TCPCommandServer,TCPEventClient






#logging.basicConfig(level=logging.INFO)
# Ports:
tcpCommandPort = 51512
udpStreamPort = 51513
tcpEventPort = 51515

# Create controller objects:
masterController = Pcs8000ControllerMaster()
slave1 = Pcs8000Controller(1)
slave2 = Pcs8000Controller(2)
masterController.addSlave(slave1)
masterController.addSlave(slave2)

# Create main TCP command server
TCPCommandServer("localhost",tcpCommandPort,masterController)
TCPEventClient("localhost",tcpEventPort,masterController)