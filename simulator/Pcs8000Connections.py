import logging
from Pcs8000Controller import Pcs8000Controller
from Pcs8000ControllerMaster import Pcs8000ControllerMaster
from threading import *
import socket

class Pcs8000Connection(Thread):
    def __init__(self, ipAddress, portNumber,controller: Pcs8000Controller):
        Thread.__init__(self)
        self.controller = controller
        self.ipAddress = ipAddress
        self.portNumber = portNumber
        self.buffer = ""
        self.start()

    def serverHandShake(self,socket):
        # Send "HELLO"
        # Wait for "DLS,1.00,1234"
        # Send "OK"

        # Send "HELLO\r\n"
        socket.send(f"HELLO\r\n".encode())

        # Wait for response code and check it's correct
        data = socket.recv(4096)
        assert data.rstrip()==b"DLS,1.00,1234","Driver sent %s" % data

        # Send "OK\r\n"
        socket.send(f"OK\r\n".encode())

    def clientHandShake(self,socket,slaveNo):
        # Wait for "HELLO"
        # Send "0,1.00,1234" for slave 0
        # Wait for "OK"

        # Wait for a "HELLO\r\n"
        logging.info(f'Pcs8000Connection.clientHandShake(), waiting for HELLO')
        data = socket.recv(4096)

        assert data==b"HELLO","Driver sent %s" % data
        logging.info(f"Pcs8000Connection.clientHandShake(), Got : {data}")
        print(f"TCPEventClient got {data}")
        # Send "OK\r\n"
        socket.send(f'{slaveNo},1.00,1234'.encode())

        data = socket.recv(4096)
        assert data==b"OK","Driver sent %s" % data

# Class for a TCPCommandServer. The controller passed to this should be a Pcs8000ControllerMaster
class TCPCommandServer(Pcs8000Connection):
    def __init__(self,ipAddress,portNumber,controller: Pcs8000ControllerMaster):
        Pcs8000Connection.__init__(self,ipAddress,portNumber,controller)


    def run(self):

        # Configure the socket
        tcpCommandServer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Allow the socket to be re-used even if it's in TIME_WAIT state
        try:
            tcpCommandServer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,tcpCommandServer.getsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR) | 1)
        except socket.error:
            pass


        tcpCommandServer.bind((self.ipAddress, self.portNumber))
        tcpCommandServer.listen()
        print(f"Listening on {self.ipAddress}:{self.portNumber} (TCPCommandServer)")

        self.serverSocket, address = tcpCommandServer.accept()
        print("Client connected on TCPCommandServer")

        self.serverHandShake(self.serverSocket)
        while True:
            self.buffer+=self.serverSocket.recv(4096).decode()
            if(self.buffer.count("</") == self.buffer.count("<")/2):
                response = self.controller.parseCommand(self.buffer)
                self.serverSocket.send(response.encode())
                self.buffer=""


class TCPEventClient(Pcs8000Connection):
    def __init__(self,ipAddress,portNumber,controller: Pcs8000Controller):
        Pcs8000Connection.__init__(self,ipAddress,portNumber,controller)

    def run(self):

        # Configure the socket
        tcpEventClient = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        connected = False
        latch = False
        print("Waiting for TCP event server to connect to")
        while not connected:
            try:
                tcpEventClient.connect((self.ipAddress, self.portNumber))
                connected = True
            except Exception as e:
                pass
        self.clientHandShake(tcpEventClient,self.controller.slaveNo)
        while True:
            if(latch == False):
                print("Connected to TCP event server")
                latch = True
            data = tcpEventClient.recv(1024)