#!/usr/bin/env python

import logging
from twisted.internet import reactor

import actorcore.ICC

class OurActor(actorcore.ICC.ICC):
    def __init__(self, name,
                 productName=None, configFile=None,
                 modelNames=(),
                 debugLevel=30):

        """ Setup an Actor instance. See help for actorcore.ICC for details. """
        
        # This sets up the connections to/from the hub, the logger, and the twisted reactor.
        #
        actorcore.ICC.ICC.__init__(self, name, 
                                   productName=productName, 
                                   configFile=configFile,
                                   modelNames=modelNames)

        self.everConnected = False

        self.monitors = dict()
        self.statusLoopCB = self.statusLoop

    def connectionMade(self):
        # Called each time the hub connects to us.
        if self.everConnected is False:
            logging.info("Attaching all controllers...")
            self.allControllers = [s.strip()
                                   for s in self.config.get(self.name, 'startingControllers').split(',')]
            self.attachAllControllers()
            self.everConnected = True

    def statusLoop(self, controller):
        try:
            self.callCommand("%s status" % (controller))
        except:
            pass

        if self.monitors[controller] > 0:
            reactor.callLater(self.monitors[controller],
                              self.statusLoopCB,
                              controller)

    def monitor(self, controller, period, cmd=None):
        """ Arrange for 'status' to be called on a named controller. """
        
        if controller not in self.monitors:
            self.monitors[controller] = 0

        running = self.monitors[controller] > 0
        self.monitors[controller] = period

        if (not running) and period > 0:
            cmd.warn('text="starting %gs loop for %s"' % (self.monitors[controller],
                                                          controller))
            self.statusLoopCB(controller)
        else:
            cmd.warn('text="adjusted %s loop to %gs"' % (controller, self.monitors[controller]))
        #

# To work
def main():
    theActor = OurActor('peb', productName='pebActor')
    theActor.run()

if __name__ == '__main__':
    main()
