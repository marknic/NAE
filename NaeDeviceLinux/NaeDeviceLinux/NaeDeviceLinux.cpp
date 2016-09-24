#include <iostream>
#include <cstdio>
#include <sys/time.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h> /* struct stat, fchmod (), stat (), S_ISREG, S_ISDIR */
#include "UartStream.h"
#include "MessageValidation.h"
#include <stdint.h>

using namespace std;

#ifndef TRUE
#define TRUE                             (1==1)
#endif

#ifndef FALSE
#define FALSE                            (1==2)
#endif

#define TELEMETRY_LOG_FILE					"/home/pi/PegasusMission/telemetryLog.txt"
#define IS_GPS								  1
#define IS_PROC2							  2
#define GPS_ALTITUDE_INVALID               -1.0

#define MPH_PER_KNOT          1.150779448023543
#define KPH_PER_KNOT                      1.852

#define DO_SPEED_IN_MPH                   FALSE          
#define DO_SPEED_IN_KPH                    TRUE        

#define TELEMETRY_DATA_LEN                  256
#define GENERAL_BUFFER_LEN                  128
#define GPS_DATA_BUFFER_LEN                  64
#define PROC1_BUFFER_LEN					 64

#define PROC1_DATA_COUNT                     12

#define DATA_ARRAY_COUNT                     39
#define DATA_ARRAY_STR_LEN                   16



// To Run on the RPi:
//  cd /tmp/VisualGDB/c/_git/NAE/NaeDeviceLinux/NaeDeviceLinux/Debug
//  ./NaeDeviceLinux

time_t _initial_time;
time_t _current_time;

UartStream* _serialStream_Gps1;             // Primary GPS
UartStream* _serialStream_subProc2;			// MC2 sensor data stream
UartStream* _serialStream_Gps2;				// Secondary GPS
UartStream* _serialStream_Radio;			// 900 MHz Data Modem

bool _gps_data1_attached = FALSE;

// GPS Data Parsers
GpsData* _gps1_data = NULL;

int _telemetry_count = 0;

MessageValidation _messageValidator;


double _gps1_altitude = -1.0;
double _gps2_altitude = -1.0;
uint8_t _gps1_gotFix = 0;


int file_exists(char* filename)
{
    if (filename == NULL) return -1;

    struct stat sts;

    if (stat(filename, &sts) == -1)
    {
        return 0;
    }

    return 1;
}


// Check a string to see if it begins with the provided characters
bool starts_with(char* stringToCheck, char* startsWith)
{
    if ((stringToCheck == NULL) || (startsWith == NULL)) return FALSE;

    int stringToCheckLen = strlen(stringToCheck);
    int startsWithLen = strlen(startsWith);

    if ((stringToCheckLen == 0) || (startsWithLen == 0)) return FALSE;
    if (stringToCheckLen < startsWithLen) return FALSE;

    for (int i = 0; i < startsWithLen; i++)
    {
        if (stringToCheck[i] != startsWith[i]) return FALSE;
    }

    return TRUE;
}


