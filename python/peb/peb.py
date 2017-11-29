import telnetlib
import socket

PW_HOST = "10.1.120.11"
TM_HOST = "10.1.120.12"
LED_HOST = "10.1.120.13"
ADAM_HOSTS = ["10.1.120.21", "10.1.120.22", "10.1.120.23"]
ADAM_PORT = 502
TIME_OUT = 3

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
POWER_ALL =     int('0111111111111',base=2)
POWER_SWITCH =  int('1000000000000',base=2)

class PebPower(object):
    """ *PFI EBOX power module* """

    def __init__(self, host=PW_HOST):
        """ connect to arduino board and check current power status """

        self.host = host

    def set_power(self, devices, powerOn):
        """ set the power for devices """

        tn = telnetlib.Telnet(self.host)
        devStr = hex(devices).upper()[2:].zfill(4)
        if powerOn:
            setStr = devStr
        else:
            setStr = "0000"
        tn.write("S" + devStr + setStr + "\r")
        res = tn.read_until(":", TIME_OUT)
        tn.close()

    def pulse_power(self, devices):
        """ turn off the power for one second to reset devices """

        tn = telnetlib.Telnet(self.host)
        devStr = hex(devices).upper()[2:].zfill(4)
        tn.write("P" + devStr + "\r")
        res = tn.read_until(":", TIME_OUT)
        tn.close()

    def query(self):
        """ query current status """

        tn = telnetlib.Telnet(self.host)
        tn.write("Q\r")
        res = tn.read_until(":", TIME_OUT)
        tn.close()
        return "0x" + res[:4]

    def query_device(self, device):
        """ query a device """

        tn = telnetlib.Telnet(self.host)
        tn.write("Q\r")
        res = tn.read_until(":", TIME_OUT)
        tn.close()
        return True if (int(res[:4], 16) & device) > 0 else False

    def all_on(self):
        """ Turn on all devices except for switch """
        self.set_power(POWER_ALL, powerOn=True)

    def all_off(self):
        """ Turn off all devices except for switch """
        self.set_power(POWER_ALL, powerOn=False)

    def adam_on(self):
        """ Turn on ADAM6015 """
        self.set_power(POWER_ADAM, powerOn=True)

    def adam_off(self):
        """ Turn off ADAM6015 """
        self.set_power(POWER_ADAM, powerOn=False)

    def adam_pulse(self):
        """ Reset ADAM6015 """
        self.pulse_power(POWER_ADAM)

    def leakage_on(self):
        """ Turn on leakage detector """
        self.set_power(POWER_LEAKAGE, powerOn=True)

    def leakage_off(self):
        """ Turn off leakage detector """
        self.set_power(POWER_LEAKAGE, powerOn=False)

    def leakage_pulse(self):
        """ Reset leakage detector """
        self.pulse_power(POWER_LEAKAGE)

    def usb_on(self):
        """ Turn on USB hubs """
        self.set_power(POWER_USB1 | POWER_USB2, powerOn=True)

    def usb_off(self):
        """ Turn off USB hubs """
        self.set_power(POWER_USB1 | POWER_USB2, powerOn=False)

    def usb_pulse(self):
        """ Reset USB hubs """
        self.pulse_power(POWER_USB1 | POWER_USB2)

    def agc_on(self):
        """ Turn on AG cameras """
        self.set_power(POWER_AGC, powerOn=True)

    def agc_off(self):
        """ Turn off AG cameras """
        self.set_power(POWER_AGC, powerOn=False)

    def boardB_pulse(self):
        """ Reset BOARD B """
        self.pulse_power(POWER_AGC)

    def boardC_pulse(self):
        """ Reset BOARD C """
        self.pulse_power(POWER_AGC)

    def switch_pulse(self):
        """ Reset network switch """
        self.pulse_power(POWER_SWITCH)


class PebTelemetry(object):
    """ *PFI EBOX telemetry module* """

    def __init__(self, host=TM_HOST):
        """ set IP for arduino board """
        self.host = host

    def query(self):
        """ Read data from Telemetry sensors """

        tn = telnetlib.Telnet(self.host)
        tn.write("Q\r")
        res = tn.read_until(":", TIME_OUT).split()
        tn.close()
        return {
            'Temperature': float(res[2]),
            'Humidity': float(res[6]),
            'DewPoint': float(res[10]),
            'FlowMeter': float(res[14]),
            'Leakage': int(res[18][:1]),
            'LeakageDisconnection': int(res[20])
        }


class Adam6015(object):
    """ *PFI ADAM 6015 module* """

    def __init__(self, hosts=ADAM_HOSTS, port=ADAM_PORT):
        """ set IP for Adam 6015 modules """
        self.hosts = hosts
        self.port = port

    def query(self):
        """ Read data from Adam 6015 modules """

        data = []
        for host in self.hosts:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, self.port))
            s.sendall('\x00\xef\x00\x00\x00\x06\x01\x04\x00\x00\x00\x07')
            data = s.recv(23, socket.MSG_WAITALL)
            s.close()

            if data[:9] != '\x00\xef\x00\x00\x00\x11\x01\x04\x0e':
                print "Receiving invalid data"
                exit()

            temp = [0.0] * 7
            j = 9
            readings = []
            for i in range(7):
                temp[i] = (ord(data[j]) * 256 + ord(data[j + 1])) / 65535.0 * 200.0 - 50.0
                j += 2
            readings.append(temp)
        return readings
