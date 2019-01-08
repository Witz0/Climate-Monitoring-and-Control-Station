/*********************
 * Arduino Uno or Trinket Pro 5V w/
 * MonoLCD Shield
 * MPL115A2 Temp and Pressure Sensor
 * 2 x DHT22 Temp and Humidity Sensors
 * 32K FRAM for long term data storage
 * for Climate Monitoring and Control
 **********************/
#include <Wire.h>
#include <Adafruit_MPL115A2.h>       //Barometric and Temp Sensor
//#include <Adafruit_MCP23017.h>       //LCD Shield Parallel to Serial
#include <Adafruit_RGBLCDShield.h>   //AdaFruit Library for LCD Shield w/ Buttons
#include <DHT.h>                     //DHT22 Humidity and Temp Sensors
#include <Adafruit_FRAM_I2C_Plus.h>       //FRAM chip I/O plus lib
#include <Time.h>                    //Time library synced via serial
#include "Timer.h"
#include "Climate_Monitoring_and_Control_Station.h"

//globals?
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// The LCD shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.

Adafruit_MPL115A2 mpl115a2;     //Barometric and Temp Sensor
//DHT dht(DHTPIN, DHTTYPE);     // Initialize DHT sensor for normal 16mhz Arduino

// Depending on ardiono chip speed the threshold of cycle counts needs to be set by passing a 3rd parameter.
// default for a 16mhz AVR is a value of 6.
// Arduino Due @ 84mhz set to 30.
// See Adafruit DHTtester Example for more info.
DHT dht1(DHTPIN_1, DHTTYPE, 6);     // for Pro Trinket 3V @ 12Mhz set = to 5
DHT dht2(DHTPIN_2, DHTTYPE, 6);

Adafruit_FRAM_I2C_Plus fram     = Adafruit_FRAM_I2C_Plus();
uint16_t          framAddr = 0;
Timer ioTimer; //instance of Timer for sensor gets
Timer lcdTimer; //instance of Timer for LCD Backlight Time Out

//bool printtoLCD( uint16_t framReadAddress );
/* +*-+-----+------+---------+--------+---------+----------+---------+---------+
 Begin Setup of arduino and
 all sensors and libraries
 +-------+-------+--------+---------+--------+----------+--------+-----------*+ */
void setup() {
  Serial.begin(9600);		// Debugging output
  lcd.begin(16, 2);			// set up the LCD's number of columns and rows:
  lcd.setBacklight(ON);

  mpl115a2.begin();
  dht1.begin();
  dht2.begin();

  //FRAM consists of 32,768 words × 8 bits = 262,144 bits
  if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
  }
  else {
    Serial.println("No I2C FRAM found ... check your connections\r\n");
    while (1);
  }

  //Read the first byte
  uint16_t test = fram.read16(0x0);
  Serial.print("Restarted ");
  //Serial.print(test);
  //Serial.println(" times");
  //Test write ++
  //fram.write16(0x0, test+1);

  Serial.println("Setup Complete. First I/O incoming....");
  //delay( 250 )
  //lcd.setBacklight(OFF);
}

/* +*-+-----+------+---------+--------+---------+----------+---------+---------+
 Begin Looping continuous code for
 arduino and all sensors and libraries
 +-------+-------+--------+---------+--------+----------+--------+-----------*+ */

