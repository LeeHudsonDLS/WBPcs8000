import asyncio
import socket
import struct

# Format of the UDP datagrams
DATA_PACKAGE = struct.Struct("<IIIQQff")

# Format of the TCP event packaged
EVENT_PACKAGE = struct.Struct("<IIQII")

# Name to send to controller for host identification
NAME = "DLS"
# Version of the controller protocol
VERSION = "1.00"
# Magic code, as set in the web interface
CODE = "1234"

mainControlDMOV="""
<sequencer>
    <slave>1</slave>
    <get>
        <seq_state></seq_state>
    </get>
</sequencer>"""

mainControlGetSensor="""
<maincontrol>
    <slave>1</slave>
    <get_sensor>
        <sensor>0</sensor>
    </get_sensor>
</maincontrol> 
"""
#Get negative limit
getDriveReady="""
<digital_io>
    <slave>1</slave>
    <get>
        <input>1</input>
    </get>
</digital_io> 
"""


#Get negative limit
getNLimit="""
<digital_io>
    <slave>1</slave>
    <get>
        <input>3</input>
    </get>
</digital_io> 
"""

#Get positive limit
getPLimit="""
<digital_io>
    <slave>1</slave>
    <get>
        <input>2</input>
    </get>
</digital_io> 
"""

mainControlGet="""
<maincontrol>
    <slave>1</slave>
    <get></get>
</maincontrol> 
"""
mainControlSet0="""
<maincontrol>
    <slave>0</slave>
    <set>
        <sys_state>Ready</sys_state>
    </set>
</maincontrol> """

mainControlSet1="""
<maincontrol>
    <slave>1</slave>
    <set>
        <sys_state>Ready</sys_state>
    </set>
</maincontrol> """

clearUDP0="""
<udpxmit>
    <slave>0</slave>
    <clear></clear>
</udpxmit>"""

clearUDP1="""
<udpxmit>
    <slave>1</slave>
    <clear></clear>
</udpxmit>"""

register0="""
<udpxmit>
    <slave>0</slave>
    <register>
        <stream>phys14</stream>
    </register>
</udpxmit>"""

register1="""
<udpxmit>
    <slave>1</slave>
    <register>
        <stream>phys14</stream>
    </register>
</udpxmit>"""

start0="""
<udpxmit>
    <slave>0</slave>
    <start></start>
</udpxmit>"""

start1="""
<udpxmit>
    <slave>1</slave>
    <start></start>
</udpxmit>"""

startSeq0="""
<sequencer>
    <slave>0</slave>
    <set>
        <seq_state>program</seq_state>
        </set>
</sequencer>"""

startSeq1="""
<sequencer>
    <slave>1</slave>
    <set>
        <seq_state>program</seq_state>
        </set>
</sequencer>"""

getSeq1="""
<sequencer>
    <slave>1</slave>
    <get>
        <seq_state>state</seq_state>
    </get>
</sequencer>"""

class PCS8000UDPProtocol:
    def __init__(self):
        self.transport = None

    def connection_made(self, transport):
        self.transport = transport
        print("UDP Connection made")

    def datagram_received(self, data, addr):
        code, slave, num_data, pkg_index, ts, min_drag, max_drag = \
              DATA_PACKAGE.unpack_from(data, offset=0)
        #print(f"Got {code}, {slave}, {num_data}, {pkg_index}, {ts}, {min_drag}, {max_drag}")
        data = struct.unpack("f" * num_data, data[DATA_PACKAGE.size:])

        #print(data[0])

class PCS8000TCPProtocol:
    def __init__(self):
        self.transport = None
        self.remaining = {b'1,1.00,1234', b'0,1.00,1234'}

    def connection_made(self, transport):
        print("TCP Connection made")
        self.transport = transport
        print("Here")
        self.transport.write(b"HELLO")

    def data_received(self, data):
        if data in self.remaining:
            # Controller registering
            self.remaining.remove(data)
            self.transport.write(b"OK")
        else:
            ev_code, slave, ts, ev_value, num_data = \
                     EVENT_PACKAGE.unpack_from(data)
            print(f"Got {ev_code}, {slave}, {ts}, {ev_value}, {num_data}")
            if num_data:
                extra_data = data[EVENT_PACKAGE.size:]
                assert len(extra_data) == num_data
                    

class PCS8000:
    def __init__(self, ip):
        self.ip = ip
        self.reader, self.writer = None, None
        self.transport, self.protocol = None, None
        self.event_server = None
        self.event_server_f = None

    async def connect(self):
        loop = asyncio.get_running_loop()
        # Bring up UDP server
        #self.transport, self.protocol = await loop.create_datagram_endpoint(
            #PCS8000UDPProtocol, local_addr=("localhost", 51513))
        # Bring up event TCP server
        self.event_server = await loop.create_server(
            PCS8000TCPProtocol, "localhost", 51515)
        self.event_server_f = self.event_server.serve_forever()
        # Make connection
        print("Opening connection")
        self.reader, self.writer = await asyncio.open_connection(self.ip, 51512)
        print("Connection opened, waiting for read")
        # Initial handshake
        data = await self.reader.readline()
        print("Got something")
        print(data)
        assert data.rstrip() == b"HELLO", "Controller sent %s" % data
        self.writer.write(f"{NAME},{VERSION},{CODE}".encode())
        data = await self.reader.readline()
        print(data)
        assert data.rstrip() == b"OK", "Controller sent %s" % data
        # Check what mode it is in

        # Clear UDP subscription on slave 0
        await self.send_recv_xml(clearUDP0)
        # Subscribe to phys14 on slave 0
        await self.send_recv_xml(register0)
        # Start UDP stream
        await self.send_recv_xml(start0)

        await asyncio.sleep(60.0)

    async def xml_cmd(self, *args):
        xml = ""
        for a in args[:-1]:
            xml += f"<{a}>"
        xml += args[-1]
        for a in reversed(args[:-1]):
            xml += f"</{a}>"
        #print(xml)
        return await self.send_recv_xml(xml)

    async def send_recv_xml(self, xml):
        xml = xml.rstrip()
        self.writer.write(xml.encode())
        marker = "</" + xml.rsplit("</", 1)[1]
        try:
            data = await asyncio.wait_for(self.reader.readuntil(marker.encode()), timeout=5.0)
        except asyncio.TimeoutError:
            print("***\n" + self.reader._buffer.decode() + "\n***" + marker)
            raise
        ret = data.decode() + marker
        print(f"Got:\n{ret}")
        if "<ackn>OK</ackn>" not in ret:
            print("---\n" + ret)
        return ret

    def disconnect(self):
        self.reader.close()
        self.writer.close()

#DLS1.001234
#1,1.00
pcs = PCS8000("localhost")
asyncio.run(pcs.connect())
        
