// LED diming board
// by Enos and ChihYi 2017/11/20

#include <SPI.h>
#include <Ethernet.h>
#include "TimerOne.h"
#include <avr/wdt.h>

#define BOOTLOADER //Comment this line if you are not using bootloader
//#define DEBUG   //Uncomment this line for debug output
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:

byte mac[] = 
{
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(10, 1, 164, 99);

// telnet defaults to port 23
EthernetServer g_server(23);
EthernetClient g_client;
boolean connected = false;
unsigned long last_active;
#define TELNET_TIMEOUT 10000
#define G_PIN 9

String g_strcmd;
long g_aduty; // duty
long g_aperiod; // period
long g_bduty; // duty
long g_bperiod; // period
long g_cduty; // duty
long g_cperiod; // period

// setup
void setup() {

#ifndef BOOTLOADER
  // Clear the reset bit
  MCUSR &= ~_BV(WDRF);
  // Disable the WDT
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  WDTCSR = 0;
#endif

#ifdef DEBUG
  Serial.begin(9600);
  // while the serial stream is not open, do nothing:
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif

  Ethernet.begin(mac, ip);
  // print your local IP address:
  DPRINTLN(Ethernet.localIP());
  
  g_server.begin();
  delay(1000);

  g_aduty = 105; // 10% duty
  g_aperiod = 100; // 100000 = 100ms 1000 = 1ms

  g_bduty = 105; // 10% duty
  g_bperiod = 100000; // 100000 = 100ms

  Timer1.initialize(100000); // the timer's period here (in microseconds)
  Timer1.pwm(G_PIN, 0); // setup pwm on pin 9, 0% duty cycle

  g_strcmd = String("");
  wdt_enable(WDTO_4S);
}

void parsing() {
  char str[256];

  if (g_strcmd == "a") {
    Timer1.setPeriod(g_aperiod);    
    Timer1.setPwmDuty(G_PIN, g_aduty);
    DPRINT("a ");
    DPRINT(g_aperiod);
    DPRINT(" ");
    DPRINTLN(g_aduty);
    g_cperiod = g_aperiod;
    g_cduty = g_aduty;
  } else if (g_strcmd == "b") {
    Timer1.setPeriod(g_bperiod);
    Timer1.setPwmDuty(G_PIN, g_bduty);   
    DPRINT("b ");
    DPRINT(g_bperiod);
    DPRINT(" ");
    DPRINTLN(g_bduty);
    g_cperiod = g_bperiod;
    g_cduty = g_bduty;
  } else if (g_strcmd == "c") {
    Timer1.setPwmDuty(G_PIN, 0);   
    DPRINTLN("c");
    g_cperiod = 0;
    g_cduty = 0;
  } else if (g_strcmd.charAt(0) == 'f') {
    g_aperiod = (long)g_strcmd.substring(5).toInt();
    g_aduty = (long)g_strcmd.substring(1, 5).toInt();
    DPRINT("f ");
    DPRINT(g_aperiod);
    DPRINT(" ");
    DPRINTLN(g_aduty);
  } else if (g_strcmd.charAt(0) == 'g') {
    g_bperiod = (long)g_strcmd.substring(5).toInt();
    g_bduty = (long)g_strcmd.substring(1, 5).toInt();
    DPRINT("g ");
    DPRINT(g_bperiod);
    DPRINT(" ");
    DPRINTLN(g_bduty);
  } else if (g_strcmd == "z") {
    g_client.stop();
  } else if (g_strcmd == "rst") {
    // command to test reset function
    DPRINTLN("Reset");
    delay(5000);
  } else if (g_strcmd == "q") {
    sprintf(str, "%ld,%ld,%ld,%ld,%ld,%ld\n", g_cperiod, g_cduty, g_aperiod, g_aduty, g_bperiod, g_bduty);
    g_client.write(str);
  } else {
    DPRINTLN(g_strcmd);
    g_client.write("unknown\n");
  }
}

void loop() {
  char c = 0;
  char t = ':';
  char r = 0x0d;
  EthernetClient g_client_new;

  // Allow only one connection
  if(connected) {
    if(!g_client.connected()) {
      g_client.stop();
      connected = false;
      DPRINTLN("Close connection");
    } else if ((millis() - last_active) > TELNET_TIMEOUT) {
      g_client.write("Bye\n");
      g_client.stop();
      connected = false;
      DPRINTLN("Connection timeouted");
    }
  }
  if(!connected && g_server.available()) {
    connected = true;
    g_client = g_server.available();
    last_active = millis();
    DPRINTLN("New connection");
  }
  if(connected) {
    while (g_client.available()) {
      c = g_client.read();
      if (c == r) {
        parsing(); // parsing command
        g_strcmd = ""; // empty cmd buffer
        g_client.write(t);
      } else if ((c >= 0x61 && c <= 0x7A) || (c >= 0x30 && c <= 0x39) || c == 0x2E) {
        g_strcmd += c;
      }
      last_active = millis();
    }
  }

  wdt_reset();
}

