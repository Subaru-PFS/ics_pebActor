# ics_pebActor
ICS - PEB: PFI Electric Box (EBox) interface

This is a python module to access EBox devices.



# Note on Arduino IDE of Mac
Arduino IDE version 1.8.10 and above uses new version of Ethernet library.  The new version of Ethernet library removed 
many functions, which will cause compiling error.  The solution is use third-paty [Ethernet library](https://github.com/masterx1981/Ethernet).  However, the Windows version of Arduino Appilicaiton uses very high security managment method, which cause the update very diffcult.  The best solution is to use a Macbook or manully install the package downloaded from official site.  Once you installed Arduino IDE on a Mac notebook, go to 

~/Application/Arduino.app/Contents/Java/libraries

Then, remove the default Ethernet library.  Put all downloaded library in the location of sketch code.  
