#include "SHT25.h"
#include "Wire.h"

SHT25 sht25;

void setup(){
  Wire.begin();
  Serial.begin(115200);
  
  Serial.println("SHT25 Demo");

  if(!sht25.begin()){
    Serial.println("SHT25 not found! Halting Program.");
    for(;;);
  }
  
  uint8_t serial_number[8] = {0};
  sht25.getSerialNumber(serial_number);
  Serial.print("Serial Number: ");
  for(uint8_t ii = 0; ii < 8; ii++){
    if(serial_number[ii] < 0x10){
      Serial.print("0");
      Serial.print(serial_number[ii], HEX);
    }
  }
  Serial.println();
  
  Serial.print("Setting Measurement Resolution to 14-Bit...");
  sht25.setMeasurementResolution(SHT25_RESOLUTION_14BIT);
  Serial.println("done.");
  
  Serial.print("User Register = 0x");
  Serial.println(sht25.getUserData(), HEX);  
}

void loop(){
  Serial.print("Temperature = ");
  Serial.println(sht25.getTemperature(), 2);
  Serial.print("Humidity = ");
  Serial.println(sht25.getRelativeHumidity(), 2);
  Serial.println("-----");
  delay(2000);
}