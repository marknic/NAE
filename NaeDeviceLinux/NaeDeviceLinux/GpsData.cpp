#include "GpsData.h"

GpsData::GpsData() {
    latitude = 0.0;
    longitude = 0.0;
    altitude = 0.0;
    speed = 0.0;
    fix = 0;
    satellites = 0;
    direction = 0.0;
}

GpsData::~GpsData() {

}

uint8_t GpsData::getFix() {
    return fix;
}


uint8_t GpsData::getSatellites() {
    return satellites;
}



double GpsData::getLatitude() {
    return latitude;
}



double GpsData::getLongitude() {
    return longitude;
}



double GpsData::getAltitude() {
    return altitude;
}



double GpsData::getSpeed() {
    return speed;
}



double GpsData::getDirection() {
    return direction;
}



void GpsData::setFix(uint8_t value) {
    fix = value;
}

void GpsData::setSatellites(uint8_t value) {
    satellites = value;
}

void GpsData::setLatitude(double value) {
    latitude = value;
}

void GpsData::setLongitude(double value) {
    longitude = value;
}

void GpsData::setAltitude(double value) {
    altitude = value;
}

void GpsData::setSpeed(double value) {
    speed = value;
}

void GpsData::setDirection(double value) {
    direction = value;
}

