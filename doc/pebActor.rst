ics_pebActor
============

PFI Electronic Box Actor

Top commands
------------

peb connect controller=\"SSS\" [name=\"SSS\"]
    Reload controller objects.
peb disconnect controller=\"SSS\"
    Disconnect the given, or all, controller objects.
peb exitexit
    Brutal exit when all else has failed.
peb help [full] [cmds=???] [pageWidth=N] [html]
    Return a summary of all commands, or the complete help string for the specified commands.
peb ipdb
    Try to start some debugger.
peb ipython
    Try to start a subshell.
peb monitor controllers=??? period=N
    Enable/disable/adjust period controller monitors.
peb ping
    Query the actor for liveness/happiness.
peb reload [cmds=???]
    If cmds argument is defined, reload the listed command sets, otherwise reload all command sets.
peb reloadConfiguration
    Reload the configuration.
peb status
    Report actor status and version.
peb version
    Return a version keyword.


Power module(controller=power)
------------------------------

peb power @raw
    Send a raw command to the power controller.
peb power on|off|bounce agc|leakage|adam|boardb|boardc|usb|switch [ids=\"SSS\"]
    Set or bounce power to a device.
peb power status
    Report current power status.

ADAM 6015(controller=temps)
---------------------------

peb temps status
    Report temperature from RTD sensors.

Flow meter(controller=flow)
---------------------------

peb flow status
    Report flow meter status.

LED module(controller=led)
--------------------------

peb led @raw
    Send a raw command to the led controller.
peb led on|flash|off
    Turn on or off LED.
peb led config|configflash [ledperiod=N] [dutycycle=FF.F]
    Configure period(us) and duty cycle(%).
peb led status
    Report LED status.

Actor keywords
--------------

Defined in ics_actorkeys/python/actorkeys/peb.py

::

  KeysDictionary('peb', (1, 2),
                 Key("text", String(help="text for humans")),
                 Key("version", String(help="EUPS/git version")),
                 Key('temps', Float()*21,
                     help='PFI Ebox temperatures'),
                 Key('humidity', Float()*3,
                     help='PFI Ebox humidity, temperature and dew point'),
                 Key('flow', Float(),
                     help='PFI Ebox flow meter'),
                 Key('leakage', Int()*2,
                     help='PFI Ebox leakage and detector disconnect status'),
                 Key('ledperiod', Int()*3,
                     help='PFI Ebox LED peroid in us [current, on, flash]'),
                 Key('dutycycle', Float()*3,
                     help='PFI Ebox LED dutycycle in % [current, on, flash]'),
                 Key('power', Int()*13,
                     help='PFI Ebox power status'),
  )

Examples
--------

Query temps controller(ADAM 6015)

::

  peb temps status

Query flow meter

::

  peb flow status

Connect power controller

::

  peb connect controller=power

Turn on power for all AG cameras

::

  peb power on agc

Query power controller

::

  peb power status

Update status every 5s for temps and flow controllers

::

  peb monitor controllers=temps,flow period=5

Turn off power for AG camera #1, #2, #5

::

  peb power off agc ids=125

Config LED on parameters(period: 100us ,duty cycle: 12%)

::

  peb led config ledperiod=100 dutycycle=12.0

Turn on LED

::

  peb led on

Config LED flash parameters(period: 100ms ,duty cycle: 15%)

::

  peb led configflash ledperiod=100000 dutycycle=15.0

Flash LED

::

  peb led flash

Turn off LED

::

  peb led off
