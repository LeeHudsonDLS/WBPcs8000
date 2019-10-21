from iocbuilder import AutoSubstitution
from iocbuilder import Device
from iocbuilder.arginfo import *
from iocbuilder.modules.asyn import Asyn, AsynPort,AsynIP
from iocbuilder.modules.motor import MotorLib, MotorRecord
from iocbuilder.modules.calc import Calc
from iocbuilder.modules.busy import Busy




class pcsController(Device):
    """Defines the device configuration"""

    Dependencies = (Asyn, MotorLib)

    # Libraries
    LibFileList = ['WBPcs8000']
    DbdFileList = ['WBPcs8000']


    # Constructor, just store parameters
    def __init__(self, PORT,  IP_ADDRESS, name = None, ASYN_ADDRESS=0, NUM_AXES=2, POLLMOVING=200,POLLNOTMOVING=200, **args):
        self.ASYN_TCP_CONTROL = AsynIP('%s:51512' % IP_ADDRESS, '%s_CTRL' % PORT, input_eos="\r\n",output_eos="\r\n")
        Device.__init__(self)
        self.PORT = PORT
        self.IP_ADDRESS = IP_ADDRESS
        self.name = name
        self.ASYN_ADDRESS = ASYN_ADDRESS
        self.NUM_AXES = NUM_AXES
        self.POLLMOVING = POLLMOVING
        self.POLLNOTMOVING = POLLNOTMOVING

    # Once per instantiation
    def Initialise(self):
        print "# Configure UDP port for Walter and Bai streams"
        print 'drvAsynIPServerPortConfigure(%(PORT)s_UDP,"192.168.113.12:51513 udp", 10, 0, 0, 0)' % self.__dict__

        print "# Configure Walter and Bai PCS8000 controller"
        print "# pcsControllerConfig(%(PORT)s,%(ASYN_ADDRESS)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)"
        print "pcsControllerConfig(%(PORT)s, %(ASYN_ADDRESS)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
        PORT = Simple("Asyn port name", str),
        IP_ADDRESS = Simple("Asyn IP port name", str),
        name = Simple("Controller name", str),
        ASYN_ADDRESS = Simple("Asyn IP port address", int),
        NUM_AXES = Simple("Number of axes", int),
        POLLMOVING = Simple("Poll period (ms) while moving", int),
        POLLNOTMOVING = Simple("Poll period (ms) while not moving", int))

class _dls_pcs_asyn_motor(AutoSubstitution,MotorRecord):
    TemplateFile = "dls_pcs_asyn_motor.db"


class pcsAxis(Device):
    """Defines the device configuration"""
    WarnMacros = False
    Dependencies = (Asyn, MotorLib,Busy, Calc)


    # Constructor, just store parameters
    def __init__(self, CONTROLLER, AXIS_NO,P,M,ADDR,DESC,MRES,VELO,PREC,EGU,TWV, **args):
        Device.__init__(self)
        self.CONTROLLER = CONTROLLER
        self.AXIS_NO = AXIS_NO
        self.P = P
        self.M = M
        self.ADDR = ADDR
        self.DESC = DESC
        self.MRES = MRES
        self.VELO = VELO
        self.PREC = PREC
        self.EGU = EGU
        self.TWV = TWV
        _dls_pcs_asyn_motor(P=self.P,M=self.M,PORT=self.CONTROLLER,ADDR=self.ADDR,DESC=self.DESC,MRES=self.MRES,VELO=self.VELO,PREC=self.PREC,EGU=self.EGU,TWV=self.TWV)
    # Once per instantiation
    def Initialise(self):
        print "# Configure Walter and Bai PCS8000 Axis"
        print "# pcsAxisConfig(%(CONTROLLER)s, %(AXIS_NO)d)"
        print "pcsAxisConfig(%(CONTROLLER)s, %(AXIS_NO)d)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
                          CONTROLLER = Simple("Controller name", str),
                          AXIS_NO = Simple("Axis number", int),
                          P = Simple("Axis number", str),
                          M = Simple("Axis number", str),
                          ADDR = Simple("Axis number", int),
                          DESC = Simple("Axis number", str),
                          MRES = Simple("Axis number", float),
                          VELO = Simple("Axis number", float),
                          PREC = Simple("Axis number", int),
                          EGU = Simple("Axis number", str),
                          TWV = Simple("Axis number", float))

