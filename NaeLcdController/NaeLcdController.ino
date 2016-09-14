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

#define DISPLAY_LINE_LENGTH         21
#define DISPLAY_LINE_COUNT           4
#define DISPLAY_MAX_LENGTH          20

#define DISPLAY_LINE_TEST_COUNT      7


char _userMessageIn[RADIO_MSG_MAX_SIZE];

uint16_t _userMsgInCharCount = RADIO_MSG_OFFSET_INIT;
char _msgOrigin[64];

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
    if (stringToSearch == NULL) return 0;

    if (findThisChar == 0) return 0;

    if (startPos < 0) return 0;

    int len = strlen(stringToSearch);

    if (startPos >= len) return 0;

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
    _msgOrigin[0] = 32;

    int len = strlen(stringToSearch);

    for (int i = len-1; i>=0; i--)
    {
        if (i>0 && (stringToSearch[i] == findThisChar) && stringToSearch[i-1] == findThisChar)
        {
            stringToSearch[i - 1] = '\0';
            
            if (len - (i + 1) <= 20)
            {
                strcpy(&_msgOrigin[1], &stringToSearch[i + 1]);

                Serial.print("Origin Extracted: "); Serial.println(_msgOrigin);
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


char userMessage[DISPLAY_LINE_TEST_COUNT][90] = {
    "What the hell is`going on here``America",
    "AAAAAAAAAAAAAAAAAAAA`AAAAAAAAAAAAAAAAAAAA`AAAAAAAAAAAAAAAAAA!``North Korea",
    "I`aasdfasdfasdfasdfasd`asdf`wwwasdfasdfasdfasdft``Costa Rica",
    "This is some text to`display in the user`message display``South Africa",
    "What's the problem?``Scotland",
    "hello?`Is this thing on?``Trump Tower",
    "This is a bunch of`useless crap``Canada"
};


// Zero out the entire display buffer with nulls
void zeroDisplay()
{
    for (int i = 0; i<DISPLAY_LINE_COUNT; i++)
    {
        memset(&_displayLines[i], 0, DISPLAY_LINE_LENGTH);
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
void displayText(char* userMessage)
{
    int lineStart = 0;
    int line = 0;
    int charsToCopy;

    // zero out the display buffer
    zeroDisplay();

    int charPairPos = findCharPair(userMessage, MESSAGE_SEPARATOR_CHAR);

    // Look for the first line terminator character '`'
    int pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, 0);

    // Loop through all lines in the message
    if (pos > -1) {

        Serial.print("user msg: "); Serial.println(userMessage);
        // Loop
        while (pos > -1) {

            // How many chars in the line
            charsToCopy = pos - lineStart;

            // Copy that many characters to the next display line
            copyText(_displayLines[line++], &userMessage[lineStart], charsToCopy);
            
            // Move to the next display line
            lineStart = pos + 1;

            // Is there another line to display?
            pos = findChar(userMessage, MESSAGE_SEPARATOR_CHAR, lineStart);

            // If not, this is the last line so copy that to the display buffer
            if (pos == -1)
            {
                // Get the length of the line
                int len = strlen(userMessage);

                // Is there text to copy
                if (len > lineStart)
                {
                    // If so, copy it to the display buffer
                    charsToCopy = len - lineStart;
                    copyText(_displayLines[line++], &userMessage[lineStart], charsToCopy);
                }
            }
        }
    }
    else
    {
        // Copy the whole line - there are no line terminator chars
        strcpy(_displayLines[0], userMessage);
    }

    Serial.print("charPairPos: ");Serial.println(charPairPos);
    if (charPairPos > -1)
    {
        int originLen = strlen(_msgOrigin);



        if (originLen < 20) {
            Serial.println("executing copytext");
            copyText(&_displayLines[3][20 - originLen], _msgOrigin, strlen(_msgOrigin));
        }
    }


    // Ensure the display is blank before displaying
    blankDisplay();

    for (int i = 0; i < DISPLAY_LINE_COUNT; i++)
    {
        lcd.setCursor(0, i); //Start at character 4 on line 0
        lcd.print(_displayLines[i]);
        delay(50);
        
        // For testing, display the message on the serial monitor
        Serial.println(_displayLines[i]);

        //printf("%s\n", _displayLines[i]);
    }
}


void setup()
{
    Serial.begin(RADIO_SERIAL_BAUD_RATE);
    Serial1.begin(RADIO_SERIAL_BAUD_RATE);

    Serial.println("Starting...");

    lcd.begin(20, 4);         // initialize the lcd for 20 chars 4 lines, turn on backlight

    // TEST CODE
    //for (int i = 0; i<DISPLAY_LINE_TEST_COUNT; i++)
    //{
    //    displayText(userMessage[i]);

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

            // display the message
            displayText(_userMessageIn);
        }
    }

    delay(10);
}