void loop() {
  /* General Functionality is to grab sensor data ~ every 15 mins, store 1hr of 15 min interval data in the first array in FRAM,
   24 hrs of hourly averaged data in second array in FRAM and daily averages to Third long term array in FRAM.
   With conditions placed on user input buttons for display print.
   FRAM storage will need some tracking for eventual overwrite on rollover (pointers) and for readout
   to UI menus
   */
  uint8_t buttons = lcd.readButtons();
  sensorData sensorDataWr;
  sensorData sensorDataRd;
  sensorData sensorDataAvg;
  uint16_t framWriteAddress = FRAM_ADDR_FIRST_QTR;
  //static unsigned long previousMillis = 990000;  //initial value over 15 minutes forces first IOEvent at startup
  //static unsigned long lcdpreviousMillis = 0;
  static uint8_t dailyIOEvents = 24 * UPDATES_PER_HOUR;
  unsigned long millisperhour = 3600000;
  unsigned long ioInterval;    //15 minute I/O update intervals
  //note: framReadAddress var for lcd readout more OOP/class should be redesigned to eliminate redundancies
  uint16_t framReadAddress;

  ioInterval = millisperhour / UPDATES_PER_HOUR;

  // Note: be nice to get first I/O on boot, which requires do-while with state machine check with static var.
  if ( ioTimer.CheckTimer( ioInterval )) {
    if ( sensorioQuarterly( sensorDataWr ) == true ) {
      writeField( sensorDataWr, framWriteAddress + (FIELD_WIDTH * ((24 * UPDATES_PER_HOUR) - dailyIOEvents)));
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      dailyIOEvents--;
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      Serial.print("Time = ");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(".");
      Serial.println(second());
      Serial.print("framWriteAddress: ");
      Serial.println(framWriteAddress, HEX );
      if ( ( dailyIOEvents ) % UPDATES_PER_HOUR == 0 ) {
        hrlyavgs( sensorDataWr, sensorDataAvg, sensorDataRd );
        if ( dailyIOEvents == 0 ) {
          dailyIOEvents = 24 * UPDATES_PER_HOUR;
          dailyavgs( sensorDataWr, sensorDataAvg, sensorDataRd );
          if ( framWriteAddress == 32768 ) {
            fram.write16( FRAM_ADDR_LAST_DAY, FRAM_ADDR_FIRST_DAY );
          }
        }
      }
    }

    framReadAddress = FRAM_ADDR_LAST_QTR;
    Serial.print("Minutes: ");
    Serial.println(minute());
    readField( sensorDataRd, framReadAddress );
  }

  if ( buttons ) {
    Serial.print("framReadAddress in HEX: ");
    Serial.println(framReadAddress, HEX);
    Serial.print("framReadAddress data in HEX: ");
    Serial.println(fram.read16( framReadAddress ), HEX);
    readField( sensorDataRd, framReadAddress );

    lcdDrawHome( sensorDataRd );
    if ( buttons & BUTTON_SELECT ){
      mainMenu();

    }
  }
  lcdTimeOut();
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 END OF MAIN LOOP FUNCTION
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

// Sensor I/O Function gets inputs and writes initial data to FRAM every 15 mins

bool sensorioQuarterly( sensorData &sensorDataWr ) {
  float tempCon;
  float pressureCon;
  //float pressurehPa;

  //DHT22 is a slow sensor so reading takes about 250 millis and data may be 2 seconds old.

  //DHT22 Read Sensor 1
  sensorDataWr.humy1 = byte( dht1.readHumidity( ));
  // Read temperature as Fahrenheit
  sensorDataWr.itmp1 = int( dht1.readTemperature(true));

  // Check if any reads failed and exit early (to try again).
  if (isnan(sensorDataWr.humy1) || isnan(sensorDataWr.itmp1)) {
    Serial.println("Failed to read DHT sensor! 01");
    return false;
  }
  Serial.print("Humy01: ");
  Serial.println(sensorDataWr.humy1);
  Serial.print("FTemp01: ");
  Serial.println(sensorDataWr.itmp1);

  //DHT22 Read Sensor 2

  sensorDataWr.humy2 = byte( dht2.readHumidity( ));
  // Read temperature as Fahrenheit
  sensorDataWr.itmp2 = int( dht2.readTemperature(true));

  // Check if any reads failed and exit early (to try again).
  if (isnan(sensorDataWr.humy2) || isnan(sensorDataWr.itmp2)) {
    Serial.println("Failed to read from DHT sensor! 02");
    return false;
  }

  Serial.print("Humy02: ");
  Serial.println(sensorDataWr.humy2);
  Serial.print("FTemp02: ");
  Serial.println(sensorDataWr.itmp2);

  // MP115A2 Sensor Read

  /*Note: conversions to english units before stores
   C to F (9/5)=1.8 +32 and kPa to hPa
   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   */

  sensorDataWr.pressurehPa = 0;
  sensorDataWr.itmp0 = 0;
  pressureCon = mpl115a2.getPressure( );
  //pressureCon = pressureCon * 10;
  Serial.print("pressureCon Float: ");
  Serial.println( pressureCon );
  sensorDataWr.pressurehPa = int(pressureCon);

  Serial.print("Pressure int(): ");
  Serial.println(sensorDataWr.pressurehPa);

  tempCon = mpl115a2.getTemperature( );
  Serial.print("tempConPre: ");
  Serial.println( tempCon, 4);
  tempCon = (( tempCon * 1.8 ) + 32);
  Serial.print("tempConPost: ");
  Serial.println( tempCon, 4);
  sensorDataWr.itmp0 = tempCon;

  Serial.print("Temp0): ");
  Serial.print(sensorDataWr.itmp0);
  Serial.println("F");

  return true;

}
/*not modular enough rewrite with func or two
 so individual addresses & values are handled by in single func call
 framWriteAddress = write/readData( framAddress, sensorDataCopy )
 */
bool writeField( sensorData &sensorDataWr, uint16_t framWriteAddress ) {

  fram.write8( framWriteAddress, sensorDataWr.humy1 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.humy1 ));
  fram.write16( framWriteAddress, sensorDataWr.itmp1 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp1 ));
  fram.write8( framWriteAddress, sensorDataWr.humy2 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.humy2 ));
  fram.write16( framWriteAddress, sensorDataWr.itmp2 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp2 ));
  fram.write16( framWriteAddress, sensorDataWr.pressurehPa );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.pressurehPa ));
  fram.write16( framWriteAddress, sensorDataWr.itmp0 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp0 ));
  Serial.println("writeField() called: ");

  return true;
}

