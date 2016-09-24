#ifndef _GPS_PROCESSOR_H_

#include "nmea.h"
#include <string.h>
#include "GpsData.h"

#define _GPS_PROCESSOR_H_ 1





class GpsProcessor
{

private:
    nmeaINFO info;
    nmeaPARSER parser;
    GpsData gpsData;

public:
    GpsProcessor();
    ~GpsProcessor();
    nmeaINFO* Parse(char*);

    int getFix();
    int getSatellites();
    GpsData* getGpsData();


};

#endif