
// power control board
// by Enos and ChihYi 2017/08/23
//
// Modified by Chi-Hung Yan 2021/04/03
//
//

#include <stdlib.h>
#include <avr/wdt.h>
#include <EthernetClient.h>
#include <Ethernet.h>
#include <Dhcp.h>
#include <EthernetServer.h>
#include <util.h>
#include <Dns.h>
#include <EthernetUdp.h>
#include <EthernetICMP.h>


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

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0F, 0x87, 0x05
};
IPAddress ip(10, 1, 164, 97);

// Arduino digital output pins

int pins[] = {
  A0, A1, A2, A3, A4,A5, 2, 3, 4, 5, 6, 7, 8
};
int n_pins = sizeof(pins)/sizeof(int);

// telnet defaults to port 23

EthernetServer g_server(23);
EthernetClient g_client;
boolean connected = false;
String g_strcmd = "";
unsigned long g_nSW = 0x0;
unsigned long g_inv = 0x1FC0;
unsigned long last_active;
#define TELNET_TIMEOUT 10000

// Network Switch to monitor

SOCKET pingSocket = 3;
EthernetICMPPing ping(pingSocket, (uint16_t)random(0, 255));
IPAddress switch_ip(10, 1, 120, 30);
boolean switch_monitor = false;
unsigned long last_ping;
unsigned long last_ping_alive;
unsigned long last_reboot;
#define SWITCH_TIMEOUT 10000
#define REBOOT_WAIT 150000
#define PING_WAIT 3000
#define SWITCH_PIN 12

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

  // print number of pins
  DPRINT("Total pin number: ");
  DPRINTLN(n_pins);

  Ethernet.begin(mac, ip);
  g_server.begin();

  // print your local IP address:
  DPRINT("My IP address: ");
  DPRINTLN(Ethernet.localIP());

  g_nSW ^= g_inv;
  unsigned long mask = 1;
  for (int i = 0; i < n_pins; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
    mask <<= 1;
  }

  wdt_enable(WDTO_4S);
}

void power_on(int n) {
  unsigned long mask;

  mask = 1 << n;
  if(g_inv & mask) {
    digitalWrite(pins[n], LOW);
  } else {
    digitalWrite(pins[n], HIGH);
  }
}

void power_off(int n) {
  unsigned long mask;

  mask = 1 << n;
  if(g_inv & mask) {
    digitalWrite(pins[n], HIGH);
  } else {
    digitalWrite(pins[n], LOW);
  }
}

void parsing() {
  unsigned long devx;
  unsigned long valx;
  unsigned long mask;
  char str[256];

  if(g_strcmd == "Q") {
    // Query command
    sprintf(str, "%lX\n", g_nSW);
    g_client.write(str);

  } else if (g_strcmd.charAt(0) == 'S' && g_strcmd.length() == 9) {
    // Set command
    sprintf(str, "%c%c%c%c", g_strcmd.charAt(1), g_strcmd.charAt(2), g_strcmd.charAt(3), g_strcmd.charAt(4));
    devx = strtoul(str, NULL, 16);
    sprintf(str, "%c%c%c%c", g_strcmd.charAt(5), g_strcmd.charAt(6), g_strcmd.charAt(7), g_strcmd.charAt(8));
    valx = strtoul(str, NULL, 16);

    mask = 1;
    for (int i = 0; i < n_pins; i++) {
      if ((devx & mask) != 0) {
        if((valx & mask) != 0) {
          power_on(i);
          g_nSW |= mask;
        } else {
          power_off(i);
          g_nSW &= ~mask;
        }
      }
      mask <<= 1;
    }
//    delay(500);

  } else if (g_strcmd.charAt(0) == 'P' && g_strcmd.length() == 5) {
    // Pulse command
    sprintf(str, "%c%c%c%c", g_strcmd.charAt(1), g_strcmd.charAt(2), g_strcmd.charAt(3), g_strcmd.charAt(4));
    devx = strtoul(str, NULL, 16);
    // Pulse only power on devices
    devx &= g_nSW;
    if (devx == 0) return;

    // Power off for 1 second and then Power on
    mask = 1;
    for (int i = 0; i < n_pins; i++) {
      if ((devx & mask) != 0) {
        power_off(i);
      }
      mask <<= 1;
    }
    delay(1000);
    mask = 1;
    for (int i = 0; i < n_pins; i++) {
      if ((devx & mask) != 0) {
        power_on(i);
      }
      mask <<= 1;
    }

  } else if (g_strcmd.substring(0, 10) == "MONITORSET") {
    // set switch IP
    if(switch_ip.fromString(g_strcmd.substring(10))) {
      g_client.write("Set switch IP done\n");
    } else {
      g_client.write("Fail to set switch IP\n");
    }

  } else if (g_strcmd == "MONITORON") {
    // Enable switch monitor
    switch_monitor = true;
    last_ping = millis();
    last_ping_alive = last_ping;
    last_reboot = last_ping;
    g_client.write("Switch monitor On\n");

  } else if (g_strcmd == "MONITOROFF") {
    // Disable switch monitor
    switch_monitor = false;
    g_client.write("Switch monitor Off\n");

  } else if (g_strcmd == "RST") {
    // command to test reset function
    delay(5000);

  } else {
    sprintf(str, "unknown\n");
    g_client.write(str);
    DPRINTLN(g_strcmd);
  }
  return;
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
      } else if ((c >= 0x41 && c <= 0x5A) || (c >= 0x30 && c <= 0x39) || c == 0x2E) {
        g_strcmd += c;
      }
      last_active = millis();
    }
  }

  // Check switch status
  unsigned long now = millis();
  if (switch_monitor && ((now - last_ping) > PING_WAIT)) {
    EthernetICMPEchoReply echoReply = ping(switch_ip, 1);
    DPRINT(switch_ip);
    last_ping = now;
    if (echoReply.status == SUCCESS) {
      last_ping_alive = now;
      DPRINTLN(": Echo request succeed");
    } else {
      DPRINT(": Echo request failure: ");
      DPRINTLN(echoReply.status);
      if(((now - last_ping_alive) > SWITCH_TIMEOUT) && ((now - last_reboot) > REBOOT_WAIT)) {
        // Reset switch for 1s
        DPRINTLN("Reset switch");
        last_reboot = now;
        power_off(SWITCH_PIN);
        delay(1000);
        power_on(SWITCH_PIN);
      }
    }
  }

  wdt_reset();
}
