#include <stdint.h>
#include <Adafruit_BME280/Adafruit_BME280.h> 
#include <Adafruit_BNO055/Adafruit_BNO055.h>
#include <Adafruit_BNO055/utility/imumaths.h>
#include <SPI.h>

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (1==0)
#endif


#define PIN_BATTERY_LEVEL_RAW_VALUE          A0   // Input: Analog pin for measuring the main battery voltage
#define PIN_SOUND_RAW_VALUE                  A1
#define PIN_CURRENT_RAW_VALUE                A2

#define PIN_CALIBRATION_LED                   7
#define PIN_CALIBRATION_SWITCH                8

#define SERIAL_PORT_BAUD                  38400

#define ADC_HIGH_VALUE                     1008   // Adjusted, high value for the ADC pins
#define BATTERY_VOLTAGE_FULL_MAIN           8.4   // Voltage level of a full battery (used to calc current voltage)


// Define hardware connections
#define PIN_GATE_IN                           2
#define IRQ_GATE_IN                           0
#define PIN_LED_OUT                          13
#define PIN_ANALOG_IN                        A1

#define SEALEVELPRESSURE_INHG                                          29.95

#define INHG_PER_HPA                                                 33.7685
#define SEALEVELPRESSURE_HPA          (SEALEVELPRESSURE_INHG * INHG_PER_HPA)
//#define SEALEVELPRESSURE_HPA                                     (1017.94)

// if the _startupCounter reaches this number, the mag calibration will be skipped 
#define STARTUP_COUNTER_LIMIT               600  //  divide by 10 for # of seconds 

Adafruit_BME280 atmSensor;
Adafruit_BNO055 bno = Adafruit_BNO055();


int _smoothed = 0;    // smoothed audio level value
int _alpha = 4;       // the number of past samples to average by

int _reading;           // the current reading from the input pin
int _previous = LOW;    // the previous reading from the input pin

int _buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin


                       // the follow variables are long's because the time, measured in miliseconds,
                       // will quickly become a bigger number than can be stored in an int.
long _time = 0;         // the last time the output pin was toggled
long _debounce = 200;   // the debounce time, increase if the output flickers



char buffer[192];

enum SoundLevel
{
    QUIET,
    MODERATE,
    LOUD,
    NOISY

};


struct sound_values_t {
    SoundLevel soundLevel;
    int rawValue;
};

struct current_values_t {
    int rawValue;
    double calculatedCurrent;
    double smoothedValue;
};

struct voltage_values_t {
    int rawValue;
    int smoothedValue;
    double calculatedSmoothedVoltage;
    double calculatedVoltage;
};

struct voltage_values_t _voltageValues;
struct current_values_t _currentValues;
struct sound_values_t _soundValues;

uint8_t _calibrationBlinkCounter = 0;
uint8_t _calibrationBlinkValue = LOW;
uint8_t _doCalibration = TRUE;

int _startupCounter = 0;

// Smooth out an analog reading:
int smoothValue(int rawValue, int currentSmoothedValue, int alpha)
{
    return currentSmoothedValue + (rawValue - currentSmoothedValue) / alpha;
}


struct sound_values_t getSoundLevel(sound_values_t soundValues)
{
    // Check the envelope input
    soundValues.rawValue = analogRead(PIN_SOUND_RAW_VALUE);
    
    //Serial.print("Sound Raw Value: ");Serial.print(soundValues.rawValue);Serial.print("  ");
    //
    //Serial.print(" Sound Level: ");

    if (soundValues.rawValue <= 10)
    {
        //Serial.println("Quiet.");
        soundValues.soundLevel = SoundLevel::QUIET;
    }
    else if ((soundValues.rawValue > 10) && (soundValues.rawValue <= 30))
    {
        //Serial.println("Moderate.");
        soundValues.soundLevel = SoundLevel::MODERATE;
    }
    else if ((soundValues.rawValue > 30) && (soundValues.rawValue <= 100))
    {
        //Serial.println("Loud.");
        soundValues.soundLevel = SoundLevel::NOISY;
    }
    else
    {
        //Serial.println("Noisy.");
        soundValues.soundLevel = SoundLevel::LOUD;
    }