bool readField( sensorData &sensorDataRd, uint16_t framReadAddress ) {

  sensorDataRd.humy1 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.humy1 ));
  sensorDataRd.itmp1 = fram.read16( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp1 ));
  sensorDataRd.humy2 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.humy2 ));
  sensorDataRd.itmp2 = fram.read16( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp2 ));
  sensorDataRd.pressurehPa = fram.read16( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.pressurehPa ));
  sensorDataRd.itmp0 = fram.read16( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp0 ));
  Serial.println("readField() called: ");

  return true;
}


bool hrlyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
  uint16_t framReadAddress = FRAM_ADDR_FIRST_QTR;
  uint16_t framWriteAddress;
  sensorDataAvg.humy1 = 0;
  sensorDataAvg.itmp1 = 0;
  sensorDataAvg.humy2 = 0;
  sensorDataAvg.itmp2 = 0;
  sensorDataAvg.pressurehPa = 0;
  sensorDataAvg.itmp0 = 0;

  if( fram.read16( FRAM_ADDR_FIRST_HR ) == 0 ) {
    framWriteAddress = FRAM_ADDR_FIRST_HR;
  }
  else {
    framWriteAddress = fram.read16( FRAM_ADDR_LAST_HR );
  }

  for ( byte updates = UPDATES_PER_HOUR; updates > 0; updates-- ) {
    readField( sensorDataRd, framReadAddress );
    framReadAddress = framReadAddress + FIELD_WIDTH;
    framWriteAddress = framWriteAddress + FIELD_WIDTH;
    sensorDataAvg.humy1 = sensorDataAvg.humy1 + sensorDataRd.humy1;
    sensorDataAvg.itmp1 = sensorDataAvg.itmp1 + sensorDataRd.itmp1;
    sensorDataAvg.humy2 = sensorDataAvg.humy2 + sensorDataRd.humy2;
    sensorDataAvg.itmp2 = sensorDataAvg.itmp2 + sensorDataRd.humy2;
    sensorDataAvg.pressurehPa = sensorDataAvg.pressurehPa + sensorDataRd.humy2;
    sensorDataAvg.itmp0 = sensorDataAvg.itmp0 + sensorDataRd.humy2;
  }

  sensorDataAvg.humy1 = sensorDataAvg.humy1 / UPDATES_PER_HOUR;
  sensorDataAvg.itmp1 = sensorDataAvg.itmp1 / UPDATES_PER_HOUR;
  sensorDataAvg.humy2 = sensorDataAvg.humy2 / UPDATES_PER_HOUR;
  sensorDataAvg.itmp2 = sensorDataAvg.itmp2 / UPDATES_PER_HOUR;
  sensorDataAvg.pressurehPa = sensorDataAvg.pressurehPa / UPDATES_PER_HOUR;
  sensorDataAvg.itmp0 = sensorDataAvg.itmp0 / UPDATES_PER_HOUR;
  sensorDataWr.humy1 = sensorDataAvg.humy1;
  sensorDataWr.itmp1 = sensorDataAvg.itmp1;
  sensorDataWr.humy2 = sensorDataAvg.humy2;
  sensorDataWr.itmp2 = sensorDataAvg.itmp2;
  sensorDataWr.pressurehPa = sensorDataAvg.pressurehPa;
  sensorDataWr.itmp0 = sensorDataAvg.itmp0;
  writeField( sensorDataWr, framWriteAddress );
  framWriteAddress = framWriteAddress + FIELD_WIDTH;
  fram.write16( framWriteAddress, FRAM_ADDR_LAST_HR );
  Serial.println("hrlyavgs() called: ");

  return true;
}

