#include <Wire.h>   
#include <NewliquidCrystal/LiquidCrystal_I2c.h>

#define RADIO_RX_PIN            10
#define RADIO_TX_PIN            11


#define RADIO_MSG_MAX_SIZE                                 64
#define RADIO_MSG_MAX_OFFSET         (RADIO_MSG_MAX_SIZE - 1)
#define RADIO_MSG_OFFSET_INIT                              -1

#define USERMSG_INDICATOR_CHAR                            '\n'
#define MESSAGE_SEPARATOR_CHAR                            '`'

#define RADIO_SERIAL_BAUD_RATE                          38400 //115200
#define RADIO_DEBUG_BAUD_RATE                          115200

#define DISPLAY_LINE_LENGTH         21
#define DISPLAY_LINE_COUNT           4
#define DISPLAY_MAX_LENGTH          20

#define DISPLAY_LINE_TEST_COUNT      7


char _userMessageIn[RADIO_MSG_MAX_SIZE];

uint16_t _userMsgInCharCount = RADIO_MSG_OFFSET_INIT;
char _msgOrigin[36];

// Buffer to hold the user message before sending to the LCD
char _displayLines[DISPLAY_LINE_COUNT][DISPLAY_LINE_LENGTH];

// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address





// Find a character in zero terminated string starting at character position
// and looking forward from there
int findChar(char* stringToSearch, char findThisChar, int startPos)
{
    if (stringToSearch == NULL) return -1;

    if (findThisChar == 0) return -1;

    if (startPos < 0) return -1;

    if (stringToSearch[0] == 0) return -1;

    int len = strlen(stringToSearch);

    if (startPos >= len) return -1;

    for (int i = startPos; i<len; i++)
    {
        if (stringToSearch[i] == findThisChar)
        {
            return i;
        }
    }

    return -1;
}


int findCharPair(char* stringToSearch, char findThisChar)
{
    if (stringToSearch == NULL) return 0;

    if (findThisChar == 0) return 0;

    memset(_msgOrigin, 0, sizeof _msgOrigin);

    int len = strlen(stringToSearch);

    //Serial.print("findCharPair len: ");Serial.println(len);

    for (int i = len-1; i>=0; i--)
    {
        //Serial.print("char: ");Serial.println(stringToSearch[i]);

        if (i > 0 && (stringToSearch[i] == findThisChar) && stringToSearch[i - 1] == findThisChar)
        {
            stringToSearch[i] = '\0';
            
            //Serial.print("len: ");Serial.println(len);
            //Serial.print("  i: ");Serial.println(i);
            //Serial.print("len - (i + 1): ");Serial.print(len - (i + 1));

            if (len - (i + 1) <= 20)
            {
                strcpy(_msgOrigin, &stringToSearch[i + 1]);

                //Serial.print("Origin Extracted: "); Serial.println(_msgOrigin);
            }

            return i - 1;
        }
    }

    return -1;
}


// Copy N characters (count) from source to destination.
void copyText(char* dest, char* source, int count)
{
    if (dest == NULL) return;
    if (source == NULL) return;

    int len = strlen(source);
    if ((count < 0) || count > len) return;

    for (int i = 0; i<count; i++)
    {
        dest[i] = source[i];
    }

    dest[count] = 0;
}


char _userMessage[90] =
//"Initializing the`Pegasus Mission`Sensor Device";
//    "OK, Let's roll!";
//    "North American Eagle`Wait, we're gonna go`how fast!?``0            Chicago";
//    "North American Eagle`Wait, we're gonna go`how fast!?``123   Cleveland Ohio";
//    "North American Eagle`Wait, we're gonna go`how fast!?";
"This is a test. Do`not be afraid!``0            Chicago";


// Zero out the entire display buffer with nulls
void zeroDisplay()
{
    for (int i = 0; i<DISPLAY_LINE_COUNT; i++)
    {
        memset(_displayLines[i], 0, DISPLAY_LINE_LENGTH);
    }
}


// Set null chars in the display to spaces
// This clears the display screen every time
// a message is sent
void blankDisplay()
{
    for (int i = 0; i<DISPLAY_LINE_COUNT; i++)
    {
        for (int j = 0; j<DISPLAY_LINE_LENGTH - 1; j++)
        {
            if (_displayLines[i][j] == '\0')
            {
                _displayLines[i][j] = ' ';
            }
        }
    }
}




// Split the message lines into the display buffer and 
// send them to the display
void displayUserMessage(char* userMessage)
{
    // zero out the display buffer
    zeroDisplay();

    Serial.print("userMessage 1: "); Serial.println(userMessage);

    int charPairPos = findCharPair(userMessage, MESSAGE_SEPARATOR_CHAR);

    //Serial.print("charPairPos: "); Serial.println(charPairPos);
    //Serial.print("userMessage 2: "); Serial.println(userMessage);

    if (charPairPos > -1)
    {
        strcpy(_displayLines[3], _msgOrigin);

        //Serial.print("_displayLines[3]: ");
        //Serial.println(_displayLines[3]);
    }

    int len = strlen(userMessage);
    if (userMessage[len] != '`')
    {
        //Serial.print("userMessage len: "); Serial.println(len);
        userMessage[len] = '`';
        userMessage[len + 1] = 0;

        //Serial.print("userMessage 3: "); Serial.println(userMessage);
    }

    // Look for the first line terminator character '`'
    int pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, 0);
    int line = 0;
    int lineStart = 0;

    while (pos > -1 && line < 3)
    {
        //Serial.print("lineStart: ");Serial.print(lineStart); Serial.print("  pos: "); Serial.print(pos); Serial.print("  chars to copy: "); Serial.println(pos - lineStart);

        // Copy that many characters to the next display line
        copyText(_displayLines[line], &userMessage[lineStart], pos - lineStart);

        //Serial.print("Line ");Serial.print(line); Serial.print(": "); Serial.println(_displayLines[line]);
        lineStart = pos + 1;
        line++;

        pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, pos + 1);
        //Serial.print(" pos: ");Serial.println(pos);
    }

    // Ensure the display is blank before displaying
    blankDisplay();

    //Serial.println("Sending output to LCD");

    for (int i = 0; i < DISPLAY_LINE_COUNT; i++)
    {
        //Serial.print(i);
        lcd.setCursor(0, i); //Start at character 4 on line 0
        lcd.print(_displayLines[i]);
        delay(50);

        // For testing, display the message on the serial monitor
        //Serial.print("'");
        //Serial.print(_displayLines[i]);
        //Serial.println("'");
    }
}


