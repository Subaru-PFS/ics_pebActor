from builtins import range
from builtins import object
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
            host = self.actor.config.get(self.name, 'host')
        self.host = host
        self.logger.warn('host: %s', self.host)        

    def query(self):
        """ Read data from Arduino board """

        tn = telnetlib.Telnet(self.host)
        tn.write(b'Q\r')
        data = tn.read_until(b':', TIME_OUT)
        tn.close()
        res = data.decode('latin-1').split()
        return {
            'Temperature': float(res[2]),
            'Humidity': float(res[6]),
            'DewPoint': float(res[10]),
            'FlowMeter': float(res[14]),
            'Leakage': int(res[18][:1]),
            'LeakageDisconnection': int(res[22]),
            'ValveLockStatus': int(res[26])
        }

    def raw(self, cmdStr):
        """ Send an arbitrary command URL to the controller. """

        raise NotImplementedError('no raw command')

    def start(self, cmd=None):
        pass

    def stop(self, cmd=None):
        pass

