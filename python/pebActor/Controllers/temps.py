from builtins import range
from builtins import object
import logging
import socket

class temps(object):
    """ PFI E-box temperatures """

    def __init__(self, actor, name,
                 logLevel=logging.INFO,
                 hosts=None, port=None):
        """ connect to Adam 6015 box """

        self.name = name
        self.actor = actor
        self.logger = logging.getLogger('temps')

        if hosts is None:
            hosts = self.actor.config.get(self.name, 'hosts')
        if port is None:
            port = int(self.actor.config.get(self.name, 'port'))
        self.hosts = [s.strip() for s in hosts.split(',')]
        self.port = port
        self.logger.warn('hosts,port: %s,%d', ' '.join(self.hosts), self.port)        

    def query(self):
        """ Read data from Adam 6015 modules """

        temps = []
        idx = 0
        for host in self.hosts:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, self.port))
            req = b'\x00\xef\x00\x00\x00\x06\x01\x04\x00\x00\x00\x07'
            self.logger.info('send: %r', req)
            s.sendall(req)
            data = s.recv(23, socket.MSG_WAITALL)
            self.logger.debug('recv: %r', data)
            s.close()

            if data[:9] != b'\x00\xef\x00\x00\x00\x11\x01\x04\x0e':
                print("Receiving invalid data")
                exit()
            data = data.decode('latin-1')

            j = 9
            for i in range(7):
                temps.append((ord(data[j]) * 256 + ord(data[j + 1])) / 65535.0 * 200.0 - 50.0)
                j += 2
            # switch the first two element since AG3 anf AG4 temp sensors are swapped
            temps[0], temps[1]= temps[1], temps[0]
        return temps

    def raw(self, cmdStr):
        """ Send an arbitrary command URL to the controller. """

        raise NotImplementedError('no raw command')

    def start(self):
        pass

    def stop(self):
        pass

