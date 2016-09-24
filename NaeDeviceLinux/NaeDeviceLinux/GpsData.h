#ifndef _GPS_DATA_H_

#define _GPS_DATA_H_

#include <stdint.h>
#include "nmea.h"

class GpsData
{

private:
    double latitude;
    double longitude;
    double altitude;
    uint8_t fix;
    uint8_t satellites;
    double speed;
    double direction;


public:
    GpsData();
    ~GpsData();

    uint8_t getFix();
    uint8_t getSatellites();
    double getLatitude();
    double getLongitude();
    double getAltitude();
    double getSpeed();
    double getDirection();

    void setFix(uint8_t);
    void setSatellites(uint8_t);
    void setLatitude(double);
    void setLongitude(double);
    void setAltitude(double);
    void setSpeed(double);
    void setDirection(double);


};


#endif