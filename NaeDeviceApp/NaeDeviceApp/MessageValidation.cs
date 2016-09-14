using System;
using System.Linq;
using System.Text;

namespace NaeDeviceApp
{
    public static class MessageValidation
    {
        public static string ValidateMessage(string messageIn)
        {
            if ((string.IsNullOrWhiteSpace(messageIn)) || (messageIn.Length <= 4)) return null;

            var message = messageIn.Substring(0, messageIn.Length - 4);

            var msgCheckVal = (byte)Encoding.ASCII.GetBytes(message).Sum(x => (int)x);

            var checkChars = messageIn.Substring(messageIn.Length - 2, 2);

            var calculatedCheckValue = Convert.ToInt32(checkChars, 16);

            return msgCheckVal == calculatedCheckValue ? message : null;
        }


        public static string GenerateStringWithCheckValue(string message)
        {
            if (string.IsNullOrWhiteSpace(message)) return null;

            var checkVal = (byte)Encoding.ASCII.GetBytes(message).Sum(x => (int)x);

            var newMsg = string.Format("{0},*{1:X2}", message, checkVal);

            return newMsg;
        }

    }
}
