#include "SHT25.h"
#include <Wire.h>
#include <stdint.h>

SHT25::SHT25(){
  
}

boolean SHT25::begin(uint8_t temperature_resolution_code){
	softReset();
	delay(20);
	return setMeasurementResolution(temperature_resolution_code);
}

boolean SHT25::requestReadAndReceiveBytes(uint8_t * buf, uint8_t num_bytes, boolean emit_stop){
  
  Wire.requestFrom((int) SHT25_7_BIT_I2C_ADDRESS, (int) num_bytes, emit_stop ? 1 : 0);
  const long interval = 100; // maximum duration should be 85ms according to datasheet
  unsigned long previousMillis = millis();  
  while(num_bytes > 0){  
    if(Wire.available()){
      *buf++ = Wire.read();
      num_bytes--;
    }    
    else if(millis() - previousMillis >= interval){ 
      // this pattern is adapted from the blinkWithoutDelay example, which correctly handles roll-over
      // if this condition is satisfied, it implies the interval has expired
      // without receiving the number of bytes expected, so a timeout has happened
      return false;
    }    
  }  
  
  return true;
}

// this implements the Hold Master mode only
// buf must have space for three byte values
boolean SHT25::getTempHumidityRequestCommon(uint8_t cmd, uint8_t * buf){
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); // address the SHT25
  Wire.write(cmd);
  Wire.endTransmission(false); // send + rep start
  
  // receive 3 bytes then stop
  if(!requestReadAndReceiveBytes(buf, 3)){
    return false;
  }
  
  //Serial.print("Bytes: ");
  //Serial.print(buf[0], HEX);
  //Serial.print(", ");
  //Serial.print(buf[1], HEX);
  //Serial.print(", ");
  //Serial.print(buf[2], HEX);
  //Serial.println();
  
  // do the crc calculation and see if it matches
  if(!checkCRC(buf, 2, buf[2])){
  	return false;
  }

 	return true;
}

boolean SHT25::getTemperature(float * result){
  uint8_t buf[3] = {0};
  float temperature = 0.0f;
  // 0xE3 is the command for Trigger T Measurement, Hold Master
  if(getTempHumidityRequestCommon(0xE3, buf)){
 		// interpret the temperature bytes
 		uint16_t data = buf[0];
 		data <<= 8;
 		data |= buf[1]; 	
 		
 		temperature = 175.72 * data;
 		temperature /= 65536.0;
 		temperature -= 46.85;
 		
 		*result = temperature; 		
 		return true;
  }
  
  return false;
}

boolean SHT25::getRelativeHumidity(float * result){
  uint8_t buf[3] = {0};
  float humidity = 0.0f;
  // 0xE3 is the command for Trigger RH Measurement, Hold Master
  if(getTempHumidityRequestCommon(0xE5, buf)){
 		// interpret the humidity bytes
 		uint16_t data = buf[0];
 		data <<= 8;
 		data |= buf[1]; 	
 		
 		humidity = 125.0 * data;
 		humidity /= 65536.0;
 		humidity -= 6.0;
 		
 		*result = humidity;
 		return true;
  }
  
  return false;
}

void SHT25::softReset(void){
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); // address the SHT25
  Wire.write(0xFE); // 0xFE is the soft reset command
  Wire.endTransmission(true); // send + stop
}

// This is based on App Note titled:
// Electronic Identification Code
// How to read-out the serial number of SHT2x
// http://www.sensirion.co.jp/fileadmin/user_upload/customers/sensirion/Dokumente/Humidity/Sensirion_Humidity_SHT2x_Electronic_Identification_Code_V1.1.pdf
// buf must have space for at least 8 bytes for the identification code
void  SHT25::getSerialNumber(uint8_t * buf){
  uint8_t local_buf[8] = { 0 };
  
  // First memory access 
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); // address the SHT25
  Wire.write(0xFA);
  Wire.write(0x0F);
  Wire.endTransmission(false); // send + rep start

  // receive 8 bytes then stop
  requestReadAndReceiveBytes(local_buf, 8);
  
  // the odd numbered bytes are all 'crc' bytes
  uint8_t snb_3 = local_buf[0];
  uint8_t snb_2 = local_buf[2];
  uint8_t snb_1 = local_buf[4];
  uint8_t snb_0 = local_buf[6]; 
  
  // Second memory access
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); // address the SHT25
  Wire.write(0xFC);
  Wire.write(0xC9);
  Wire.endTransmission(false); // send + rep start
  
  // receive 6 bytes then stop
  requestReadAndReceiveBytes(local_buf, 6);
  
  uint8_t snc_1 = local_buf[0];
  uint8_t snc_0 = local_buf[1];
  uint8_t sna_1 = local_buf[3];
  uint8_t sna_0 = local_buf[4];    
  
  buf[0] = sna_1;
  buf[1] = sna_0;
  buf[2] = snb_3;
  buf[3] = snb_2;
  buf[4] = snb_1;
  buf[5] = snb_0;
  buf[6] = snc_1;
  buf[7] = snc_0;
}

uint8_t SHT25::getUserData(boolean emit_stop){
  uint8_t user_data = 0;
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); // address the SHT25
  Wire.write(0xE7);                 // 0xE7 is the code for 'Read user register'
  Wire.endTransmission(false);      // xmit & send repeated start
  
  // read 1 byte, then stop or repeat start
  requestReadAndReceiveBytes(&user_data, 1, emit_stop);
  return user_data;
}

boolean SHT25::setMeasurementResolution(uint8_t temperature_resolution_code){
  boolean ret = true;
  uint8_t user_data = getUserData(false);
  if(user_data == 0){
    // this is only ever the case if the sensor is absent
    ret = false;
  }
  
  if(temperature_resolution_code < 4){
    // resolution code goes in the top two bits of the user register
    user_data &= 0x3F; 
    user_data |= (temperature_resolution_code << 6);    
  }
  
  Wire.beginTransmission(SHT25_7_BIT_I2C_ADDRESS); 
  Wire.write(0xE6);      // 0xE6 is the code for 'Write user register'
  Wire.write(user_data);              
  Wire.endTransmission();     
  
  return ret; 
}


boolean SHT25::checkCRC(uint8_t * data, uint8_t nbrOfBytes, uint8_t checksum){
    int crc = 0;
    int byteCtr = 0;
    const int POLYNOMIAL = 0x131;  //P(x)=x^8+x^5+x^4+1 = 100110001
    //calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr) {
        crc ^= (data[byteCtr]);
        for (int bit = 8; bit > 0; --bit) {
            if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else crc = (crc << 1);
        }
    }

    crc &= 0xff;    
    
    //Serial.print("Expected: ");
    //Serial.print(checksum, HEX);
    //Serial.print(", Calculated: ");
    //Serial.print(crc, HEX);
    //Serial.println();
    
    if (crc != checksum) return false;
    else return true;
}