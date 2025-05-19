#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys

class FlowCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('flow', 'status', self.status),
            ('flow', '@valve @(open|close)', self.valve),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_flow", (1, 1),
                                        )
        #self.kFactor = float(self.actor.config.get('flow', 'kFactor'))
        self.kFactor  = self.actor.actorConfig['flow']['kfactor']
    @property
    def flowDev(self):
        return self.actor.controllers['flow']

    def valve(self, cmd):
        """ Open or close flow valve. """

        cmdKeys = cmd.cmd.keywords
        doOpen = 'open' in cmdKeys

        self.flowDev.openClose(doOpen)
        self.status(cmd)

    def status(self, cmd, doFinish=True):
        """Report flow meter status."""

        status = self.flowDev.query()

        # convert from Hz to Gal/min
        speed = status['FlowMeter'] / self.kFactor * 60

        # You need to format this as keywords...
        humidity = ','.join(["%0.2f" % s for s in [status['Humidity'], status['Temperature'], status['DewPoint']]])
        flow = '%.2f,%.1f' % (speed, status['FlowMeter'])
        leakage = '%d,%d' % (status['Leakage'],status['LeakageDisconnection'])
        valve_status = '%d' % status['ValveLockStatus']

        cmd.inform('humidity=%s' % (humidity))
        cmd.inform('flow=%s' % (flow))
        cmd.inform('leakage=%s' % (leakage))
        cmd.inform('valve_status=%s' % (valve_status))
        #cmd.inform(f"valveLock = {status['ValveLockStatus']}")

        if doFinish:
            cmd.finish()