bool dailyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
  uint16_t framReadAddress = FRAM_ADDR_FIRST_HR;
  uint16_t framWriteAddress;
  sensorDataAvg.humy1 = 0;
  sensorDataAvg.itmp1 = 0;
  sensorDataAvg.humy2 = 0;
  sensorDataAvg.itmp2 = 0;
  sensorDataAvg.pressurehPa = 0;
  sensorDataAvg.itmp0 = 0;

  if( fram.read16( FRAM_ADDR_FIRST_DAY ) == 0 ) {
    framWriteAddress = FRAM_ADDR_FIRST_DAY;
  }
  else {
    framWriteAddress = fram.read16( FRAM_ADDR_LAST_DAY);
  }

  for ( byte hours = 24; hours > 0; hours-- ) {
    readField( sensorDataRd, framReadAddress );
    framReadAddress = framReadAddress + FIELD_WIDTH;
    sensorDataAvg.humy1 = sensorDataAvg.humy1 + sensorDataRd.humy1;
    sensorDataAvg.itmp1 = sensorDataAvg.itmp1 + sensorDataRd.itmp1;
    sensorDataAvg.humy2 = sensorDataAvg.humy2 + sensorDataRd.humy2;
    sensorDataAvg.itmp2 = sensorDataAvg.itmp2 + sensorDataRd.itmp2;
    sensorDataAvg.pressurehPa = sensorDataAvg.pressurehPa + sensorDataRd.pressurehPa;
    sensorDataAvg.itmp0 = sensorDataAvg.itmp0 + sensorDataRd.itmp0;
  }

  sensorDataAvg.humy1 = sensorDataAvg.humy1 / 24;
  sensorDataAvg.itmp1 = sensorDataAvg.itmp1 / 24;
  sensorDataAvg.humy2 = sensorDataAvg.humy2 / 24;
  sensorDataAvg.itmp2 = sensorDataAvg.itmp2 / 24;
  sensorDataAvg.pressurehPa = sensorDataAvg.pressurehPa / 24;
  sensorDataAvg.itmp0 = sensorDataAvg.itmp0 / 24;
  sensorDataWr.humy1 = sensorDataAvg.humy1;
  sensorDataWr.itmp1 = sensorDataAvg.itmp1;
  sensorDataWr.humy2 = sensorDataAvg.humy2;
  sensorDataWr.itmp2 = sensorDataAvg.itmp2;
  sensorDataWr.pressurehPa = sensorDataAvg.pressurehPa;
  sensorDataWr.itmp0 = sensorDataAvg.itmp0;
  writeField( sensorDataWr, framWriteAddress );
  framWriteAddress = framWriteAddress + FIELD_WIDTH;
  fram.write16( framWriteAddress, FRAM_ADDR_LAST_DAY );
  Serial.println("dailyavgs() called: ");

  return true;
}

