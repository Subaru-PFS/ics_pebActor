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
            #host = self.actor.config.get(self.name, 'host')
            host = self.actor.actorConfig['led']['host']
        self.host = host
        self.logger.info('LED Arduino host: %s', self.host)        

    def _sendReq(self, req):
        """ Actually send the request. """

        tn = telnetlib.Telnet(self.host)
        self.logger.info('sent: %s', req)
        tn.write(str.encode(req))
        res = tn.read_until(b':', TIME_OUT).decode('latin-1').split()[0]
        tn.close()
        return res

    def raw(self, cmdStr):
        """ Send an arbitrary command to the controller. """

        return self._sendReq(cmdStr + '\r')

    def config_modeA(self, period=None, dutycycle=None):
        """ set period(us) and duty cycle(%) for mode A """

        if period is None:
            #period = int(self.actor.config.get(self.name, 'aperiod'))
            period = self.actor.actorConfig['led']['aperiod']
        if dutycycle is None:
            #dutycycle = float(self.actor.config.get(self.name, 'adutycycle'))
            dutycycle = self.actor.actorConfig['led']['adutycycle']
        
        self.logger.info(f'Setting period = {period} with duty cycle = {dutycycle}')        

        self._sendReq('f' + str(int(dutycycle * 10.23)).zfill(4) + str(int(period)) + '\r')

    def config_modeB(self, period=None, dutycycle=None):
        """ set period(us) and duty cycle(%) for mode B """

        if period is None:
            #period = int(self.actor.config.get(self.name, 'bperiod'))
            period = self.actor.actorConfig['led']['bperiod']
        if dutycycle is None:
            #dutycycle = float(self.actor.config.get(self.name, 'bdutycycle'))
            dutycycle = self.actor.actorConfig['led']['bdutycycle']
        self._sendReq('g' + str(int(dutycycle * 10.23)).zfill(4) + str(int(period)) + '\r')

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

    def query(self):
        """ Query current LED status """

        res = self._sendReq('q\r')
        self.logger.info('received: %s', res)
        res = res.split(',')
        period = int(res[0])
        dutycycle = float(res[1]) / 10.23
        aperiod = int(res[2])
        adutycycle = float(res[3]) / 10.23
        bperiod = int(res[4])
        bdutycycle = float(res[5]) / 10.23
        return (period, dutycycle, aperiod, adutycycle, bperiod, bdutycycle)

    def start(self, cmd=None):
        pass

    def stop(self, cmd=None):
        pass
