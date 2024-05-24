from builtins import range
from builtins import object
import numpy as np
import logging
import socket

TIME_OUT = 20

class pfi(object):
    """ PFI E-box flow meter, leakage dector and humidity sensor """

    def __init__(self, actor, name,
                 logLevel=logging.INFO,
                 host=None):
        """ connect to Arduino board """

        self.name = name
        self.actor = actor
        self.logger = logging.getLogger('pfi')

        if host is None:
            #host = self.actor.config.get(self.name, 'host')
            host = self.actor.actorConfig['pfi']['host']
        self.port = 80
        self.host = host
        self.logger.info('PFI monitor host: %s', self.host)        

    def query(self):
        """ Check the socket connection """

        try:
            with socket.create_connection((self.host, self.port), timeout=5) as sock:
                self.logger.INFO(f"Connection to {self.host} on port {self.port} successful")
            return True
        except (socket.timeout, socket.error) as e:
            self.logger.INFO(f"Connection to {self.host} on port {self.port} failed: {e}")
            return False
        

    def raw(self, cmdStr):
        """ Send an arbitrary command URL to the controller. """

        raise NotImplementedError('no raw command')

    def start(self, cmd=None):
        pass

    def stop(self, cmd=None):
        pass