void lcdDrawHome( sensorData &sensorDataRd ) {
  lcd.setBacklight(OFF);
  lcd.setBacklight(ON);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(sensorDataRd.humy1);
  lcd.print("%");
  lcd.setCursor(4,0);
  lcd.print(sensorDataRd.itmp1);
  lcd.print("F");
  lcd.setCursor(8,0);
  lcd.print(sensorDataRd.humy2);
  lcd.print("%");
  lcd.setCursor(12,0);
  lcd.print(sensorDataRd.itmp2);
  lcd.print("F");
  lcd.setCursor(0,1);
  lcd.print(sensorDataRd.pressurehPa);
  lcd.print("P");
  lcd.setCursor(4,1);
  lcd.print(sensorDataRd.itmp0);
  lcd.print("F");
  lcd.setCursor(0,1);
  lcd.print("E");
  //lcd.print(dailyIOEvents);
  lcd.setCursor(12,1);
  lcd.print("MENU");
  lcd.setCursor(12,1);
  lcd.blink();
  Serial.println("lcdDrawHome() called: ");
  Serial.print("sensorDataRd.humy2: ");
  Serial.println(sensorDataRd.humy2);
}

//need to do better design maybe menu class with enumerated items maybe try printing enumsfor menu names and use struct for lcd x,y?
void mainMenu() {
  uint8_t buttons = lcd.readButtons();
  //if (first run) fix this so menu data is set once to reduce cpu
  byte numItems = 7;
  byte currentItemNum = 0;

  lcd.setBacklight(OFF);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("NOW");
  lcd.setCursor(4,0);
  lcd.print("QTR");
  lcd.setCursor(8,0);
  lcd.print("HRS");
  lcd.setCursor(12,0);
  lcd.print("DAY");
  lcd.setCursor(0,1);
  lcd.print("FANS");
  lcd.setCursor(5,1);
  lcd.print("PUMPS");
  lcd.setCursor(15,1);
  lcd.print(">");
  lcd.setCursor(0,0);
  //lcd.setCursor();
  lcd.blink();
  lcd.setBacklight(ON);

  currentItemNum = menuItemNav( numItems, currentItemNum );

  if (buttons & BUTTON_SELECT ) {
    switch (currentItemNum) {
    case 1:
      nowMenu();
      break;
    case 2:
      qtrMenu();
      break;
    case 3:
      hrsMenu();
      break;
    case 4:
      dayMenu();
      break;
    case 5:
      fansMenu();
      break;
    case 6:
      pumpsMenu();
      break;
    case 7:
      goBack();
      break;
    }
  }
  Serial.println("mainMenu() called");
}

byte menuItemNav( byte numberItems, byte currentItemNumber ) {
  // must take into account button polling speed and state changes
  uint8_t buttons = lcd.readButtons();
  byte itemNum = currentItemNumber;

  if (buttons & BUTTON_RIGHT ) {
    if (buttons){
      if (itemNum == numberItems ) {
        return itemNum = 0;
      }
      else {
        return itemNum++;
      }
    }
  }
  if (buttons & BUTTON_LEFT ) {
    if (buttons){
      if (itemNum == 0 ) {
        return itemNum = numberItems;
      }
      else {
        return itemNum--;
      }
    }
  }
  Serial.println("menuItemNav() called: ");
}

bool lcdTimeOut() {
  uint8_t buttons = lcd.readButtons();
  unsigned long lcdInterval = LCD_TIME_OUT;    // seconds to lcd timeout
  if ( !buttons) {
    if ( lcdTimer.CheckTimer( lcdInterval )) {
      lcd.clear();
      lcd.setBacklight(OFF);
      return true;
    }
    else {
      return false;
    }
  }
  else {
    Serial.println("lcdTimeOut() buttons true");
    return false;
  }
}

void nowMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("NOW MENU");
  return;
}
void qtrMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("QTR MENU");
  return;
}
void hrsMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("NOW MENU");
  return;
}
void dayMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("DAY MENU");
  return;
}
void fansMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("FAN MENU");
  return;
}
void pumpsMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PUMP MENU");
  return;
}
void goBack() {
  return;
}


