from __future__ import print_function
from builtins import str
from builtins import range
from builtins import object
import telnetlib
import socket
import requests

PW_HOST = "10.1.120.91"
PW_USER = "admin"
PW_PASS = "12345678"
ADAM_HOST = "10.1.120.92"
ADAM_PORT = 502
TM_HOST = "10.1.120.93"

POWER_MC = 0
POWER_STF = 1
POWER_CISCO = 2
POWER_PC = 3


class Power(object):
    """ *MCS power module* """

    def __init__(self, host=PW_HOST, user=PW_USER, password=PW_PASS):
        """ connect to IP power 9858DX """

        self.host = host
        self.user = user
        self.password = password
        self.url = 'http://' + user + ':' + password + '@' + host + '/set.cmd?cmd='

    def set_power(self, device, powerOn):
        """ set the power for a device """

        s = str(device + 1) + '='
        s += '1' if powerOn else '0'
        r = requests.get(self.url + 'setpower&p6' + s)

    def pulse_power(self, device, duration):
        """ turn off and on the power for a period """

        s = str(device + 1) + '=' + str(duration)
        r = requests.get(self.url + 'setpowercycle&p6' + s)

    def query(self):
        """ query current status """

        r = requests.get(self.url + 'getpower')
        idx = r.text.find('p61') + 4
        state = []
        for n in range(4):
            st = 0 if r.text[idx+n*7]=='0' else 1
            state.append(st)
        return state


class Adam6015(object):
    """ *MCS ADAM 6015 module* """

    def __init__(self, host=ADAM_HOST, port=ADAM_PORT):
        """ set IP for Adam 6015 modules """
        self.host = host
        self.port = port

    def query(self):
        """ Read data from Adam 6015 modules """

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((self.host, self.port))
        s.sendall('\x00\xef\x00\x00\x00\x06\x01\x04\x00\x00\x00\x07')
        data = s.recv(23, socket.MSG_WAITALL)
        s.close()

        if data[:9] != '\x00\xef\x00\x00\x00\x11\x01\x04\x0e':
            print("Receiving invalid data")
            exit()

        temp = [0.0] * 7
        j = 9
        for i in range(7):
            temp[i] = (ord(data[j]) * 256 + ord(data[j + 1])) / 65535.0 * 200.0 - 50.0
            j += 2
        return temp


class Telemetry(object):
    """ *MCS EBOX telemetry module* """

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