    return soundValues;
}


struct voltage_values_t calcBatteryLevel(struct voltage_values_t values) {

    //values.rawValue = readMeanAverageValue(PIN_BATTERY_LEVEL_RAW_VALUE, 20);
    values.rawValue = analogRead(PIN_BATTERY_LEVEL_RAW_VALUE);

    values.smoothedValue = smoothValue(values.rawValue, values.smoothedValue, 4);

    double ratioSmooth = static_cast<double>(values.smoothedValue) / static_cast<double>(ADC_HIGH_VALUE);
    double batValSmooth = ratioSmooth * BATTERY_VOLTAGE_FULL_MAIN;

    values.calculatedSmoothedVoltage = (batValSmooth > BATTERY_VOLTAGE_FULL_MAIN) ? BATTERY_VOLTAGE_FULL_MAIN : batValSmooth;


    double ratio = static_cast<double>(values.rawValue) / static_cast<double>(ADC_HIGH_VALUE);
    double batVal = ratio * BATTERY_VOLTAGE_FULL_MAIN;

    values.calculatedVoltage = batVal; //(batVal > BATTERY_VOLTAGE_FULL_MAIN) ? BATTERY_VOLTAGE_FULL_MAIN : batVal;

    return values;
}


struct current_values_t getCurrentValues(current_values_t values)
{
    values.rawValue = analogRead(PIN_CURRENT_RAW_VALUE);

    values.smoothedValue = smoothValue(values.rawValue, values.smoothedValue, 4);

    // Analog value 475 = .435A
    values.calculatedCurrent = values.smoothedValue * 0.916;

    return values;
}




void mag_calibration()
{
    /* Display calibration status for each sensor. */
    uint8_t system, gyro, accel, mag = 0;

    bno.getCalibration(&system, &gyro, &accel, &mag);

    if (mag < 3) {
        _calibrationBlinkCounter++;

        if (_calibrationBlinkCounter >= 5) {
            _calibrationBlinkCounter = 0;

            if (_calibrationBlinkValue == HIGH) {
                _calibrationBlinkValue = LOW;
            }
            else {
                _calibrationBlinkValue = HIGH;
            }

            digitalWrite(PIN_CALIBRATION_LED, _calibrationBlinkValue);
        }
    }
    else {
        
        //Serial.println("Calibration status: Fully Calibrated");
        
        _doCalibration = FALSE;

        digitalWrite(PIN_CALIBRATION_LED, HIGH);
    }
}



void blinkFast(uint8_t rate)
{
    int blinkRate = 1000 / (rate * 2);

    while (TRUE)
    {
        digitalWrite(PIN_CALIBRATION_LED, HIGH);

        delay(blinkRate);

        digitalWrite(PIN_CALIBRATION_LED, LOW);

        delay(blinkRate);
    }
}


void setup()
{
    pinMode(PIN_CALIBRATION_LED, OUTPUT);
    pinMode(PIN_CALIBRATION_SWITCH, INPUT);

    Serial.begin(SERIAL_PORT_BAUD);

    delay(2000);

    Serial.println("Starting...");

    if (!atmSensor.begin())
    {
        Serial.println("Failed Atmospheric Sensor");
        //blinkFast(2);
    }

    /* Initialise the sensor */
    if (!bno.begin())
    {
        Serial.println("Failed Accelerometer");
        //blinkFast(2);
    }

    delay(1000);

    bno.setExtCrystalUse(true);

    _voltageValues.rawValue = 1008;
    _voltageValues.smoothedValue = 1008;
    _voltageValues.calculatedSmoothedVoltage = 8.4;

    buffer[0] = 0;

    Serial.println("Setup Done!");

}


/*
atmTemp
atmHumidity
atmPressure
imuLinAccelX
imuLinAccelY
imuLinAccelZ
imuHeading
imuPitch
imuRoll
soundAmp
devVoltage
devCurrent
*/

