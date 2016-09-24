
#ifndef _MESSAGEVALIDATION_h
#define _MESSAGEVALIDATION_h
#include <stdint.h>

class MessageValidation
{
private:
    double receptionErrors;

    uint8_t convertHexChar(char charToConvert);

public:
    MessageValidation();

    char* appendCheckSum(char *msg);
    int validateMessage(char* msg);
    int getReceptionErrorCount();
};


#endif

