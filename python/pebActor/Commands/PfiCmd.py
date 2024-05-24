#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys

class PfiCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('pfi', 'status', self.status),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_pfi", (1, 1),
                                        )
        
    @property
    def PfiDev(self):
        return self.actor.controllers['pfi']

    def status(self, cmd, doFinish=True):
        """Report PFI router status."""

        status = self.PfiDev.query()

        if status is True:
            cmd.inform(f"pfi_status='PFI is online'")
        else:
            cmd.inform(f"pfi_status='PFI is offline'")
        

        if doFinish:
            cmd.finish()

