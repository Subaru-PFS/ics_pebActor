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
            ('led', '@raw', self.raw),
            ('led', '@(on|flash|off)', self.setPower),
            ('led', '@(config|configflash) [<ledperiod>] [<dutycycle>]', self.configParameters),
            ('led', 'status', self.status),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_led", (1, 2),
                                        keys.Key("ledperiod", types.Int(), help="Period in us"),
                                        keys.Key("dutycycle", types.Float(), help="Duty cycle in %"),
                                        )
    @property
    def ledDev(self):
        return self.actor.controllers['led']

    def raw(self, cmd):
        """ Send a raw command to the led controller """

        cmdTxt = cmd.cmd.keywords['raw'].values[0]
        ret = self.ledDev.raw(cmdTxt)
        cmd.inform('text="raw return: %s"' % (ret))
        self.status(cmd)

    def setPower(self, cmd):
        """ Turn on or off LED """

        cmdKeys = cmd.cmd.keywords
        powerOn = 'on' in cmdKeys
        if 'on' in cmdKeys:
            self.ledDev.set_modeA()
        elif 'flash' in cmdKeys:
            self.ledDev.set_modeB()
        else:
            self.ledDev.power_off()
        self.status(cmd)

    def configParameters(self, cmd):
        """ Configure period(us) and duty cycle(%) """

        cmdKeys = cmd.cmd.keywords
        if 'ledperiod' in cmdKeys:
            period = cmdKeys['ledperiod'].values[0]
        else:
            period = None
        if 'dutycycle' in cmdKeys:
            dutycycle = cmdKeys['dutycycle'].values[0]
        else:
            dutycycle = None

        if 'config' in cmdKeys:
            self.ledDev.config_modeA(period, dutycycle)
        else:
            self.ledDev.config_modeB(period, dutycycle)
        self.status(cmd)

    def status(self, cmd, doFinish=True):
        """Report status """

        (period, dutycycle, aperiod, adutycycle, bperiod, bdutycycle) = self.ledDev.query()
        cmd.inform('ledperiod=%d,%d,%d' % (period, aperiod, bperiod))
        cmd.inform('dutycycle=%0.1f,%0.1f,%0.1f' % (dutycycle, adutycycle, bdutycycle))

        if doFinish:
            cmd.finish()

