/**
* Agentuino SNMP Agent Library Prototyping...
*
* Copyright 2010 Eric C. Gionet <lavco_eg@hotmail.com>
*
* Update snmpGetNext by Petr Domorazek <petr@domorazek.cz>
*/

#define BOOTLOADER //Comment this line if you are not using bootloader
//#define DEBUG   //Uncomment this line for debug output
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif

// telemetry sensor board
// by Enos and ChihYi 2017/08/23

#include <Streaming.h>         // Include the Streaming library
#include <Ethernet.h>          // Include the Ethernet library
#include <SPI.h>
#include <MemoryFree.h>
#include <Agentuino.h>
#include <Flash.h>
#include <avr/wdt.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x87, 0x03 };
IPAddress ip(133, 40, 164, 98);

// telnet defaults to port 23

EthernetServer g_server(23);
EthernetClient g_client;
boolean connected = false;
String g_strcmd = "";
unsigned long last_active;
#define TELNET_TIMEOUT 15000

int flowPin = 2;
volatile uint32_t flowLastTrigger = 0;
volatile uint32_t flowPeriod = 0;
unsigned long duration;
int valveLockPin = 5;
int leakPin = 6;
int disPin = 7;

#include <Sensirion.h>

const uint8_t dataPin =  8;             // SHT serial data
const uint8_t clockPin =  9;            // SHT serial clock

Sensirion tempSensor = Sensirion(dataPin, clockPin);
const uint32_t minUpdateTime = 1000;     // SHT update time (10s)
float temperature = 0.0;
float humidity = 0.0;
float dewpoint = 0.0;

int valveSafeLock = 0;

String version = "1.0";

//
// tkmib - linux mib browser
//
// .iso (.1)
// .iso.org (.1.3)
// .iso.org.dod (.1.3.6)
// .iso.org.dod.internet (.1.3.6.1)
// .iso.org.dod.internet.private (.1.3.6.1.4)
// .iso.org.dod.internet.private.enterprises (.1.3.6.1.4.1)
//
// ASIAA defined OIDs
//
// .iso.org.dod.internet.private.enterprises.asiaa (.1.3.6.1.4.1.50399)
// .iso.org.dod.internet.private.enterprises.asiaa.sysDescr (.1.3.6.1.4.1.50399.1)
const static char sysDescr[] PROGMEM      = "1.3.6.1.4.1.50399.1.0";  // read-only  (DisplayString)
// .iso.org.dod.internet.private.enterprises.asiaa.sysObjectID (.1.3.6.1.4.1.50399.2)
const static char sysObjectID[] PROGMEM   = "1.3.6.1.4.1.50399.2.0";  // read-only  (ObjectIdentifier)
// .iso.org.dod.internet.private.enterprises.asiaa.sysUpTime (.1.3.6.1.4.1.50399.3)
const static char sysUpTime[] PROGMEM     = "1.3.6.1.4.1.50399.3.0";  // read-only  (TimeTicks)
// .iso.org.dod.internet.private.enterprises.asiaa.sysContact (.1.3.6.1.4.1.50399.4)
const static char sysContact[] PROGMEM    = "1.3.6.1.4.1.50399.4.0";  // read-write (DisplayString)
// .iso.org.dod.internet.private.enterprises.asiaa.sysName (.1.3.6.1.4.1.50399.5)
const static char sysName[] PROGMEM       = "1.3.6.1.4.1.50399.5.0";  // read-write (DisplayString)
// .iso.org.dod.internet.private.enterprises.asiaa.sysLocation (.1.3.6.1.4.1.50399.6)
const static char sysLocation[] PROGMEM   = "1.3.6.1.4.1.50399.6.0";  // read-write (DisplayString)
// .iso.org.dod.internet.private.enterprises.asiaa.sysTemp (.1.3.6.1.4.1.50399.7)
const static char sysTemp[] PROGMEM       = "1.3.6.1.4.1.50399.7.0";   // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysHumidity (.1.3.6.1.4.1.50399.8)
const static char sysHumidity[] PROGMEM   = "1.3.6.1.4.1.50399.8.0";   // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysDewPoint (.1.3.6.1.4.1.50399.9)
const static char sysDewPoint[] PROGMEM   = "1.3.6.1.4.1.50399.9.0";   // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysFlow (.1.3.6.1.4.1.50399.10)
const static char sysFlow[] PROGMEM       = "1.3.6.1.4.1.50399.10.0";  // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysLeakage (.1.3.6.1.4.1.50399.11)
const static char sysLeakage[] PROGMEM    = "1.3.6.1.4.1.50399.11.0";  // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysDisconnection (.1.3.6.1.4.1.50399.12)
const static char sysDisconnection[] PROGMEM    = "1.3.6.1.4.1.50399.12.0";  // read-only  (Integer)
// .iso.org.dod.internet.private.enterprises.asiaa.sysServices (.1.3.6.1.4.1.50399.13)
const static char sysServices[] PROGMEM    = "1.3.6.1.4.1.50399.13.0";  // read-only  (Integer)
//
// RFC1213 local values
static char locDescr[]              = "Subaru PFI telemmetry sensors";  // read-only (static)
static char locObjectID[]           = "1.3.6.1.4.1.50399";        // read-only (static)
static uint32_t locUpTime           = 0;                                // read-only (static)
static char locContact[20]          = "ChihYi Wen";                     // should be stored/read from EEPROM - read/write (not done for simplicity)
static char locName[20]             = "Telemetry sensors";              // should be stored/read from EEPROM - read/write (not done for simplicity)
static char locLocation[20]         = "Subaru";                         // should be stored/read from EEPROM - read/write (not done for simplicity)
static uint32_t lastUpdateTime      = 0;
static int32_t locServices          = 12;

uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

void pduReceived()
{
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);
  //
  if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET)
    && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS ) {
    //
    pdu.OID.toString(oid);
    // Implementation SNMP GET NEXT
    if ( pdu.type == SNMP_PDU_GET_NEXT ) {
      char tmpOIDfs[SNMP_MAX_OID_LEN];
      if ( strcmp_P(oid, sysDescr ) == 0 ) {
        strcpy_P ( oid, sysObjectID );
        strcpy_P ( tmpOIDfs, sysObjectID );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysObjectID ) == 0 ) {
        strcpy_P ( oid, sysUpTime );
        strcpy_P ( tmpOIDfs, sysUpTime );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysUpTime ) == 0 ) {
        strcpy_P ( oid, sysContact );
        strcpy_P ( tmpOIDfs, sysContact );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysContact ) == 0 ) {
        strcpy_P ( oid, sysName );
        strcpy_P ( tmpOIDfs, sysName );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysName ) == 0 ) {
        strcpy_P ( oid, sysLocation );
        strcpy_P ( tmpOIDfs, sysLocation );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysLocation ) == 0 ) {
        strcpy_P ( oid, sysTemp );
        strcpy_P ( tmpOIDfs, sysTemp );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysTemp ) == 0 ) {
        strcpy_P ( oid, sysHumidity );
        strcpy_P ( tmpOIDfs, sysHumidity );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysHumidity ) == 0 ) {
        strcpy_P ( oid, sysDewPoint );
        strcpy_P ( tmpOIDfs, sysDewPoint );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysDewPoint ) == 0 ) {
        strcpy_P ( oid, sysFlow );
        strcpy_P ( tmpOIDfs, sysFlow );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysFlow ) == 0 ) {
        strcpy_P ( oid, sysLeakage );
        strcpy_P ( tmpOIDfs, sysLeakage );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysLeakage ) == 0 ) {
        strcpy_P ( oid, sysDisconnection );
        strcpy_P ( tmpOIDfs, sysDisconnection );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysDisconnection ) == 0 ) {
        strcpy_P ( oid, sysServices );
        strcpy_P ( tmpOIDfs, sysServices );
        pdu.OID.fromString(tmpOIDfs);
      } else if ( strcmp_P(oid, sysServices ) == 0 ) {
        strcpy_P ( oid, "1.0" );
      } else {
        int ilen = strlen(oid);
        if ( strncmp_P(oid, sysDescr, ilen ) == 0 ) {
          strcpy_P ( oid, sysDescr );
          strcpy_P ( tmpOIDfs, sysDescr );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysObjectID, ilen ) == 0 ) {
          strcpy_P ( oid, sysObjectID );
          strcpy_P ( tmpOIDfs, sysObjectID );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysUpTime, ilen ) == 0 ) {
          strcpy_P ( oid, sysUpTime );
          strcpy_P ( tmpOIDfs, sysUpTime );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysContact, ilen ) == 0 ) {
          strcpy_P ( oid, sysContact );
          strcpy_P ( tmpOIDfs, sysContact );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysName, ilen ) == 0 ) {
          strcpy_P ( oid, sysName );
          strcpy_P ( tmpOIDfs, sysName );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysLocation, ilen ) == 0 ) {
          strcpy_P ( oid, sysLocation );
          strcpy_P ( tmpOIDfs, sysLocation );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysTemp, ilen ) == 0 ) {
          strcpy_P ( oid, sysTemp );
          strcpy_P ( tmpOIDfs, sysTemp );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysHumidity, ilen ) == 0 ) {
          strcpy_P ( oid, sysHumidity );
          strcpy_P ( tmpOIDfs, sysHumidity );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysDewPoint, ilen ) == 0 ) {
          strcpy_P ( oid, sysDewPoint );
          strcpy_P ( tmpOIDfs, sysDewPoint );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysFlow, ilen ) == 0 ) {
          strcpy_P ( oid, sysFlow );
          strcpy_P ( tmpOIDfs, sysFlow );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysLeakage, ilen ) == 0 ) {
          strcpy_P ( oid, sysLeakage );
          strcpy_P ( tmpOIDfs, sysLeakage );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysDisconnection, ilen ) == 0 ) {
          strcpy_P ( oid, sysDisconnection );
          strcpy_P ( tmpOIDfs, sysDisconnection );
          pdu.OID.fromString(tmpOIDfs);
        } else if ( strncmp_P(oid, sysServices, ilen ) == 0 ) {
          strcpy_P ( oid, sysServices );
          strcpy_P ( tmpOIDfs, sysServices );
          pdu.OID.fromString(tmpOIDfs);
        }
      }
    }
    // End of implementation SNMP GET NEXT / WALK

    if ( strcmp_P(oid, sysDescr ) == 0 ) {
      // handle sysDescr (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locDescr
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locDescr);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysObjectID ) == 0 ) {
      // handle sysDescr (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locDescr
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locObjectID);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysUpTime ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locUpTime
        status = pdu.VALUE.encode(SNMP_SYNTAX_TIME_TICKS, locUpTime);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysName ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locName, strlen(locName));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locName
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysContact ) == 0 ) {
      // handle sysContact (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locContact, strlen(locContact));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locContact
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locContact);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysLocation ) == 0 ) {
      // handle sysLocation (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locLocation, strlen(locLocation));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locLocation
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysTemp ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        if(locUpTime - lastUpdateTime > minUpdateTime) {
          // No need to update within 5s
          tempSensor.measure(&temperature, &humidity, &dewpoint);
          lastUpdateTime = locUpTime;
        }
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, (int) (temperature * 100) );
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysHumidity ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        if(locUpTime - lastUpdateTime > minUpdateTime) {
          // No need to update within 5s
          tempSensor.measure(&temperature, &humidity, &dewpoint);
          lastUpdateTime = locUpTime;
        }
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, (int) (humidity * 100) );
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysDewPoint ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        if(locUpTime - lastUpdateTime > minUpdateTime) {
          // No need to update within 5s
          tempSensor.measure(&temperature, &humidity, &dewpoint);
          lastUpdateTime = locUpTime;
        }
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, (int) (dewpoint * 100) );
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysFlow ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, (int) (100 * getFlow()));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysLeakage ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, 1-digitalRead(leakPin) );
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysDisconnection ) == 0 ) {
      // handle sysFlow (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, digitalRead(disPin) );
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysServices) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locServices
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }    
    } else {
      // oid does not exist
      // response packet - object not found
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = SNMP_ERR_NO_SUCH_NAME;
    }
    //
    Agentuino.responsePdu(&pdu);
  }
  //
  Agentuino.freePdu(&pdu);
  //
}

