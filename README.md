# ics_pebActor
ICS - PEB: PFI Electric Box (EBox) interface

This is a python module to access EBox devices.



# Note on Arduino IDE of Mac
Arduino IDE 1.8.10 and above uses new version of Ethernet library.  The new version of Ethernet library removed 
may functions, which will cause compiling error.  The solution is use third-paty [Ethernet library](https://github.com/masterx1981/Ethernet).  Once you installed Arduino IDE, go to 

~/Application/Arduino.app/Contents/Java/libraries

Then, remove the default Ethernet library.