// Split the message lines into the display buffer and 
// send them to the display
//void displayText(char* userMessage)
//{
//    int lineStart = 0;
//    int line = 0;
//    int charsToCopy;
//
//    // zero out the display buffer
//    zeroDisplay();
//
//    int charPairPos = findCharPair(userMessage, MESSAGE_SEPARATOR_CHAR);
//
//    Serial.print("Received: ");
//    Serial.println(userMessage);
//
//    // Look for the first line terminator character '`'
//    int pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, 0);
//
//    // Loop through all lines in the message
//    if (pos > -1) {
//
//        Serial.print("user msg: "); Serial.println(userMessage);
//        // Loop
//        while (pos > -1) {
//
//            // How many chars in the line
//            charsToCopy = pos - lineStart;
//
//            // Copy that many characters to the next display line
//            copyText(_displayLines[line++], &userMessage[lineStart], charsToCopy);
//            
//            // Move to the next display line
//            lineStart = pos + 1;
//
//            // Is there another line to display?
//            pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, lineStart);
//
//            // If not, this is the last line so copy that to the display buffer
//            if (pos == -1)
//            {
//                // Get the length of the line
//                int len = strlen(userMessage);
//
//                // Is there text to copy
//                if (len > lineStart)
//                {
//                    // If so, copy it to the display buffer
//                    Serial.print("len: ");Serial.print(len); Serial.print("  -  lineStart: "); Serial.print(lineStart);
//                    charsToCopy = len - lineStart;
//                    Serial.print("  -  charsToCopy: "); Serial.print(charsToCopy);
//                    copyText(_displayLines[line++], &userMessage[lineStart], charsToCopy);
//                }
//            }
//        }
//    }
//    else
//    {
//        // Copy the whole line - there are no line terminator chars
//        strcpy(_displayLines[0], userMessage);
//    }
//
//    Serial.print("charPairPos: ");Serial.println(charPairPos);
//
//    if (charPairPos > -1)
//    {
//        _msgOrigin[20] = 0;
//
//        strcpy(_displayLines[3], _msgOrigin);
//    }
//
//
//    // Ensure the display is blank before displaying
//    blankDisplay();
//
//    for (int i = 0; i < DISPLAY_LINE_COUNT; i++)
//    {
//        lcd.setCursor(0, i); //Start at character 4 on line 0
//        lcd.print(_displayLines[i]);
//        delay(50);
//        
//        // For testing, display the message on the serial monitor
//        Serial.println(_displayLines[i]);
//
//        //printf("%s\n", _displayLines[i]);
//    }
//}


void setup()
{
    Serial.begin(RADIO_SERIAL_BAUD_RATE);
    Serial1.begin(RADIO_SERIAL_BAUD_RATE);

    Serial.println("Starting...");

    lcd.begin(20, 4);         // initialize the lcd for 20 chars 4 lines, turn on backlight

    displayUserMessage(_userMessage);

    // TEST CODE
    //for (int i = 0; i<DISPLAY_LINE_TEST_COUNT; i++)
    //{
    //    displayText(_userMessage[i]);

    //    delay(2000);
    //}
}


void loop()
{
    while ((Serial.available() > 0) || Serial1.available() > 0) {
        
        char incomingByte;

        if (Serial1.available()) {
            // Read a byte from the serial port 1
            incomingByte = static_cast<char>(Serial1.read());
        } else {
            // Read a byte from the serial port
            incomingByte = static_cast<char>(Serial.read());
        }
        //Serial.print("'");

        // User Message
        /*if (incomingByte == USERMSG_INDICATOR_CHAR) {
            _userMsgInCharCount = RADIO_MSG_OFFSET_INIT;
        }*/

        Serial.print(incomingByte);

        // Count the characters
        _userMsgInCharCount++;

        // Reset if the max character count is reached
        if (_userMsgInCharCount >= RADIO_MSG_MAX_OFFSET) {
            _userMsgInCharCount = RADIO_MSG_OFFSET_INIT;
        }

        // add the incoming character to the user message buffer
        _userMessageIn[_userMsgInCharCount] = incomingByte;

        // Look for the end of message <EOM> character
        if ((incomingByte == '\n') || (incomingByte == '|')) {

            // if <EOM>, close off the message with zero termination
            _userMessageIn[_userMsgInCharCount] = 0;

            // reset the count for the next message
            _userMsgInCharCount = RADIO_MSG_OFFSET_INIT;

            Serial.println(_userMessageIn);

            // display the message
            displayUserMessage(_userMessageIn);
        }
    }

    delay(10);
}