void doSHT75()
{
  char str[30];
  char str_temp[10];

  if(locUpTime - lastUpdateTime > minUpdateTime) {
    // No need to update within 5s
    tempSensor.measure(&temperature, &humidity, &dewpoint);
    lastUpdateTime = locUpTime;
  }

  dtostrf(temperature, 4, 2, str_temp);
  sprintf(str, "Temperature = %s C, ", str_temp );
  g_client.write(str);

  dtostrf(humidity, 4, 2, str_temp);
  sprintf(str, "Humidity = %s %%, ", str_temp);
  g_client.write(str);

  dtostrf(dewpoint, 4, 2, str_temp);
  sprintf(str, "Dewpoint = %s C\n", str_temp);
  g_client.write(str);
}

double getFlow()
{
  uint32_t diff = millis() - flowLastTrigger;

  if (flowPeriod == 0 || diff >= 10000) {
    // If there is no trigger for more than 10s, then the frequency is below 0.1Hz
    return 0.0;
  } else {
    return 1000.0 / flowPeriod;
  }
}

void doFlow()
{
  char str[30], hzstr[16];
  double hz;

  hz = getFlow();
  dtostrf(hz, 6, 1, hzstr);
  sprintf(str, "Flow = %s Hz\n", hzstr);
  g_client.write(str);
}


