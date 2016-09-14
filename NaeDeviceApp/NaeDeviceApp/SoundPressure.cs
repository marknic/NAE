using System;
using System.Diagnostics;

namespace NaeDeviceApp
{
    public static class SoundPressure
    {
        /*
         9	58
        12	77
        14	80
        18	85
        24	86
        28	88
        35	90
        56	93
        60	95
        93	100
        100	102
        149	103
        180	105
        200	107
        275	109
        340	111
        375	113
        419	116
        440	120
        */

        private static readonly double[] BoardValues = { 9, 12, 14, 18, 24, 28, 35, 56, 60, 93, 100, 149, 180, 200, 275, 340, 375, 419, 440, 460 };

        private static readonly double[] DecibelValues = { 58, 77, 80, 85, 86, 88, 90, 93, 95, 100, 102, 103, 105, 107, 109, 111, 113, 116, 120, 125 };

        private static readonly double _maxValue;
        private static readonly double _minValue;

        static SoundPressure()
        {
            _maxValue = BoardValues[BoardValues.Length - 1];
            _minValue = BoardValues[0];
        }

        public static double CalculateDecibelLevel(double measuredValue)
        {
            if (measuredValue > _maxValue)
            {
                Debug.WriteLine($"CalculateDecibelLevel: measuredValue = {measuredValue} - returning 130.0");
                return 130.0;
            }

            if (measuredValue <= _minValue)
            {
                return DecibelValues[0];
            }

            try
            {
                for (var i = 0; i < BoardValues.Length; i++)
                {
                    if (measuredValue < BoardValues[i])
                    {
                        var lowerVal = BoardValues[i - 1];
                        var distance = BoardValues[i] - lowerVal;
                        var position = measuredValue - lowerVal;

                        var percentage = position/distance;

                        var decibels = (percentage*(DecibelValues[i] - DecibelValues[i - 1])) + DecibelValues[i - 1];

                        return decibels;
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"CalculateDecibelLevel: Exception = {ex.Message}");
            }

            Debug.WriteLine($"CalculateDecibelLevel: measuredValue = {measuredValue} - returning 0.0");

            return 0.0;
        }
    }
}
