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
Timer lcdTimer; //instance of Timer for LCD Backlight Time Out
Timer ioTimer; //instance of Timer for sensor gets

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

  fram.write8( FRAM_ADDR_RESERV_0, true );  //set state to true for reboot when setup runs
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

  static uint16_t framWriteAddress = FRAM_ADDR_FIRST_QTR;
  static uint16_t framReadAddress = FRAM_ADDR_FIRST_QTR;

  static sensorData sensorDataWr;
  static sensorData sensorDataRd;
  static sensorData sensorDataAvg;

  static uint8_t dailyIOEvents = 24 * UPDATES_PER_HOUR;
  unsigned long ioInterval = 3600000 / UPDATES_PER_HOUR;    //millis per hour / updates per hour

  if ( fram.read8( FRAM_ADDR_RESERV_0 ) == true ) {      //check state for reboot
    fram.write8( FRAM_ADDR_RESERV_0, false );
    Serial.print("framWriteAddress on reboot(): ");
    Serial.println(framWriteAddress, HEX );
    if ( sensorioUpdate( sensorDataWr ) == true ) {
      writeField( sensorDataWr, framWriteAddress );
      Serial.print("Time = ");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(".");
      Serial.println(second());
      Serial.print("framWriteAddress first boot post write(): ");
      Serial.println(framWriteAddress, HEX );
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      dailyIOEvents--;
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      Serial.println(" ");
    }
    if( fram.read16( FRAM_ADDR_LAST_QTR ) != FRAM_ADDR_FIRST_QTR ) {
      framReadAddress = fram.read16( FRAM_ADDR_LAST_QTR );
    }
    else {
      framReadAddress = FRAM_ADDR_FIRST_QTR;
    }
  }

  if ( ioTimer.CheckTimer( ioInterval )) {
    if ( sensorioUpdate( sensorDataWr ) == true ) {
      if( fram.read16( FRAM_ADDR_LAST_QTR ) != FRAM_ADDR_FIRST_QTR ) {
        framWriteAddress = fram.read16( FRAM_ADDR_LAST_QTR );
      }
      else {
        framWriteAddress = FRAM_ADDR_FIRST_QTR;
      }
      writeField( sensorDataWr, framWriteAddress );    //redo logic similar to funcs
      Serial.print("Time = ");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(".");
      Serial.println(second());
      Serial.println(" ");
      Serial.print("framWriteAddress in loop timer: ");
      Serial.println(framWriteAddress, HEX );
      Serial.println(" ");
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      dailyIOEvents--;
      Serial.print("dailyIOEvents:  ");
      Serial.println(dailyIOEvents);
      Serial.println(" ");
      if ( dailyIOEvents % UPDATES_PER_HOUR == 0 ) {
        hrlyavgs( sensorDataWr, sensorDataAvg, sensorDataRd );
        if ( dailyIOEvents == 0 ) {
          dailyIOEvents = 24 * UPDATES_PER_HOUR;
          dailyavgs( sensorDataWr, sensorDataAvg, sensorDataRd );
          if ( framWriteAddress == 32768 ) {    // set this to some lower multiple of sizeof(sensorData) - static reserves & segments
            fram.write16( FRAM_ADDR_LAST_DAY, FRAM_ADDR_FIRST_DAY );
          }
        }
      }
    }
    if( fram.read16( FRAM_ADDR_LAST_QTR ) != FRAM_ADDR_FIRST_QTR ) {
      framReadAddress = fram.read16( FRAM_ADDR_LAST_QTR );
    }
    else {
      framReadAddress = FRAM_ADDR_FIRST_QTR;
    }
  }

  if ( buttons ) {
    Serial.println("buttons");

    lcdDrawHome( sensorDataWr );
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

bool sensorioUpdate( sensorData &sensorDataWr ) {
  Serial.println("SensorioUpdate call: ");
  float tempCon;
  float pressureCon;

  //DHT22 is a slow sensor so reading takes about 250 millis and data may be 2 seconds old.

  //DHT22 Read Sensor 1
  sensorDataWr.humy1 = byte( dht1.readHumidity( ));
  // Read temperature as Fahrenheit
  sensorDataWr.itmp1 = byte( dht1.readTemperature(true));

  // Check if any reads failed and exit early (to try again).
  if (isnan(sensorDataWr.humy1) || isnan(sensorDataWr.itmp1)) {
    Serial.println("Failed to read DHT sensor! 01");
    return false;
  }
  Serial.print("Humy01: ");
  Serial.println(sensorDataWr.humy1);
  Serial.print("FTemp01: ");
  Serial.println(sensorDataWr.itmp1);
  Serial.println(" ");

  //DHT22 Read Sensor 2
  sensorDataWr.humy2 = byte( dht2.readHumidity( ));
  // Read temperature as Fahrenheit
  sensorDataWr.itmp2 = byte( dht2.readTemperature(true));
  // Check if any reads failed and exit early (to try again).
  if (isnan(sensorDataWr.humy2) || isnan(sensorDataWr.itmp2)) {
    Serial.println("Failed to read from DHT sensor! 02");
    return false;
  }
  Serial.print("Humy02: ");
  Serial.println(sensorDataWr.humy2);
  Serial.print("FTemp02: ");
  Serial.println(sensorDataWr.itmp2);
  Serial.println(" ");

  // MP115A2 Sensor Read
  /*Note: conversions to english units and before stores
   C to F (9/5)=1.8 +32 and kPa to hPa
   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   */
  sensorDataWr.pressurehPa = 0;
  sensorDataWr.itmp0 = 0;
  pressureCon = mpl115a2.getPressure( );
  sensorDataWr.pressurehPa = byte(pressureCon);

  Serial.print("Pressure int(): ");
  Serial.println(sensorDataWr.pressurehPa);
  Serial.println(" ");

  tempCon = mpl115a2.getTemperature( );
  tempCon = (( tempCon * 1.8 ) + 32);
  sensorDataWr.itmp0 = tempCon;

  Serial.print("Temp0): ");
  Serial.print(sensorDataWr.itmp0);
  Serial.println("F");
  Serial.println(" ");

  return true;

}
/*not modular enough rewrite with func or two
 so individual addresses & values are handled by in single func call
 framWriteAddress = write/readData( framAddress, sensorDataCopy )
 */
bool writeField( sensorData &sensorDataWr, uint16_t framWriteAddress ) {
  Serial.println("writeField() called: ");
  Serial.println(" ");

  Serial.println("framWriteAdress in writeField(): "); 
  fram.write8( framWriteAddress, sensorDataWr.humy1 );
  Serial.println(framWriteAddress, HEX );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.humy1 ));
  fram.write8( framWriteAddress, sensorDataWr.itmp1 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp1 ));
  fram.write8( framWriteAddress, sensorDataWr.humy2 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.humy2 ));
  fram.write8( framWriteAddress, sensorDataWr.itmp2 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp2 ));
  fram.write8( framWriteAddress, sensorDataWr.pressurehPa );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.pressurehPa ));
  fram.write8( framWriteAddress, sensorDataWr.itmp0 );
  framWriteAddress = framWriteAddress + ( sizeof( sensorDataWr.itmp0 ));
  fram.write16( FRAM_ADDR_LAST_QTR, framWriteAddress );
  Serial.println(framWriteAddress, HEX );
  return true;
}

