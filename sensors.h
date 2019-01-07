/*
sensor data management class
License: see open source license file in repo https://github.com/Witz0/Climate-Monitoring-and-Control-Station
*/

#ifndef sensors_h
#define sensors_h
#include "Arduino.h"

class Sensors
  {
  public:
    uint16_t framWriteAddress;
    uint16_t framReadAddress;

    Sensors();

    CheckSensors();

    SensorioUpdate( uint16_t framWriteAddress, uint16_t framReadAddress );

    WriteField( sensorData &sensorDataWr, uint16_t framWriteAddress );

    ReadField( sensorData &sensorDataRd, uint16_t framReadAddress );

    HrlyAverages( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd );

    DailyAverages( sensorData &sensorDataWr, sensorData &sensorDataAvg, sensorData &sensorDataRd );

  private:

    struct sensorData {
      uint8_t humy1;
      int itmp1;
      uint8_t humy2;
      int itmp2;
      uint16_t pressurehPa;
      int itmp0;
    };

    sensorData sensorDataWr;
    sensorData sensorDataRd;
    sensorData sensorDataAvg;

    float tempCon;
    float pressureCon;

  };

  #endif