/*
//  1: atmTemp
//  2: atmHumidity
//  3: atmPressure
//  4: imuLinAccelX
//  5: imuLinAccelY
//  6: imuLinAccelZ
//  7: imuHeading
//  8: imuPitch
//  9: imuRoll
// 10: soundAmp
// 11: devVoltage
// 12: devCurrent
// 13: Accelx
// 14: Accely
// 15: Accelz
// 16: Magx
// 17: Magy
// 18: Magz
// 19: Gyrox
// 20: Gyroy
// 21: Gyroz
// 22: Gravityx
// 23: Gravityy
// 24: Gravityz
// 25: sensorAltitudeMeters

*/




int concatNumber(char* dest, int position, double value, uint8_t precision, char delimiter)
{
    //dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);

    dtostrf(value, 3, precision, &dest[position]);

    int len = strlen(dest);

    if (delimiter)
    {
        dest[len++] = delimiter;
        dest[len] = '\0';
    }

    return len;
}

int concatNumber(char* dest, int position, int value, char delimiter)
{
    itoa(value, &dest[position], 10);
    
    int len = strlen(dest);

    if (delimiter)
    {
        dest[len++] = delimiter;
        dest[len] = '\0';
    }

    return len;
}

unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


void loop()
{
    if (_doCalibration) {
        mag_calibration();

        delay(250);

        _startupCounter++;

        if (_startupCounter > STARTUP_COUNTER_LIMIT)
        {
            _doCalibration = false;

            digitalWrite(PIN_CALIBRATION_LED, HIGH);
        }

        return;
    }

    _reading = digitalRead(PIN_CALIBRATION_SWITCH);


    // If the switch changed, due to noise or pressing:
    if (_reading != _previous) {
        // reset the debouncing timer
        _time = millis();
    }


    if ((millis() - _time) > debounceDelay) {
        // whatever the reading is at, it's been there for longer
        // than the debounce delay, so take it as the actual current state:

        // if the button state has changed:
        if (_reading != _buttonState) {
            _buttonState = _reading;

            // only toggle the LED if the new button state is HIGH
            if (_buttonState == LOW) {
                _doCalibration = true;
                _startupCounter = 0;
            }
        }
    }


    _previous = _reading;

    //Serial.println("Hello");
    //  1: atmTemp
    int len = concatNumber(buffer, 0, atmSensor.readTemperature(), 2, ',');
    //Serial.print(atmSensor.readTemperature(), 2);
    //Serial.print(",");
    //  2: atmHumidity
    len = concatNumber(buffer, len, atmSensor.readHumidity(), 3, ',');
    //Serial.print(atmSensor.readHumidity(), 3);
    //Serial.print(",");
    //  3: atmPressure
    len = concatNumber(buffer, len, atmSensor.readPressure() / 100.0, 2, ',');
    //Serial.print(atmSensor.readPressure() / 100.0, 2);
    //Serial.print(",");
    
    //Possible vector values can be:
    //- VECTOR_ACCELEROMETER - m/s^2
    //- VECTOR_MAGNETOMETER  - uT
    //- VECTOR_GYROSCOPE     - rad/s
    //- VECTOR_EULER         - degrees
    //- VECTOR_LINEARACCEL   - m/s^2
    //- VECTOR_GRAVITY       - m/s^2

    imu::Vector<3> linAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    //  4: imuLinAccelX
    len = concatNumber(buffer, len, linAccel.x(), 2, ',');
    //Serial.print(linAccel.x(), 4);
    //Serial.print(",");
    //  5: imuLinAccelY
    len = concatNumber(buffer, len, linAccel.y(), 2, ',');
    //Serial.print(linAccel.y(), 4);
    //Serial.print(",");
    //  6: imuLinAccelZ
    len = concatNumber(buffer, len, linAccel.z(), 2, ',');
    //Serial.print(linAccel.z(), 4);
    //Serial.print(",");

    imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    //  7: imuHeading
    len = concatNumber(buffer, len, euler.x(), 4, ',');
    //Serial.print(euler.x(), 4);
    //Serial.print(",");
    //  8: imuPitch
    len = concatNumber(buffer, len, euler.y(), 4, ',');
    //Serial.print(euler.y(), 4);
    //Serial.print(",");
    //  9: imuRoll
    len = concatNumber(buffer, len, euler.z(), 4, ',');
    //Serial.print(euler.z(), 4);
    //Serial.print(",");


    // Check the envelope input
    _soundValues = getSoundLevel(_soundValues);
    _smoothed = smoothValue(_soundValues.rawValue, _smoothed, _alpha);

    // 10: soundAmp
    len = concatNumber(buffer, len, _soundValues.rawValue, ',');
    //Serial.print(_soundValues.rawValue, 1);
    //Serial.print(",");

    _voltageValues = calcBatteryLevel(_voltageValues);
    _currentValues = getCurrentValues(_currentValues);

    // 11: devVoltage
    len = concatNumber(buffer, len, _voltageValues.calculatedSmoothedVoltage, 2, ',');
    //Serial.print(_voltageValues.calculatedSmoothedVoltage, 2);
    //Serial.print(",");

    // 12: devCurrent
    //.calculatedCurrent
    len = concatNumber(buffer, len, static_cast<int>(_currentValues.calculatedCurrent), ',');
    //Serial.print(_currentValues.calculatedCurrent, 0);
    //Serial.print(",");

    imu::Vector<3> accelMagGyroGrav = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
    // 13: x
    len = concatNumber(buffer, len, accelMagGyroGrav.x(), 4, ',');
    //Serial.print(accelMagGyroGrav.x(), 4);
    //Serial.print(",");
    // 14: y
    len = concatNumber(buffer, len, accelMagGyroGrav.y(), 4, ',');
    //Serial.print(accelMagGyroGrav.y(), 4);
    //Serial.print(",");
    // 15: z
    len = concatNumber(buffer, len, accelMagGyroGrav.z(), 4, ',');
    //Serial.print(accelMagGyroGrav.z(), 4);
    //Serial.print(",");


    accelMagGyroGrav = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
    // 16: x
    len = concatNumber(buffer, len, accelMagGyroGrav.x(), 4, ',');
    //Serial.print(accelMagGyroGrav.x(), 4);
    //Serial.print(",");
    // 17: y
    len = concatNumber(buffer, len, accelMagGyroGrav.y(), 4, ',');
    //Serial.print(accelMagGyroGrav.y(), 4);
    //Serial.print(",");
    // 18: z
    len = concatNumber(buffer, len, accelMagGyroGrav.z(), 4, ',');
    //Serial.print(accelMagGyroGrav.z(), 4);
    //Serial.print(",");


    accelMagGyroGrav = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
    // 19: x
    len = concatNumber(buffer, len, accelMagGyroGrav.x(), 4, ',');
    //Serial.print(accelMagGyroGrav.x(), 4);
    //Serial.print(",");
    // 20: y
    len = concatNumber(buffer, len, accelMagGyroGrav.y(), 4, ',');
    //Serial.print(accelMagGyroGrav.y(), 4);
    //Serial.print(",");
    // 21: z
    len = concatNumber(buffer, len, accelMagGyroGrav.z(), 4, ',');
    //Serial.print(accelMagGyroGrav.z(), 4);
    //Serial.print(",");


    accelMagGyroGrav = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
    // 22: x
    len = concatNumber(buffer, len, accelMagGyroGrav.x(), 4, ',');
    //Serial.print(accelMagGyroGrav.x(), 4);
    //Serial.print(",");
    // 23: y
    len = concatNumber(buffer, len, accelMagGyroGrav.y(), 4, ',');
    //Serial.print(accelMagGyroGrav.y(), 4);
    //Serial.print(",");
    // 24: z
    len = concatNumber(buffer, len, accelMagGyroGrav.z(), 4, ',');
    //Serial.print(accelMagGyroGrav.z(), 4);
    //Serial.print(",");

    // 25: sensorAltitudeMeters
    //Serial.print(atmSensor.readAltitude(SEALEVELPRESSURE_HPA), 1);
    len = concatNumber(buffer, len, atmSensor.readAltitude(SEALEVELPRESSURE_HPA), 1, '\n');

    Serial.print(buffer);
    //Serial.println("gabye");

    delay(500);

}