bool readField( sensorData &sensorDataRd, uint16_t framReadAddress ) {
  Serial.println("readField() called: ");
  Serial.println(" ");

  Serial.println("framReadAdress in readField(): ");
  sensorDataRd.humy1 = fram.read8( framReadAddress );
  Serial.println(framReadAddress, HEX );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.humy1 ));
  sensorDataRd.itmp1 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp1 ));
  sensorDataRd.humy2 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.humy2 ));
  sensorDataRd.itmp2 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp2 ));
  sensorDataRd.pressurehPa = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.pressurehPa ));
  Serial.println(framReadAddress, HEX );
  sensorDataRd.itmp0 = fram.read8( framReadAddress );
  framReadAddress = framReadAddress + ( sizeof( sensorDataRd.itmp0 ));
  Serial.println(framReadAddress, HEX );
  return true;
}


bool hrlyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
  Serial.println("hrlyavgs() called: ");
  Serial.println(" ");

  uint16_t framReadAddress = FRAM_ADDR_FIRST_QTR;
  uint16_t framWriteAddress;
  sensorDataAvg.humy1 = 0;
  sensorDataAvg.itmp1 = 0;
  sensorDataAvg.humy2 = 0;
  sensorDataAvg.itmp2 = 0;
  sensorDataAvg.pressurehPa = 0;
  sensorDataAvg.itmp0 = 0;

  if( fram.read16( FRAM_ADDR_LAST_HR ) != FRAM_ADDR_FIRST_HR ) {
    framWriteAddress = fram.read16( FRAM_ADDR_LAST_HR );
  }
  else {
    framWriteAddress = FRAM_ADDR_FIRST_HR;
  }

  for ( byte updates = UPDATES_PER_HOUR; updates > 0; updates-- ) {
    readField( sensorDataRd, framReadAddress );
    framReadAddress = framReadAddress + sizeof(sensorDataWr);
    framWriteAddress = framWriteAddress + sizeof(sensorDataWr);
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
  Serial.println(framWriteAddress, HEX );
  writeField( sensorDataWr, framWriteAddress );
  Serial.println(framWriteAddress, HEX );
  ;
  framWriteAddress = framWriteAddress + sizeof(sensorDataWr);
  Serial.println(framWriteAddress, HEX );
  fram.write16( FRAM_ADDR_LAST_HR, framWriteAddress );
  framWriteAddress = FRAM_ADDR_FIRST_QTR;
  return true;
}