char* format_timestamp_with_milliseconds(char* timestampDest)
{
    if (timestampDest == NULL) return NULL;

    struct timeval timeNow;

    int mtime;

    gettimeofday(&timeNow, NULL);

    mtime = timeNow.tv_usec / 1000.0 + 0.5;

    _current_time = time(NULL);
    struct tm tm = *gmtime(&_current_time);

    sprintf(timestampDest,
        "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        mtime);

    return timestampDest;
}
void initialize_log(char* filename)
{
    FILE* fd;

    if (filename == NULL) {

        printf("\n>>>>> Tried to initialize the log file but the filename was null\n\n");

        return;
    }

    if (!file_exists(filename))
    {
        ssize_t nrd;

        fd = fopen(filename, "w");

        if (fd != NULL)
        {
            fprintf(fd, "INIT,Logfile Initialized,INIT\n");

            fflush(fd);

            fclose(fd);
        }
    }
    else
    {
        fd = fopen(TELEMETRY_LOG_FILE, "a");

        if (fd == NULL) {
            printf("Couldn't open logfile for appending.\n");

            return;
        }

        fprintf(fd, "INIT,Logfile Initialized,INIT\n");


        fflush(fd);

        /* close the file */
        fclose(fd);
    }

}



void write_to_log(const char* source, char* data) {

    char timestamp[36];
    //char telemetry_string[256];

    FILE* fp;

    if ((source == NULL) || (data == NULL)) return;

    /* open the file for append */
    fp = fopen(TELEMETRY_LOG_FILE, "a");

    if (fp == NULL) {
        printf("I couldn't open results.dat for appending.\n");

        return;
    }

    format_timestamp_with_milliseconds(timestamp);

    fprintf(fp, "%s,%s,%s", source, timestamp, data);

    fflush(fp);

    /* close the file */
    fclose(fp);
}


int test_uart_streams(char uartNumber)
{
    UartStream* _serialStream_Test = new UartStream(uartNumber); //, process_radio_data, FALSE, "test");

    char tstBuffer[1024];
    int result = 0;

    usleep(1000000);

    for (int i = 0; i < 4; i++)
    {
        int bytes = _serialStream_Test->uart_receive(tstBuffer);

        if (bytes > 0)
        {
            _serialStream_Test->closeFileStream();

            if ((tstBuffer[0] == '$') && (tstBuffer[1] == 'G') && (tstBuffer[2] == 'P'))
            {
                result = IS_GPS;
            }
            else
            {
                result = IS_PROC2;
            }
        }
    }

    _serialStream_Test->closeFileStream();

    delete _serialStream_Test;

    return result;
}



/**
* \fn convert_knots_to_mph
* \brief convert the current speed from knots per hour
* \return speed in miles per hour
*/
double convert_knots_to_mph(double speedKnots) {
    return speedKnots * MPH_PER_KNOT;
}


/**
* \fn convert_knots_to_kph
* \brief convert the current speed from knots per hour
* \return speed in kilometers per hour
*/
double convert_knots_to_kph(double speedKnots) {
    return speedKnots * KPH_PER_KNOT;
}

/**
* \fn do_speed_conversion
* \brief determines the desired speed format and does the conversion
* \return speed in desired measure per hour
*/
double do_speed_conversion(double speedKnots) {

    if (DO_SPEED_IN_MPH) {
        return convert_knots_to_mph(speedKnots);
    }

    if (DO_SPEED_IN_KPH) {
        return convert_knots_to_kph(speedKnots);
    }

    return speedKnots;
}


double _gpsLat1 = 0.0;

char _gpsDataString1[GPS_DATA_BUFFER_LEN];
char _gpsDataString2[GPS_DATA_BUFFER_LEN];

char _lastGpsDataString[GPS_DATA_BUFFER_LEN];
char _craftMessage[TELEMETRY_DATA_LEN];
char _radioMessage[TELEMETRY_DATA_LEN];

char _proc1_dataArray[PROC1_DATA_COUNT][DATA_ARRAY_STR_LEN];



void process_gps1_invalid_data(char* dataLine) {
    write_to_log("GP1-Invalid", dataLine);
}






/**
* \fn process_radio_data
* \brief Process incoming radio data (commands)
*/
void process_radio_data(char* dataLine) {

    write_to_log("RADIO", dataLine);

    int len = strlen(dataLine);

    if (dataLine[len - 1] == '\n') {
        dataLine[len - 1] = 0;
    }

    int checksumPos = _messageValidator.validateMessage(dataLine);

    if ((checksumPos != -1) && (checksumPos < 256)) {
        // Valid Message
        strcpy(_radioMessage, dataLine);

        _radioMessage[checksumPos] = 0;

        processCommand(_radioMessage);
    }
}



/**
* \fn process_gps1_data
* \brief convert the incoming raw GPS data to a usable format
*/
void process_gps1_data(char* dataLine) {

    if ((dataLine == NULL) || (!starts_with(dataLine, "$GPRMC") && !starts_with(dataLine, "$GPGGA"))) return;

    write_to_log("GP1", dataLine);

    _gps_data1_attached = FALSE;

    _gps1_data = _serialStream_Gps1->parseGps(dataLine);

    if (_gps1_data) {

        _gps_data1_attached = TRUE;

        double speed = _gps1_data->getSpeed();

        speed = do_speed_conversion(speed);

        if (speed < 1.0) {
            speed = 0.1;
        }

        _gps1_gotFix = _gps1_data->getFix();
        double alt = _gps1_data->getAltitude();

        if (_gps1_gotFix && (alt > 0.0))
        {
            _gps1_altitude = alt;
        }
        else
        {
            _gps1_altitude = GPS_ALTITUDE_INVALID;
        }

        _gpsLat1 = _gps1_data->getLatitude();

        sprintf(_gpsDataString1,
            "%7.5f,%7.5f,%3.1f,%3.1f,%3.1f,%d,%d",
            _gpsLat1,
            _gps1_data->getLongitude(),
            alt,
            speed,
            _gps1_data->getDirection(),
            _gps1_gotFix,
            _gps1_data->getSatellites());
    }
}


void initialize_serial_devices()
{

    if (test_uart_streams('3') == IS_GPS)
    {
        _serialStream_Gps1 = new UartStream('3', process_gps1_data, process_gps1_invalid_data, FALSE, "GPS 1");
        _serialStream_Radio = new UartStream('2', process_radio_data, process_radio_invalid_data, TRUE, "Radio Modem");
        write_to_log("InitDevices", "\nGPS1-3, Radio-2\n");
    }
    else
    {
        _serialStream_Gps1 = new UartStream('2', process_gps1_data, process_gps1_invalid_data, FALSE, "GPS 1");
        _serialStream_Radio = new UartStream('3', process_radio_data, process_radio_invalid_data, TRUE, "Radio Modem");
        write_to_log("InitDevices", "\nGPS1-2, Radio-3\n");
    }

    if (test_uart_streams('1') == IS_GPS)
    {
        _serialStream_subProc2 = new UartStream('0', process_mc2_data, process_mc2_invalid_data, FALSE, "MicroController 2");
        _serialStream_Gps2 = new UartStream('1', process_gps2_data, process_gps2_invalid_data, FALSE, "GPS 2");
        write_to_log("InitDevices", "GPS2-1, Proc2-0\n\n");
    }
    else
    {
        _serialStream_subProc2 = new UartStream('1', process_mc2_data, process_mc2_invalid_data, FALSE, "MicroController 2");
        _serialStream_Gps2 = new UartStream('0', process_gps2_data, process_gps2_invalid_data, FALSE, "GPS 2");
        write_to_log("InitDevices", "GPS2-0, Proc2-1\n\n");
    }
}


int main(int argc, char *argv[])
{
    initialize_log(TELEMETRY_LOG_FILE);

    _altitude_calculation.Initialize(2.8, 188.0, 1004.5);

    initialize_serial_devices();
}