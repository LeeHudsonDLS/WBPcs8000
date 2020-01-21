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
    def __init__(self, PORT,  IP_ADDRESS, NO_OF_SLAVES, name = None, ASYN_ADDRESS=0, NUM_AXES=2, POLLMOVING=200,POLLNOTMOVING=200, **args):
        self.ASYN_TCP_CONTROL = AsynIP('%s:51512' % IP_ADDRESS, '%s_CTRL' % PORT, noProcessEos=1)
        Device.__init__(self)
        self.PORT = PORT
        self.IP_ADDRESS = IP_ADDRESS
        self.NO_OF_SLAVES = NO_OF_SLAVES
        self.name = name
        self.ASYN_ADDRESS = ASYN_ADDRESS
        self.NUM_AXES = NUM_AXES
        self.POLLMOVING = POLLMOVING
        self.POLLNOTMOVING = POLLNOTMOVING

    # Once per instantiation
    def Initialise(self):
        print "# Configure UDP port for Walter and Bai streams"
        print 'drvAsynIPServerPortConfigure(%(PORT)s_UDP,"192.168.113.12:51513 udp", 10, 0, 0, 0)' % self.__dict__
        print 'drvAsynIPServerPortConfigure(%(PORT)s_TCP,"192.168.113.12:51515 tcp", 10, 0, 0, 0)' % self.__dict__

        print "# Configure Walter and Bai PCS8000 controller"
        print "# pcsControllerConfig(%(PORT)s,%(ASYN_ADDRESS)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)"
        print "pcsControllerConfig(%(PORT)s, %(ASYN_ADDRESS)d, %(NO_OF_SLAVES)d, %(NUM_AXES)d, %(POLLMOVING)d, %(POLLNOTMOVING)d)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
        PORT = Simple("Asyn port name", str),
        IP_ADDRESS = Simple("Asyn IP port name", str),
        NO_OF_SLAVES = Choice("Number of slaves", range(1,9)),
        name = Simple("Controller name", str),
        ASYN_ADDRESS = Simple("Asyn IP port address", int),
        NUM_AXES = Simple("Number of axes", int),
        POLLMOVING = Simple("Poll period (ms) while moving", int),
        POLLNOTMOVING = Simple("Poll period (ms) while not moving", int))

class _sequencerControl(AutoSubstitution):
    TemplateFile = "sequencerControl.db"

class _dls_pcs_asyn_motor(AutoSubstitution,MotorRecord):
    TemplateFile = "dls_pcs_asyn_motor.db"

class pcsSlave(Device):
    """Defines the device configuration"""
    WarnMacros = False
    Dependencies = (Asyn, MotorLib,Busy, Calc)


    # Constructor, just store parameters
    def __init__(self, CONTROLLER, SLAVE_NO, P,ADDR=0,**args):
        Device.__init__(self)
        self.CONTROLLER = CONTROLLER
        self.PORT = self.CONTROLLER.name
        self.SLAVE_NO = SLAVE_NO
        self.P = P
        self.M = ":SLAVE"+str(SLAVE_NO)
        self.ADDR = ADDR

        if self.SLAVE_NO >= self.CONTROLLER.NO_OF_SLAVES:
            print("SLAVE_NO too high")
            self.SLAVE_NO = self.CONTROLLER.NO_OF_SLAVES -1
        else:
            print("SLAVE_NO Valid")

        _sequencerControl(P=self.P,M=self.M,PORT=self.PORT,SLAVE_NO=self.SLAVE_NO,ADDR=self.ADDR)
    # Once per instantiation
    def Initialise(self):
        print "# Slave"

    # Arguments
    ArgInfo = makeArgInfo(__init__,
                          CONTROLLER = Ident("Controller name", pcsController),
                          SLAVE_NO = Choice("Slave number", range(0,9)),
                          P = Simple("Axis number", str),
                          M = Simple("Axis number", str),
                          ADDR = Simple("Axis number", int))


