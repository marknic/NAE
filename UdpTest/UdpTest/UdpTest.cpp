#include <iostream>
#include <sys/select.h>
#include <cstring>
#include <cstdlib>                // For atoi()

#include "PracticalSocket.h"

using namespace std;

const int ECHOMAX = 255;          // Longest string to echo

int main(int argc, char *argv[]) {
    //if ((argc < 3) || (argc > 4)) {   // Test for correct number of arguments
    //    cerr << "Usage: " << argv[0]
    //        << " <Server> <Echo String> [<Server Port>]\n";
    //    exit(1);
    //}

    string servAddress = "192.168.3.143";             // First arg: server address
    char* echoString = "This is a test";               // Second arg: string to echo

    int echoStringLen = strlen(echoString);   // Length of string to echo
    
    if (echoStringLen > ECHOMAX) {    // Check input length
        cerr << "Echo string too long" << endl;
        exit(1);
    }
    
    unsigned short echoServPort = 11000; //Socket::resolveService(
        //(argc == 4) ? argv[3] : "echo", "udp");

    try {
        UDPSocket sock;

        // Send the string to the server
        sock.sendTo(echoString, echoStringLen, servAddress, echoServPort);

        // Receive a response
        char echoBuffer[ECHOMAX + 1];       // Buffer for echoed string + \0
        int respStringLen;                  // Length of received response
        if ((respStringLen = sock.recv(echoBuffer, ECHOMAX)) != echoStringLen) {
            cerr << "Unable to receive" << endl;
            exit(1);
        }

        echoBuffer[respStringLen] = '\0';             // Terminate the string!
        cout << "Received: " << echoBuffer << endl;   // Print the echoed arg

                                                      // Destructor closes the socket

    }
    catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}