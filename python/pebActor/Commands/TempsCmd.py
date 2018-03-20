#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys

class TempsCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('temps', 'status', self.status),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_temps", (1, 1),
                                        )
    @property
    def tempsDev(self):
        return self.actor.controllers['temps']

    def status(self, cmd, doFinish=True):
        """Report temperature from RTD sensors."""

        status = self.tempsDev.query()

        # You need to format this as keywords...
        temps = ','.join(["%0.2f" % s for s in status])
        cmd.inform('temps=%s' % (temps))

        if doFinish:
            cmd.finish()