bool dailyavgs( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
  Serial.println("dailyavgs() called: ");
  Serial.println(" ");
  uint16_t framReadAddress = FRAM_ADDR_FIRST_HR;
  uint16_t framWriteAddress;
  sensorDataAvg.humy1 = 0;
  sensorDataAvg.itmp1 = 0;
  sensorDataAvg.humy2 = 0;
  sensorDataAvg.itmp2 = 0;
  sensorDataAvg.pressurehPa = 0;
  sensorDataAvg.itmp0 = 0;

  fram.write16( FRAM_ADDR_LAST_HR, FRAM_ADDR_FIRST_HR );    //resets first address for hourlyavgs function

  if( fram.read16( FRAM_ADDR_LAST_DAY ) != FRAM_ADDR_FIRST_DAY ) {
    framWriteAddress = FRAM_ADDR_LAST_DAY;
  }
  else {
    framWriteAddress = fram.read16( FRAM_ADDR_FIRST_DAY);
  }

  for ( byte hours = 24; hours > 0; hours-- ) {
    readField( sensorDataRd, framReadAddress );
    framReadAddress = framReadAddress + sizeof(sensorDataRd);
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
  Serial.println(framWriteAddress, HEX );
  writeField( sensorDataWr, framWriteAddress );
  Serial.println(framWriteAddress, HEX );
  framWriteAddress = framWriteAddress + sizeof(sensorDataRd);
  Serial.println(framWriteAddress, HEX );
  fram.write16( FRAM_ADDR_LAST_DAY, framWriteAddress );
  framWriteAddress = FRAM_ADDR_FIRST_QTR;
  return true;
}

void lcdDrawHome( sensorData &sensorDataWr ) {
  lcd.setBacklight(OFF);
  lcd.setBacklight(ON);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(sensorDataWr.humy1);
  lcd.print("%");
  lcd.setCursor(4,0);
  lcd.print(sensorDataWr.itmp1);
  lcd.print("F");
  lcd.setCursor(8,0);
  lcd.print(sensorDataWr.humy2);
  lcd.print("%");
  lcd.setCursor(12,0);
  lcd.print(sensorDataWr.itmp2);
  lcd.print("F");
  lcd.setCursor(0,1);
  lcd.print(sensorDataWr.pressurehPa);
  lcd.print("P");
  lcd.setCursor(4,1);
  lcd.print(sensorDataWr.itmp0);
  lcd.print("F");
  lcd.setCursor(9,1);
  //lcd.print("E");
  //lcd.print(dailyIOEvents);
  lcd.setCursor(12,1);
  lcd.print("MENU");
  lcd.setCursor(12,1);
  lcd.blink();
  Serial.println("lcdDrawHome() called: ");
  Serial.println(" ");
}

//need to do better design maybe menu class with enumerated items maybe try printing enumsfor menu names and use struct for lcd x,y?
void mainMenu() {

  uint8_t buttons = lcd.readButtons();
  //if (first run) fix this so menu data is set once to reduce cpu
  byte numItems = 7;
  static byte mainMenuCurrentItemNum = 0;

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

  mainMenuCurrentItemNum = menuItemNav( numItems, mainMenuCurrentItemNum );

  if (buttons & BUTTON_SELECT ) {
    switch (mainMenuCurrentItemNum) {
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
}

byte menuItemNav( byte numberItems, byte currentItemNumber ) {
  Serial.println("menuItemNav() called: ");
  Serial.println(" ");
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






