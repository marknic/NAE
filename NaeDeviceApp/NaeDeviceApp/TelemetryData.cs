using System;
using System.Diagnostics;

namespace NaeDeviceApp
{
    public static class TelemetryData
    {
        public static double AtmTemp { get; private set; }
        public static double AtmHumidity { get; private set; }
        public static double AtmPressure { get; private set; }
        public static double ImuLinAccelX { get; private set; }
        public static double ImuLinAccelY { get; private set; }
        public static double ImuLinAccelZ { get; private set; }
        public static double ImuHeading { get; private set; }
        public static double ImuPitch { get; private set; }
        public static double ImuRoll { get; private set; }
        public static double SoundAmp { get; private set; }
        public static double DevVoltage { get; private set; }
        public static int DevCurrent { get; private set; }
        public static double Accelx { get; private set; }
        public static double Accely { get; private set; }
        public static double Accelz { get; private set; }
        public static double Magx { get; private set; }
        public static double Magy { get; private set; }
        public static double Magz { get; private set; }
        public static double Gyrox { get; private set; }
        public static double Gyroy { get; private set; }
        public static double Gyroz { get; private set; }
        public static double Gravityx { get; private set; }
        public static double Gravityy { get; private set; }
        public static double Gravityz { get; private set; }
        public static double SensorAltitudeMeters { get; private set; }

        private static string _sentence = string.Empty;

        /// <summary>
        /// Ingests strings of characters and extracts the telemetry values
        /// Each line is delimited by a '\n' newline character
        /// Each telemetry value is separated with a ',' comma 
        /// </summary>
        /// <param name="dataIn">string of telemetry data</param>
        public static void ProcessData(string dataIn)
        {


            if (string.IsNullOrWhiteSpace(dataIn))
            {
                return;
            }

            var eolPos = dataIn.IndexOf('\n');
            
            if (eolPos == -1)
            {
                _sentence += dataIn;
            }
            else
            {

                while (eolPos > -1)
                {
                    _sentence += dataIn.Substring(0, eolPos);

                    var dataSplit = _sentence.Split(',');

                    if (dataSplit.Length != 25)
                    {
                        Debug.WriteLine($"Error only {dataSplit.Length} values. Data: '{dataIn}'");

                    }
                    else
                    {
                        try
                        {
                            // Break up all of the data
                            AtmTemp = Convert.ToDouble(dataSplit[0]);
                            AtmHumidity = Convert.ToDouble(dataSplit[1]);
                            AtmPressure = Convert.ToDouble(dataSplit[2]);
                            ImuLinAccelX = Convert.ToDouble(dataSplit[3]);
                            ImuLinAccelY = Convert.ToDouble(dataSplit[4]);
                            ImuLinAccelZ = Convert.ToDouble(dataSplit[5]);
                            ImuHeading = Convert.ToDouble(dataSplit[6]);
                            ImuPitch = Convert.ToDouble(dataSplit[7]);
                            ImuRoll = Convert.ToDouble(dataSplit[8]);
                            SoundAmp = Convert.ToDouble(dataSplit[9]);
                            DevVoltage = Convert.ToDouble(dataSplit[10]);
                            DevCurrent = Convert.ToInt32(dataSplit[11]);
                            Accelx = Convert.ToDouble(dataSplit[12]);
                            Accely = Convert.ToDouble(dataSplit[13]);
                            Accelz = Convert.ToDouble(dataSplit[14]);
                            Magx = Convert.ToDouble(dataSplit[15]);
                            Magy = Convert.ToDouble(dataSplit[16]);
                            Magz = Convert.ToDouble(dataSplit[17]);
                            Gyrox = Convert.ToDouble(dataSplit[18]);
                            Gyroy = Convert.ToDouble(dataSplit[19]);
                            Gyroz = Convert.ToDouble(dataSplit[20]);
                            Gravityx = Convert.ToDouble(dataSplit[21]);
                            Gravityy = Convert.ToDouble(dataSplit[22]);
                            Gravityz = Convert.ToDouble(dataSplit[23]);
                            SensorAltitudeMeters = Convert.ToDouble(dataSplit[24]);
                        }
                        catch (Exception ex)
                        {
                            Debug.WriteLine($"error converting telemetry data: {ex.Message}");
                        }
                    }

                    _sentence = string.Empty;

                    dataIn = dataIn.Remove(0, eolPos + 1);

                    eolPos = dataIn.IndexOf('\n');

                    if (eolPos == -1)
                    {
                        _sentence = dataIn;
                    }
                }
            }
        }
    }
}
