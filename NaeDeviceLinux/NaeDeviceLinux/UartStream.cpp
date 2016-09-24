#include "UartStream.h"


UartStream::UartStream() {

}


UartStream::UartStream(char usbNumber, void(*processDataFunction)(char*), void(*invalidDataFunction)(char*), bool highSpeed, char* usbName) {

    if (highSpeed) {
        speed_flag = HIGH_SPEED;
    }
    else {
        speed_flag = LOW_SPEED;
    }

    Initialize(usbNumber);

    process_data = processDataFunction;
    invalid_data = invalidDataFunction;
    
    int len = strlen(usbName);

    if (len >= USB_NAME_MAX_LEN) {
        strncpy(_usb_name, usbName, USB_NAME_MAX_LEN - 1);
    }
    else {
        strcpy(_usb_name, usbName);
    }
}


UartStream::UartStream(char usbNumber, void(*processDataFunction) (char*), bool highSpeed, char* usbName) {

    if (highSpeed) {
        speed_flag = HIGH_SPEED;
    }
    else {
        speed_flag = LOW_SPEED;
    }

    Initialize(usbNumber);

    process_data = processDataFunction;

    int len = strlen(usbName);

    if (len >= USB_NAME_MAX_LEN) {
        strncpy(_usb_name, usbName, USB_NAME_MAX_LEN - 1);
    }
    else {
        strcpy(_usb_name, usbName);
    }
}


char* UartStream::get_usb_name() {
    return _usb_name;
}

UartStream::UartStream(char usbNumber, void(*processDataFunction) (char*), bool highSpeed) {

    if (highSpeed) {
        speed_flag = HIGH_SPEED;
    }
    else {
        speed_flag = LOW_SPEED;
    }

    Initialize(usbNumber);

    process_data = processDataFunction;

    _usb_name[0] = '\0';
}

UartStream::UartStream(char usbNumber, void(*processDataFunction) (char*)) {

    speed_flag = LOW_SPEED;

    Initialize(usbNumber);

    process_data = processDataFunction;

    _usb_name[0] = '\0';
}

UartStream::UartStream(char usbNumber)
{
    speed_flag = LOW_SPEED;

    Initialize(usbNumber);

    _usb_name[0] = '\0';
}

