#include "MessageValidation.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

MessageValidation::MessageValidation() {
    receptionErrors = 0;
}

uint8_t MessageValidation::convertHexChar(char charToConvert) {
    if ((charToConvert >= '0') && (charToConvert <= '9')){
        return charToConvert - '0';
    }

    if ((charToConvert >= 'A') && (charToConvert <= 'F')){
        return charToConvert - 55;
    }

    if ((charToConvert >= 'a') && (charToConvert <= 'f')){
        return charToConvert - 87;
    }

    return 0;
}


char* MessageValidation::appendCheckSum(char *msg) {

    int i;
    uint8_t checkSum = 0;
    uint16_t charTotal = 0;
    char CheckSumSuffix[6];

    if (msg == NULL) return NULL;

    int len = strlen(msg);

    for (i = 0; i < len; i++) {
        charTotal += msg[i];
    }

    checkSum = (uint8_t) (charTotal & 0x00ff);

    sprintf(msg, "%s,*%02X", msg, checkSum);

    return msg;
}

// Validates the message with the checksum
// If valid, it will return the length of the message minus the checksum characters (>0)
int MessageValidation::validateMessage(char* msg) {

    int len = strlen(msg);

    if (len < 5) return -1;

    if ((msg[len - 4] != ',') || (msg[len - 3] != '*')) {
        return -1;
    }

    int i;
    uint8_t checkVal = convertHexChar(msg[len - 2]) * 16 + convertHexChar(msg[len - 1]);
    uint8_t checkSum;

    for (i = 0, checkSum = 0; i<len - 4; i++) {
        checkSum += msg[i];
    }

    if (checkSum == checkVal) return len - 4;
    
    receptionErrors++;

    return -1;
}

int MessageValidation::getReceptionErrorCount() {

    return receptionErrors;
}