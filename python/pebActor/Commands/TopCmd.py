#!/usr/bin/env python

from builtins import object
import opscore.protocols.keys as keys
import opscore.protocols.types as types
from opscore.utility.qstr import qstr
import logging

class TopCmd(object):

    def __init__(self, actor):
        # This lets us access the rest of the actor.
        self.actor = actor

        # Declare the commands we implement. When the actor is started
        # these are registered with the parser, which will call the
        # associated methods when matched. The callbacks will be
        # passed a single argument, the parsed and typed command.
        #
        self.vocab = [
            ('ping', '', self.ping),
            ('status', '', self.status),
            ('monitor', '<controllers> <period>', self.monitor),
        ]

        # Define typed command arguments for the above commands.
        self.keys = keys.KeysDictionary("peb_peb", (1, 1),
                                        keys.Key("controllers", types.String()*(1,None),
                                                 help='the names of 1 or more controllers to load'),
                                        keys.Key("period", types.Int(),
                                                 help='the period to sample at.'),
                                        )
        self.logger = logging.getLogger('peb')

    def ping(self, cmd):
        """Query the actor for liveness/happiness."""

        cmd.warn("text='I am an empty and fake actor'")
        cmd.finish("text='Present and (probably) well'")

    def status(self, cmd):
        """Report actor status and version."""

        self.actor.sendVersionKey(cmd)
        type = self.actor.actorConfig['eboxtype']
        
        cmd.inform(f'text="Current E-box Setting = {type}"')
        cmd.inform('text="Present!"')
        cmd.finish()

    def monitor(self, cmd):
        """Enable/disable/adjust period controller monitors. """
        
        period = cmd.cmd.keywords['period'].values[0]
        controllers = cmd.cmd.keywords['controllers'].values

        knownControllers = []
        for c in self.actor.actorConfig['controllers']:
            #c = c.strip()
            knownControllers.append(c)

        foundOne = False

        if controllers[0] == 'all':
            controllers = self.actor.actorConfig['controllers']
        
        self.logger.info(f'Monitoring controllers : {controllers}')
        
        for c in controllers:
            if c not in knownControllers:
                cmd.warn('text="not starting monitor for %s: unknown controller"' % (c))
                continue

            self.actor.monitor(c, period, cmd=cmd)
            foundOne = True

        if foundOne:
            cmd.finish()
        else:
            cmd.fail('text="no controllers found"')

        
