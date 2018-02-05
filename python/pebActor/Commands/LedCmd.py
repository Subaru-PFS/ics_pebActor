#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys
import opscore.protocols.types as types
from opscore.utility.qstr import qstr

class LedCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('led', '@(on|off)', self.setPower),
            ('led', 'set [<periods>] [<dutycycle>]', self.setParameters),
            ('led', 'status', self.status),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_led", (1, 1),
                                        keys.Key("periods", types.Float(), help="Period in ms"),
                                        keys.Key("dutycycle", types.Float(), help="Duty cycle in %"),
                                        )
    @property
    def ledDev(self):
        return self.actor.controllers['led']

    def setPower(self, cmd):
        """ Turn on or off LED """

        cmdKeys = cmd.cmd.keywords
        powerOn = 'on' in cmdKeys
        if powerOn:
            self.ledDev.set_modeA()
            cmd.inform('ledpower=1')
        else:
            self.ledDev.power_off()
            cmd.inform('ledpower=0')
        cmd.finish()

    def setParameters(self, cmd):
        """ Set period(ms) and duty cycle(%) """

        cmdKeys = cmd.cmd.keywords
        if 'periods' in cmdKeys:
            periods = cmdKeys['periods'].values[0]
        else:
            periods = 100.0
        if 'dutycycle' in cmdKeys:
            dutycycle = cmdKeys['dutycycle'].values[0]
        else:
            dutycycle = 20.0

        self.ledDev.power_set(periods, dutycycle)
        cmd.inform('ledparam=%0.2f,%0.2f' % (periods, dutycycle))
        cmd.inform('ledpower=1')
        cmd.finish()

    def status(self, cmd, doFinish=True):
        """Report status """

        if doFinish:
            cmd.finish()

