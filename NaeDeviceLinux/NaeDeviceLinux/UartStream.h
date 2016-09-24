#include <string.h>         // strlen  
#include <stdio.h>          // printf
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <string.h>         // strlen  
#include "nmea.h"
#include "GpsProcessor.h"

#define UART_DATA_BUFFER_LEN        1024
#define UART_DATA_BUFFER_XTRA       128
#define UART_DATA_LEN_MAX           (UART_DATA_BUFFER_LEN - 1)
#define HIGH_SPEED                  (B38400 | CS8 | CLOCAL | CREAD)
#define LOW_SPEED                   (B9600 | CS8 | CLOCAL | CREAD)
#define USB_NAME_MAX_LEN            48

class UartStream
{
private:
    tcflag_t speed_flag;
    int uart_filestream;
    
    char _usb_number;

    int buffer_position;
    
    void(*process_data)(char*);
    void(*invalid_data)(char*);

    char dataBuffer[UART_DATA_BUFFER_LEN];
    char dataLine[UART_DATA_BUFFER_LEN];
    char rx_buffer[UART_DATA_BUFFER_LEN];

    char _usb_name[32];

    //nmeaPARSER parser;
    //nmeaINFO info;

    GpsProcessor gpsProcessor;

    void Initialize(char usbNumber);

public:
    UartStream(char usbNumber);
    UartStream(char usbNumber, void(*processDataFunction) (char*));
    UartStream(char usbNumber, void(*processDataFunction) (char*), bool highSpeed);
    UartStream(char usbNumber, void(*processDataFunction) (char*), bool highSpeed, char* usbName);
    UartStream(char usbNumber, void(*processDataFunction)(char*), void(*invalidDataFunction)(char*), bool highSpeed, char* usbName);

    UartStream();
    ~UartStream();

    void uart_transmit(char* data);
    int uart_receive();
	int uart_receive(char* xferBuffer);
    int get_fileId();

    char* get_usb_name();

    void setupGps();
    GpsData* parseGps(char* buffer);

    void closeFileStream();

    void print_error(char* msg);
};

