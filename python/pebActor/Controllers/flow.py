from builtins import range
from builtins import object
import numpy as np
import logging
import telnetlib

TIME_OUT = 20

class flow(object):
    """ PFI E-box flow meter, leakage dector and humidity sensor """

    def __init__(self, actor, name,
                 logLevel=logging.INFO,
                 host=None):
        """ connect to Arduino board """

        self.name = name
        self.actor = actor
        self.logger = logging.getLogger('flow')

        if host is None:
            #host = self.actor.config.get(self.name, 'host')
            host = self.actor.actorConfig['flow']['host']
        self.host = host
        self.logger.info('Flow monitor host: %s', self.host)        

    def query(self):
        """ Read data from Arduino board """

        tn = telnetlib.Telnet(self.host)
        flowRaw = []

        for i in range(10):
            tn.write(b'Q\r')
            data = tn.read_until(b':', TIME_OUT)
        
            res = data.decode('latin-1').split()
            flowRaw.append(float(res[14]))
        tn.close()    
        flowRaw = np.array(flowRaw)
        
        #eboxType = self.actor.config.get('peb', 'eboxtype')
        # eboxType = self.actor.actorConfig.['eboxtype']
        # if eboxType == 'oldebox':
        #     self.logger.info(f'Loading setting for old Ebox')        
        #     LeakageDisconnection = int(res[22])
        #     ValveLockStatus = int(res[26])
        # else:
        #     LeakageDisconnection = int(res[22])
        #     ValveLockStatus = int(res[26])
        LeakageDisconnection = int(res[22])
        ValveLockStatus = int(res[26])

        return {
            'Temperature': float(res[2]),
            'Humidity': float(res[6]),
            'DewPoint': float(res[10]),
            #'FlowMeter': float(res[14]),
            'FlowMeter': float(np.median(flowRaw)),
            'Leakage': int(res[18][:1]),
            'LeakageDisconnection': LeakageDisconnection,
            'ValveLockStatus': ValveLockStatus
        }

    def openClose(self, doOpen):
        """Open or close the flow valve"""

        tn = telnetlib.Telnet(self.host)

        cmdInt = int(not doOpen)
        cmdStr = bytes(f':C{cmdInt}\r', 'latin-1')

        tn.write(cmdStr)
        data = tn.read_until(b':', TIME_OUT)
        tn.close()

        res = data.decode('latin-1').split()
        if res[0] != 'valveSafeLock':
            self.logger.warning(f'flow controller returned: {res}')
        return int(res[3])

    def raw(self, cmdStr):
        """ Send an arbitrary command URL to the controller. """

        raise NotImplementedError('no raw command')

    def start(self, cmd=None):
        pass

    def stop(self, cmd=None):
        pass

