#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys
import opscore.protocols.types as types
from opscore.utility.qstr import qstr

class PowerCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('power', '@raw', self.raw),
            ('power', '@(on|off|bounce) @(agc|leakage|adam|switch|usb|boardb|boardc)', self.setPower),
            ('power', 'status', self.status),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_power", (1, 1),
                                        )
    @property
    def powerDev(self):
        return self.actor.controllers['power']

    def raw(self, cmd):
        """ Send a raw command to the power controller """

        cmdTxt = cmd.cmd.keywords['raw'].values[0]
        ret = self.powerDev.raw(cmdTxt)
        cmd.inform('text="raw return: %s"' % (ret))
        self.status(cmd)

    def status(self, cmd, doFinish=True):
        """Report camera status and actor version. """

        status = self.powerDev.query()

        # You need to format this as keywords...
        cmd.inform('power=%s' % status)

        if doFinish:
            cmd.finish()

    def setPower(self, cmd):
        """ Set or bounce power to a device """

        cmdKeys = cmd.cmd.keywords
        for name in 'boardb', 'boardc', 'switch', 'agc', 'leakage', 'adam', 'usb':
            if name in cmdKeys:
                deviceName = name
                break

        if 'bounce' in cmdKeys:
            self.powerDev.bounce_power(deviceName)
        elif deviceName in ('boardb', 'boardc', 'switch'):
            cmd.warn('%s only support [bounce] option' % deviceName)
        else:
            powerOn = 'on' in cmdKeys
            self.powerDev.set_power(deviceName, powerOn)

        self.status(cmd)

