using System;
using System.Diagnostics;
using System.Linq;

namespace NaeDeviceApp
{
    public sealed class GpsData
    {
        public DateTimeTracker UtcDateTime { get; private set; }

        public double LatitudeDegrees { get; private set; }

        public double LongitudeDegrees { get; private set; }

        public double SpeedInKph { get; private set; }

        public double Direction { get; private set; }

        public int Fix { get; private set; }

        public int Satellites { get; private set; }

        public double Altitude { get; private set; }

        private string _sentenceBuffer = string.Empty;

        public GpsData()
        {
            UtcDateTime = new DateTimeTracker(2016, 1, 1, 0, 0, 0);
        }


        private static bool IsSentenceProperlyFormatted(string sentence)
        {
            if (string.IsNullOrWhiteSpace(sentence))
            {
                return false;
            }

            var pos = sentence.IndexOf('$');

            if (pos == -1)
            {
                return false;
            }

            if (pos > 0)
            {
                sentence = sentence.Remove(0, pos);
            }

            pos = sentence.IndexOf('*');

            if (pos == -1)
            {
                return false;
            }

            if (pos < sentence.Length - 3)
            {
                sentence = sentence.Remove(pos);
            }

            var calculatedCheckSum = GetChecksum(sentence.Substring(1, sentence.Length - 4));

            var sentenceInChecksum = sentence.Substring(sentence.Length - 2, 2);

            if (calculatedCheckSum != sentenceInChecksum)
            {
                Debug.WriteLine($"Bad checksum - In: {sentenceInChecksum} vs Calculated: {calculatedCheckSum} in sentence: '{sentence}'");
            }

            return calculatedCheckSum == sentenceInChecksum;
        }


        private static string GetChecksum(string sentence)
        {
            if (string.IsNullOrWhiteSpace(sentence))
            {
                return string.Empty;
            }

            var checksum = sentence.Aggregate(0, (current, t) => current ^ Convert.ToByte(t));

            // Return the checksum formatted as a two-character hexadecimal
            return checksum.ToString("X2");
        }


        public void ProcessData(string dataIn)
        {
            dataIn = dataIn.Replace("\r", "");

            if (string.IsNullOrWhiteSpace(dataIn))
            {
                return;
            }

            var eolPos = dataIn.IndexOf('\n');

            if (eolPos == -1)
            {
                _sentenceBuffer += dataIn;
            }
            else
            {
                while (eolPos > -1)
                {
                    _sentenceBuffer += dataIn.Substring(0, eolPos);

                    var asteriskPos = _sentenceBuffer.IndexOf('*');
                    var goodSentence = _sentenceBuffer.IndexOf('$') > -1 && asteriskPos > -1 && _sentenceBuffer.Length >= asteriskPos + 1;

                    if (IsSentenceProperlyFormatted(_sentenceBuffer) == false)
                    {
                        Debug.WriteLine($"GPS Sentence error: {_sentenceBuffer}'");
                    }
                    else
                    {
                        try
                        {
                            //Debug.WriteLine("Parsing the sentence...");
                            ParseData(_sentenceBuffer);
                        }
                        catch (Exception ex)
                        {
                            Debug.WriteLine($"error converting telemetry data: {ex.Message}");
                        }
                    }

                    _sentenceBuffer = string.Empty;

                    dataIn = dataIn.Remove(0, eolPos + 1);

                    eolPos = dataIn.IndexOf('\n');

                    if (eolPos == -1)
                    {
                        _sentenceBuffer = dataIn;
                    }
                }
            }
        }


        private static double ConvertToDegrees(string value, string direction)
        {
            var decimalPos = value.IndexOf('.') - 2;
            var positionInDegrees = 0.0;

            if (decimalPos <= 0) return positionInDegrees;

            var deg = value.Substring(0, decimalPos);
            var min = value.Substring(decimalPos);

            positionInDegrees = Convert.ToDouble(deg) + (Convert.ToDouble(min) / 60.0);

            direction = direction.ToUpper();

            if ((direction == "S") || (direction == "W"))
            {
                positionInDegrees *= -1;
            }

            return positionInDegrees;
        }


        private static double ConvertToKilometersPerHour(string value)
        {
            const double knotsToKmh = 1.8520;
            var speed = 0.0;

            if (string.IsNullOrWhiteSpace(value)) return speed;

            var knots = SafeConvertDouble(value);

            speed = knots * knotsToKmh;

            return speed;
        }


