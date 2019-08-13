from iocbuilder import AutoSubstitution
from iocbuilder import Device
from iocbuilder.arginfo import *
from iocbuilder.modules.asyn import Asyn, AsynPort
from iocbuilder.modules.motor import MotorLib

class pcsController(Device):
    """Defines the device configuration"""

    Dependencies = (Asyn, MotorLib)

    # Libraries
    LibFileList = ['WBPcs8000']
    DbdFileList = ['WBPcs8000']


    # Constructor, just store parameters
    def __init__(self, PORT, ASYN_PORT, ASYN_ADDRESS=0, NUM_AXES=2, POLLMOVING=200,POLLNOTMOVING=200, **args):
        Device.__init__(self)
        self.PORT = PORT
        self.ASYN_PORT = ASYN_PORT
        self.ASYN_ADDRESS = ASYN_ADDRESS
        self.NUM_AXES = NUM_AXES
        self.POLLMOVING = POLLMOVING
        self.POLLNOTMOVING = POLLNOTMOVING

    # Once per instantiation
    def Initialise(self):
        print "# Configure Walter and Bai PCS8000 controller"
        print "# pcsControllerConfig(%(PORT)s, %(ASYN_PORT)s, %(ASYN_ADDRESS)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)"
        print "pcsControllerConfig(%(PORT)s, %(ASYN_PORT)s, %(ASYN_ADDRESS)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
        PORT = Simple("Asyn port name", str),
        ASYN_PORT = Ident("Asyn IP port name", AsynPort),
        ASYN_ADDRESS = Simple("Asyn IP port address", int),
        NUM_AXES = Simple("Number of axes", int),
        POLLMOVING = Simple("Poll period (ms) while moving", int),
        POLLNOTMOVING = Simple("Poll period (ms) while not moving", int))

class pcsAxis(Device):
    """Defines the device configuration"""

    Dependencies = (Asyn, MotorLib)

    # Libraries
    #LibFileList = ['WBPcs8000']
    #DbdFileList = ['WBPcs8000']


    # Constructor, just store parameters
    def __init__(self, CONTROLLER, AXIS_NO, **args):
        Device.__init__(self)
        self.CONTROLLER = CONTROLLER
        self.AXIS_NO = AXIS_NO

    # Once per instantiation
    def Initialise(self):
        print "# Configure Walter and Bai PCS8000 Axis"
        print "# pcsAxisConfig(%(CONTROLLER)s, %(AXIS_NO)d)"
        print "pcsControllerConfig(%(CONTROLLER)s, %(AXIS_NO)d)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
                          CONTROLLER = Ident("Asyn port name", pcsController),
                          AXIS_NO = Simple("Axis number", int))