void leakControl(){
  char str[30];
  int val1, val2;
  
  val1 = digitalRead(leakPin);
  val2 = digitalRead(disPin);

  valveSafeLock = val1||val2;
  if (valveSafeLock == 1){
     digitalWrite(valveLockPin, HIGH);
  } 

}

void doVersion()
{
  //char vstr = version;
  char charBuf[50];
  char str[256];
  version.toCharArray(charBuf, 50);
  
  sprintf(str, "Current firmware version %s \n", charBuf);

  g_client.write(str);

}


void doLeak()
{
  char str[256];
  int val1, val2;

  
  val1 = digitalRead(leakPin);
  sprintf( str, "Liquid leakage %d, ", val1);
  g_client.write(str);

  val2 = digitalRead(disPin);
  // If disPin is 0, it means it is connected.
  sprintf( str, "leakge sensor status %d\n", val2);

  g_client.write(str);

  valveSafeLock = digitalRead(valveLockPin);
  sprintf( str, "valveSafeLock status = %d\n", valveSafeLock);
 
  g_client.write(str);
  
}


void valveLockControl(int lockStatus)
{
   char str[256];
  if (lockStatus == 1){
     digitalWrite(valveLockPin, HIGH);
     sprintf( str, "valveSafeLock status = %d\n", lockStatus);
  } else if (lockStatus == 0){
     digitalWrite(valveLockPin, LOW);
     sprintf( str, "valveSafeLock status = %d\n", lockStatus);
  } else{
     sprintf( str, "valveSafeLock status = %d unknow/unacceptable.\n", lockStatus);
  }

   g_client.write(str);
}

void parsing()
{
  
  int lockStatus;
  char str[30];
  
  if(g_strcmd == "Q") {
    doSHT75();
    doFlow();
    doLeak();
  } else if (g_strcmd == "V"){
    doVersion();
  } else if (g_strcmd.charAt(0) == 'C'){
    lockStatus = g_strcmd.substring(1).toInt();
    valveLockControl(lockStatus);
  } else if (g_strcmd == "RST") {
    // command to test reset function
    delay(15000);
  } else {
    sprintf(str, "unknown\n");
    g_client.write("unknown\n");
  }
  return;
}

void setup()
{
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

  // start the Ethernet connection
  Ethernet.begin(mac, ip);
  // print your local IP address
  DPRINT("My IP address: ");
  DPRINTLN(Ethernet.localIP());
  //
  g_server.begin();
  //
  pinMode(flowPin, INPUT);
  pinMode(leakPin, INPUT);
  pinMode(disPin, INPUT);
  pinMode(valveLockPin, OUTPUT);

  // Initial the default value to LOW, close the valve.
  //digitalWrite(valveLockPin, LOW);
  digitalWrite(valveLockPin, HIGH);
  
  //
  attachInterrupt(digitalPinToInterrupt(flowPin), trigger, FALLING);
  //
  api_status = Agentuino.begin();
  //
  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
  } else {
    DPRINTLN("Failed to start SNMP server");
  }
  wdt_enable(WDTO_8S);
}

void trigger()
{
  uint32_t now;

  now = millis();
  flowPeriod = now - flowLastTrigger;
  flowLastTrigger = now;
}

void loop()
{
  // listen/handle for incoming SNMP requests
  Agentuino.listen();
  //
  // sysUpTime - The time (in hundredths of a second) since
  // the network management portion of the system was last
  // re-initialized.
  if ( millis() - prevMillis > 1000 ) {
    // increment previous milliseconds
    prevMillis += 1000;
    //
    // increment up-time counter
    locUpTime += 100;
  }
  //
  char c = 0;
  char t = ':';
  char r = 0x0d;
  EthernetClient g_client_new;

  leakControl();
  
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

  //
  wdt_reset();
}
