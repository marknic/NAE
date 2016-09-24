#include "GpsProcessor.h"


GpsProcessor::GpsProcessor()
{
    nmea_zero_INFO(&info);
    nmea_parser_init(&parser);
}


GpsProcessor::~GpsProcessor()
{
    
    nmea_parser_destroy(&parser);
    
}

nmeaINFO* GpsProcessor::Parse(char* dataLine) {
    
    int result = nmea_parse(&parser, dataLine, strlen(dataLine), &info);


    if (result > 0) {

        gpsData.setLatitude(nmea_ndeg2degree(info.lat));
        gpsData.setLongitude(nmea_ndeg2degree(info.lon));
        gpsData.setSatellites(info.satinfo.inview);
        gpsData.setFix(info.fix == 0 ? 0 : 1);
        gpsData.setSpeed(info.speed);
        gpsData.setDirection(info.direction);
        gpsData.setAltitude(info.elv);

        return &info;
    }

}

GpsData* GpsProcessor::getGpsData() {
    return &gpsData;
}