class pcsAxis(Device):
    """Defines the device configuration"""
    WarnMacros = False
    Dependencies = (Asyn, MotorLib,Busy, Calc)


    # Constructor, just store parameters
    def __init__(self, SLAVE,P,M,ADDR,DESC,VELO,PREC,EGU,TWV,FEEDBACK1='phys14',FEEDBACK2='phys15', SEN_MIN = 0.0,KP=0.5,TI=0.05,TD=0,T1=0.000125,KE=60,KE2=0,KFF=1200,KREI=0,TAU=0,ELIM=0,KDCC=0,SYM_MAN=0,SYM_ADP=0,GKI=0,TKI=0,PK=1, **args):
        Device.__init__(self)
        self.SLAVE = SLAVE
        self.SLAVE_NO = SLAVE.SLAVE_NO
        self.CONTROLLER = SLAVE.CONTROLLER
        self.PORT = self.CONTROLLER.PORT
        self.SLAVE = SLAVE
        self.P = P
        self.M = M
        self.ADDR = ADDR
        self.DESC = DESC
        self.VELO = VELO
        self.PREC = PREC
        self.EGU = EGU
        self.TWV = TWV
        self.FEEDBACK1 = FEEDBACK1
        self.FEEDBACK2 = FEEDBACK2
        self.SEN_MIN = float(SEN_MIN)
        self.KP = KP
        self.TI = TI
        self.TD = TD
        self.T1 = T1
        self.KE = KE
        self.KE2 = KE2
        self.KFF = KFF
        self.KREI = KREI
        self.TAU = TAU
        self.ELIM = ELIM
        self.KDCC = KDCC
        self.SYM_MAN = SYM_MAN
        self.SYM_ADP = SYM_ADP
        self.GKI = GKI
        self.TKI = TKI
        self.PK = PK
        _dls_pcs_asyn_motor(P=self.P,M=self.M,PORT=self.PORT,ADDR=self.ADDR,DESC=self.DESC,MRES=0.001,VELO=self.VELO,PREC=self.PREC,EGU=self.EGU,TWV=self.TWV,
                            KP=self.KP,
                            TI=self.TI,
                            TD=self.TD,
                            T1=self.T1,
                            KE=self.KE,
                            KE2=self.KE2,
                            KFF=self.KFF,
                            KREI=self.KREI,
                            TAU=self.TAU,
                            ELIM=self.ELIM,
                            KDCC=self.KDCC,
                            SYM_MAN=self.SYM_MAN,
                            SYM_ADP=self.SYM_ADP,
                            GKI=self.GKI,
                            TKI=self.TKI,
                            PK=self.PK)

    # Once per instantiation
    def Initialise(self):
        print "# Configure Walter and Bai PCS8000 Axis"
        print "# pcsAxisConfig(%(CONTROLLER)s, %(ADDR)d)"
        print "pcsAxisConfig(%(PORT)s, %(ADDR)d,%(SLAVE_NO)d, %(FEEDBACK1)s,%(FEEDBACK2)s,%(SEN_MIN)f)" % self.__dict__

    # Arguments
    ArgInfo = makeArgInfo(__init__,
                          SLAVE = Ident("Slave number",pcsSlave),
                          P = Simple("Axis number", str),
                          M = Simple("Axis number", str),
                          ADDR = Simple("Axis number", int),
                          DESC = Simple("Axis number", str),
                          VELO = Simple("Axis number", float),
                          PREC = Simple("Axis number", int),
                          EGU = Simple("Axis number", str),
                          TWV = Simple("Axis number", float),
                          SEN_MIN = Simple("Minimum sensor value threshold", float),
                          FEEDBACK1 = Choice("Phys stream for primary feedback", ['phys'+str(i) for i in range(1,16)]),
                          FEEDBACK2 = Choice("Phys stream for secondary feedback", ['phys'+str(i) for i in range(1,16)]),
                          KP = Simple("PIDx controller, Kp.", float),
                          TI = Simple("PIDx controller, Ti.", float),
                          TD = Simple("PIDx controller, Td.", float),
                          T1 = Simple("PIDT controller, T1.", float),
                          KE = Simple("PIDV contr, controllergain.", float),
                          KE2 = Simple("PIDV contr, quadratic gain.", float),
                          KFF = Simple("PIDV contr, feedforward.", float),
                          KREI = Simple("PIDV contr, residual error integrator.", float),
                          TAU = Simple("PIDV contr, timeconstant for residual error integrator", float),
                          ELIM = Simple("PIDV contr, Errorlimit for residual error integrator.", float),
                          KDCC = Simple("PIDV contr, DC-correction.", float),
                          SYM_MAN = Simple("PIDV contr, Manual bal- ancer.", float),
                          SYM_ADP = Simple("PIDV contr, Adaptive bal- ancer.", float),
                          GKI = Simple("PIDV contr, Kickergain.", float),
                          TKI = Simple("PIDV contr, Timeconstant for kicker.", float),
                          PK = Simple("PIDV contr,Peakcontroller.", float))

