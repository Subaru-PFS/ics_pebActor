from builtins import str
from builtins import range
from builtins import object
import logging
import telnetlib

TIME_OUT = 3

class led(object):
    """ PFI E-box led controller """

    def __init__(self, actor, name,
                 logLevel=logging.INFO,
                 host=None):
        """ connect to Arduino board """

        self.name = name
        self.actor = actor
        self.logger = logging.getLogger('led')

        if host is None:
            host = self.actor.config.get(self.name, 'host')
        self.host = host
        self.logger.warn('host: %s', self.host)        

    def _sendReq(self, req):
        """ Actually send the request. """

        tn = telnetlib.Telnet(self.host)
        self.logger.info('sent: %s', req)
        tn.write(str.encode(req))
        tn.read_until(b':', TIME_OUT)
        tn.close()

    def power_set(self, period, dutycycle):
        """ set period and duty cycle """
        # period in ms, duty cyle in %

        self._sendReq('p' + str(int(period)) + '\r')
        self._sendReq('d' + str(int(dutycycle * 10.23)) + '\r')

    def power_off(self):
        """ Turn off LED """

        self._sendReq('c\r')

    def set_modeA(self):
        """ Switch to mode A """
        # Defailt: turn on for 10.24us, turn off for 89.64us, period is 0.1ms

        self._sendReq('a\r')

    def set_modeB(self):
        """ Switch to mode B """
        # Defailt: turn on for 10.24ms, turn off for 89.60ms, period is 100ms

        self._sendReq('b\r')

    def start(self):
        pass

    def stop(self):
        pass