void UartStream::Initialize(char usbNumber) {
    //-------------------------
    //----- SETUP USART 0 -----
    //-------------------------
    //At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively


    //OPEN THE UART
    //The flags (defined in fcntl.h):
    //	Access modes (use 1 of these):
    //		O_RDONLY - Open for reading only.
    //		O_RDWR - Open for reading and writing.
    //		O_WRONLY - Open for writing only.
    //
    //	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
    //											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
    //											immediately with a failure status if the output can't be written immediately.
    //
    //	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
    //uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode

    if ((usbNumber < '0') || (usbNumber > '9')) {
        print_error("SETUP error - usbNumber must be '0' - '9'");
        return;
    }

    // Initialize the file handle variable
    uart_filestream = 0;

    _usb_number = usbNumber;

    // Init the function pointer
    process_data = NULL;

    // Initialize/clear the data buffers
    memset(dataBuffer, 0, UART_DATA_BUFFER_LEN);
    memset(dataLine, 0, UART_DATA_BUFFER_LEN);
    buffer_position = 0;

    char deviceName[16];

    strcpy(deviceName, "/dev/ttyUSB");
    deviceName[11] = usbNumber;
    deviceName[12] = 0;

    uart_filestream = open(deviceName, O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode

    if (uart_filestream == 0)
    {
        //ERROR - CAN'T OPEN SERIAL PORT
        print_error("OPEN error - Unable to open UART.  Ensure it is not in use by another application");
    }

    //CONFIGURE THE UART
    //The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
    //	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
    //	CSIZE:- CS5, CS6, CS7, CS8
    //	CLOCAL - Ignore modem status lines
    //	CREAD - Enable receiver
    //	IGNPAR = Ignore characters with parity errors
    //	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
    //	PARENB - Parity enable
    //	PARODD - Odd parity (else even)
    struct termios options;

    tcgetattr(uart_filestream, &options);
  
    options.c_cflag = speed_flag; 		//<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;

    tcflush(uart_filestream, TCIFLUSH);
    tcsetattr(uart_filestream, TCSANOW, &options);
}


GpsData* UartStream::parseGps(char* buffer) {

    gpsProcessor.Parse(buffer);

    return gpsProcessor.getGpsData();

}


UartStream::~UartStream()
{
}


void UartStream::print_error(char* msg) {

    char usbNoString[2];
    usbNoString[0] = _usb_number;
    usbNoString[1] = '\0';

    printf("UART ERROR: %s in USB: %s for %s\n", msg, usbNoString, _usb_name);
}

void UartStream::uart_transmit(char* data) {

    if (data == NULL) {
        print_error("TX error - writing failed - data is NULL");
        return;
    }

    int dataLen = strlen(data);

    if (dataLen > UART_DATA_LEN_MAX){
        print_error("TX error - writing failed - data length is too long");
        return;
    }

    if (uart_filestream != 0)
    {
        int count = write(uart_filestream, data, dataLen);		//Filestream, bytes to write, number of bytes to write
        if (count < 0)
        {
            print_error("TX error - writing failed");
        }
    }
}

int UartStream::uart_receive() {

    //----- CHECK FOR ANY RX BYTES -----
    if (uart_filestream != 0)
    {
        // Read characters from the port if they are there
        
        int rx_length = read(uart_filestream, (void*) rx_buffer, UART_DATA_LEN_MAX);		//Filestream, buffer to store in, number of bytes to read (max)

        if (rx_length > 0) {

            int i;
            for (i = 0; i < rx_length; i++) {

                // Invalid data
                if ((buffer_position + 1) >= UART_DATA_LEN_MAX) {
                    printf("Handling invalid data from UART\n");
                    
                    memcpy(dataLine, dataBuffer, buffer_position);
                    dataLine[buffer_position] = 0;
                        
                    buffer_position = 0;

                    if (invalid_data != NULL)
                    {
                        invalid_data(dataLine);
                    }
                 }

                dataBuffer[buffer_position++] = rx_buffer[i];

                // Handle end of line and (hopefully) good data
                if (rx_buffer[i] == '\n') {
                    // Shift buffer to line storage
                    memcpy(dataLine, dataBuffer, buffer_position);
                    dataLine[buffer_position] = 0;

                    // reset
                    buffer_position = 0;

                    // Call event to process the line data

                    if (process_data != NULL) {
                        process_data(dataLine);
                    }
                }
            }
            
            //printf("%i bytes read : %s\n", rx_length, rx_buffer);

            return rx_length;
        } 

        //else if (rx_length < 0)
        //{
        //    //An error occured (will occur if there are no bytes)
        //}
        //else // (rx_length == 0)
        //{
        //    //No data waiting
        //}
    }

    return 0;
}


int UartStream::uart_receive(char* xferBuffer) 
{
    //----- CHECK FOR ANY RX BYTES -----
    if (uart_filestream != 0)
    {
        // Read characters from the port if they are there
        int rx_length = read(uart_filestream, (void*) rx_buffer, UART_DATA_LEN_MAX);		//Filestream, buffer to store in, number of bytes to read (max)

        if (rx_length > 0) {

            int i;
            for (i = 0; i < rx_length; i++) {

                if ((buffer_position + 1) >= UART_DATA_LEN_MAX) {
                    printf("maybe over\n");

                    buffer_position = 0;
                }

                dataBuffer[buffer_position++] = rx_buffer[i];
                
                if (rx_buffer[i] == '\n') {
                    // Shift buffer to line storage
                    memcpy(dataLine, dataBuffer, buffer_position);
                    dataLine[buffer_position] = 0;
                    
                    strcpy(xferBuffer, dataLine);

                                        // reset
                    buffer_position = 0;
                }
            }

            return rx_length;
        } 
    }

    return 0;
}



int UartStream::get_fileId() {
    return uart_filestream;
}

void UartStream::closeFileStream() {
    if (uart_filestream > 0)
        close(uart_filestream);
}