#ifndef _Climate_Monitoring_and_Control_Station_H
#define _Climate_Monitoring_and_Control_Station_H

#define OFF 0x0 // set the back-light
#define ON 0x1  //for monochrome LCD
#define LCD_TIME_OUT 10000   // seconds to lcd timeout
#define DHTPIN_1 3     //pin DHT22 Sensor 1 is on
#define DHTPIN_2 4     //pin DHT22 Sensor 2 is on
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define UPDATES_PER_HOUR 4
//Define field byte width (4 * 2 ints + 2 bytes in struct)
//Note operations could be replaced with more dynamic code using sizeof(), but probably slower.
#define FIELD_WIDTH 10

//Define fixed FRAM addresses manually calculated based off field width size
//need to create class to do this more dynamically
#define FRAM_ADDR_RESERV_0 0x0
#define FRAM_ADDR_RESERV_1 0x1
#define FRAM_ADDR_RESERV_2 0x2
#define FRAM_ADDR_RESERV_3 0x3
#define FRAM_ADDR_RESERV_4 0x4
#define FRAM_ADDR_RESERV_5 0x5
#define FRAM_ADDR_RESERV_6 0x6
#define FRAM_ADDR_RESERV_7 0x7
//Last (most recently) Written Adresses 16bits
#define FRAM_ADDR_LAST_QTR 0x8
#define FRAM_ADDR_LAST_HR 0xA
#define FRAM_ADDR_LAST_DAY 0xC
//First Adress of FRAM Segment
#define FRAM_ADDR_FIRST_QTR 0xE
#define FRAM_ADDR_FIRST_HR 0x1A
#define FRAM_ADDR_FIRST_DAY 0x26

// custom structures functions declarations and prototypes

struct sensorData {
  uint8_t humy1;
  int itmp1;
  uint8_t humy2;
  int itmp2;
  uint16_t pressurehPa;
  int itmp0;
};
/*
struct menuItemData {
  uint8_t lcdY;
  uint8_t lcdX;
  char menuItemName;  // I think this would have to be set manually to menuItemName[9] for 8 chars or 17 for the full LCD 16 chars +1 for NULL
};
*/
bool sensorioQuarterly( sensorData &sensorDataWr );
bool writeField( sensorData &sensorDataWr, uint16_t framWriteAddress );
bool readField( sensorData &sensorDataRd, uint16_t framReadAddress );
bool hrlyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd );
bool dailyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd );
//bool printtoLCD( uint16_t framReadAddress );
void lcdDrawHome( sensorData &sensorDataRd );
void mainMenu();
byte menuItemNav( byte numItems, byte currentItemNum );
bool lcdTimeOut();
void nowMenu();
void qtrMenu();
void hrsMenu();
void dayMenu();
void fansMenu();
void pumpsMenu();
void goBack();

#endif