        // Calculates the checksum for a sentence
        private static bool ValidateChecksum(string sentence)
        {
            //Start with first Item
            int checksum = Convert.ToByte(sentence[sentence.IndexOf('$') + 1]);

            // Loop through all chars to get a checksum
            for (var i = sentence.IndexOf('$') + 2; i < sentence.IndexOf('*'); i++)
            {
                // No. XOR the checksum with this character's value
                checksum ^= Convert.ToByte(sentence[i]);
            }

            var checkSumString = checksum.ToString("X2");

            // Return the checksum formatted as a two-character hexadecimal
            return checkSumString == sentence.Substring(sentence.Length - 2);
        }


        private void UpdateDateTime(string dateVal, string timeVal)
        {
            var hour = 0;
            var minute = 0;
            var seconds = 0;
            var day = 0;
            var month = 0;
            var year = 0;

            if (!string.IsNullOrWhiteSpace(timeVal) && (timeVal.Length >= 6))
            {
                try
                {
                    var time = SafeConvertInt(timeVal.Substring(0, 6));

                    hour = time / 10000;
                    minute = (time % 10000) / 100;
                    seconds = (time % 100);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Exception in UpdateDateTime: {0}", ex.Message);
                }
            }

            if (!string.IsNullOrWhiteSpace(dateVal) && (dateVal.Length == 6))
            {
                var dateNumber = SafeConvertInt(dateVal);

                day = dateNumber / 10000;
                month = (dateNumber % 10000) / 100;
                year = (dateNumber % 100);

                if (year >= 80)
                {
                    year += 1900;
                }
                else
                {
                    year += 2000;
                }
            }

            if ((year >= 0) && (month != 0) && (day != 0))
            {
                UtcDateTime = new DateTimeTracker(year, month, day, hour, minute, seconds);
            }
        }

        private void UpdateTime(string value)
        {
            if (string.IsNullOrWhiteSpace(value)) return;

            if (value.Length != 6) return;

            try
            {
                var time = SafeConvertInt(value);

                var hour = time / 10000;
                var minute = (time % 10000) / 100;
                var seconds = (time % 100);

                UtcDateTime = new DateTimeTracker(UtcDateTime.Year, UtcDateTime.Month, UtcDateTime.Day, hour,
                    minute, seconds);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception in UpdateTime: {0}", ex.Message);
            }
        }


        private static double SafeConvertDouble(string value)
        {
            var convertedValue = 0.0;

            if (string.IsNullOrWhiteSpace(value)) return convertedValue;

            try
            {
                convertedValue = Convert.ToDouble(value);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception in SafeConvertDouble: {0}", ex.Message);
            }

            return convertedValue;
        }


        private static int SafeConvertInt(string value)
        {
            var convertedValue = 0;

            if (string.IsNullOrWhiteSpace(value)) return convertedValue;

            try
            {
                convertedValue = Convert.ToInt32(value);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception in SafeConvertInt: {0}", ex.Message);
            }

            return convertedValue;
        }


        private void ParseData(string sentence)
        {
            if (string.IsNullOrWhiteSpace(sentence)) return;
            if (!ValidateChecksum(sentence)) return;

            var parts = sentence.Split(',');

            if (sentence.StartsWith("$GPRMC") || sentence.StartsWith("$GNRMC"))
            {
                UpdateDateTime(parts[9], parts[1]);

                LatitudeDegrees = ConvertToDegrees(parts[3], parts[4]);

                LongitudeDegrees = ConvertToDegrees(parts[5], parts[6]);

                SpeedInKph = ConvertToKilometersPerHour(parts[7]);

                Direction = SafeConvertDouble(parts[8]);
            }
            else if (sentence.StartsWith("$GPGGA") || sentence.StartsWith("$GNGGA"))
            {
                UpdateTime(parts[1]);

                if (parts[2].Length >= 6)
                {
                    LatitudeDegrees = ConvertToDegrees(parts[2], parts[3]);
                }

                if (parts[4].Length >= 6)
                {
                    LongitudeDegrees = ConvertToDegrees(parts[4], parts[5]);
                }

                Fix = SafeConvertInt(parts[6]); //((parts[6] == "1") || (parts[6] == "2"));

                Satellites = SafeConvertInt(parts[7]);

                Altitude = SafeConvertDouble(parts[9]);

            }
            else if (sentence.StartsWith("$GPGLL") || sentence.StartsWith("$GNGLL"))
            {
                UpdateTime(parts[5]);

                if (parts[2].Length >= 6)
                {
                    LatitudeDegrees = ConvertToDegrees(parts[2], parts[3]);
                }

                if (parts[4].Length >= 6)
                {
                    LongitudeDegrees = ConvertToDegrees(parts[4], parts[5]);
                }
            }
        }
    }
}
