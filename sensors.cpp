#include "sensors.h"

Sensors::Sensors() {
}

Sensors::CheckSensors() {

}

Sensors::SensorioUpdate() {

  //float pressurehPa;

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

Sensors::WriteField( sensorData &sensorDataWr, uint16_t framWriteAddress ) {
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

Sensors::ReadField( sensorData &sensorDataRd, uint16_t framReadAddress ) {
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

Sensors::HrlyAverages( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
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

Sensors::DailyAverages( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd ) {
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
