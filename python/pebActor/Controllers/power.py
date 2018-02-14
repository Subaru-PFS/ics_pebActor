from builtins import hex
from builtins import str
from builtins import range
from builtins import object
import telnetlib
import logging

POWER_AGC1 =    int('0000000000001',base=2)
POWER_AGC2 =    int('0000000000010',base=2)
POWER_AGC3 =    int('0000000000100',base=2)
POWER_AGC4 =    int('0000000001000',base=2)
POWER_AGC5 =    int('0000000010000',base=2)
POWER_AGC6 =    int('0000000100000',base=2)
POWER_AGC =     int('0000000111111',base=2)
POWER_LEAKAGE = int('0000001000000',base=2)
POWER_ADAM =    int('0000010000000',base=2)
POWER_BOARDB =  int('0000100000000',base=2)
POWER_BOARDC =  int('0001000000000',base=2)
POWER_USB1 =    int('0010000000000',base=2)
POWER_USB2 =    int('0100000000000',base=2)
POWER_USB =     int('0110000000000',base=2)
POWER_ALL =     int('0111111111111',base=2)
POWER_SWITCH =  int('1000000000000',base=2)

TIME_OUT = 3

class power(object):
    """ *PFI power module* """

    deviceIds = {'agc':POWER_AGC, 'leakage':POWER_LEAKAGE,
                 'adam':POWER_ADAM, 'switch':POWER_SWITCH,
                 'usb':POWER_USB, 'boardb':POWER_BOARDB,
                 'boardc':POWER_BOARDC}

    def __init__(self, actor, name,
                 logLevel=logging.INFO,
                 host=None):
        """ connect to Arduino board """

        self.name = name
        self.actor = actor
        self.logger = logging.getLogger('power')

        if host is None:
            host = self.actor.config.get(self.name, 'host')
        self.host = host
        self.logger.warn('host: %s', self.host)

    def _sendReq(self, req):
        """ Actually send the request. """

        tn = telnetlib.Telnet(self.host)
        self.logger.info('sent: %s', req)
        tn.write(str.encode(req))
        res = tn.read_until(b':', TIME_OUT).decode('latin-1')
        tn.close()
        self.logger.info('get: %s', res)
        return res

    def raw(self, cmdStr):
        """ Send an arbitrary command to the controller. """

        return self._sendReq(cmdStr + "\r")

    def _set_power(self, devices, powerOn):
        """ set the power for devices """

        devStr = hex(devices).upper()[2:].zfill(4)
        if powerOn:
            setStr = devStr
        else:
            setStr = "0000"
        self._sendReq("S" + devStr + setStr + "\r")

    def _bounce_power(self, devices):
        """ turn off and on the power for a second """

        devStr = hex(devices).upper()[2:].zfill(4)
        self._sendReq("P" + devStr + "\r")

    def _build_deviceIds(self, devices, ids=None):
        """ Construct device IDs """

        dids = 0
        if devices == 'agc' and ids != None:
            for i in [1, 2, 3, 4, 5, 6]:
                if i in ids:
                    dids |= POWER_AGC1 << (i - 1)
        elif devices == 'usb' and ids != None:
            for i in [1, 2]:
                if i in ids:
                    dids |= POWER_USB1 << (i - 1)
        else:
            dids |= self.deviceIds[devices]
        return dids

    def set_power(self, devices, powerOn, ids=None):
        """ set the power for devices """

        self._set_power(self._build_deviceIds(devices, ids), powerOn)

    def bounce_power(self, devices, ids=None):
        """ turn off and on the power for one second """

        self._bounce_power(self._build_deviceIds(devices, ids))
    
    def query(self):
        """ query all devices """

        res = self._sendReq("Q\r")
        return bin(int(res[:4], 16))[2:]

    def query_device(self, device):
        """ query a device """

        res = self._sendReq("Q\r")
        return True if (int(res[:4], 16) & device) > 0 else False

    def all_on(self):
        """ Turn on all devices except for switch """
        self._set_power(POWER_ALL, powerOn=True)

    def all_off(self):
        """ Turn off all devices except for switch """
        self._set_power(POWER_ALL, powerOn=False)

    def adam_on(self):
        """ Turn on ADAM6015 """
        self._set_power(POWER_ADAM, powerOn=True)

    def adam_off(self):
        """ Turn off ADAM6015 """
        self._set_power(POWER_ADAM, powerOn=False)

    def adam_pulse(self):
        """ Reset ADAM6015 """
        self._bounce_power(POWER_ADAM)

    def leakage_on(self):
        """ Turn on leakage detector """
        self._set_power(POWER_LEAKAGE, powerOn=True)

    def leakage_off(self):
        """ Turn off leakage detector """
        self._set_power(POWER_LEAKAGE, powerOn=False)

    def leakage_pulse(self):
        """ Reset leakage detector """
        self._bounce_power(POWER_LEAKAGE)

    def usb_on(self):
        """ Turn on USB hubs """
        self._set_power(POWER_USB1 | POWER_USB2, powerOn=True)

    def usb_off(self):
        """ Turn off USB hubs """
        self._set_power(POWER_USB1 | POWER_USB2, powerOn=False)

    def usb_pulse(self):
        """ Reset USB hubs """
        self._bounce_power(POWER_USB1 | POWER_USB2)

    def agc_on(self):
        """ Turn on AG cameras """
        self._set_power(POWER_AGC, powerOn=True)

    def agc_off(self):
        """ Turn off AG cameras """
        self._set_power(POWER_AGC, powerOn=False)

    def agc_pulse(self):
        """ Reset AG cameras """
        self._bounce_power(POWER_AGC)

    def boardB_pulse(self):
        """ Reset BOARD B """
        self._bounce_power(POWER_AGC)

    def boardC_pulse(self):
        """ Reset BOARD C """
        self._bounce_power(POWER_AGC)

    def switch_pulse(self):
        """ Reset network switch """
        self._bounce_power(POWER_SWITCH)

    def start(self):
        pass

    def stop(self):
        pass

