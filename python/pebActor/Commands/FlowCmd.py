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
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_flow", (1, 1),
                                        )
        self.kFactor = float(self.actor.config.get('flow', 'kFactor'))

    @property
    def flowDev(self):
        return self.actor.controllers['flow']

    def status(self, cmd, doFinish=True):
        """Report flow meter status."""

        status = self.flowDev.query()

        # convert from Hz to Gal/min
        speed = status['FlowMeter'] / self.kFactor * 60

        # You need to format this as keywords...
        humidity = ','.join(["%0.2f" % s for s in [status['Humidity'], status['Temperature'], status['DewPoint']]])
        flow = '%.2f,%.1f' % (speed, status['FlowMeter'])
        leakage = '%d,%d' % (status['Leakage'], status['LeakageDisconnection'])
        cmd.inform('humidity=%s' % (humidity))
        cmd.inform('flow=%s' % (flow))
        cmd.inform('leakage=%s' % (leakage))

        if doFinish:
            cmd.finish()

