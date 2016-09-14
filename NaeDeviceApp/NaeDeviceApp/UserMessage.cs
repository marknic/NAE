using System;

namespace NaeDeviceApp
{
    public static class UserMessage
    {
        private const int UserDisplayMaxLineChars = 20;
        
        
        /// <summary>
        /// Takes a user message from the radio/UDP receiver and formats it for the 
        /// Arduino/LCD display
        /// </summary>
        /// <param name="msg">string user message to format</param>
        /// <returns>user message for easy display through the Arduino</returns>
        public static string PrepUserMessage(string msg)
        {
            var stringOut = string.Empty;

            var output = msg.Trim();
            while (output.IndexOf("  ", StringComparison.Ordinal) > 0)
            {
                output = output.Replace("  ", " ");
            }

            var modified = true;

            while (modified)
            {
                output = CheckForLongStrings(output, out modified);
            }

            var stringSplit = output.Split(' ');

            for (var i = 0; i < stringSplit.Length; i++)
            {
                if (stringSplit[i].Length > UserDisplayMaxLineChars)
                {
                    stringSplit[i] = stringSplit[i].Substring(0, UserDisplayMaxLineChars);
                }

                stringOut += stringSplit[i];

                if (i < stringSplit.Length - 1)
                {
                    stringOut += " ";
                }
            }

            stringOut = FormatDisplayText(stringOut);

            return stringOut;
        }


        /// <summary>
        /// Splits long "words" 
        /// </summary>
        /// <param name="stringIn"></param>
        /// <param name="modified"></param>
        /// <returns></returns>
        private static string CheckForLongStrings(string stringIn, out bool modified)
        {
            modified = false;

            if (string.IsNullOrWhiteSpace(stringIn)) return string.Empty;

            var split = stringIn.Split(' ');
            var stringOut = string.Empty;

            foreach (var word in split)
            {
                var newWord = word;
                if (word.Length > UserDisplayMaxLineChars)
                {
                    newWord = word.Insert(UserDisplayMaxLineChars, " ");

                    modified = true;
                }

                stringOut += newWord + " ";
            }

            return stringOut.Trim();
        }


        /// <summary>
        /// Sets up the "lines" for the ePaper/LCD display using a limit of 
        /// UserDisplayMaxLineChars (the number of characters per line).
        /// Each line is separated with a '`' characters
        /// </summary>
        /// <param name="stringIn">line of text to format</param>
        /// <returns>Formatted line of text</returns>
        private static string FormatDisplayText(string stringIn)
        {
            if (string.IsNullOrWhiteSpace(stringIn)) return string.Empty;

            var split = stringIn.Split();

            var stringOut = string.Empty;

            var curLen = 0;

            foreach (var word in split)
            {
                if (curLen + word.Length > UserDisplayMaxLineChars)
                {
                    stringOut = stringOut.Trim();
                    stringOut += "`";
                    curLen = 0;
                }

                stringOut += word;

                stringOut += " ";

                curLen += word.Length + 1;
            }

            stringOut = stringOut.Trim();
            stringOut += '\n';  // Add end of message <EOM> character

            return stringOut;
        }


    }
}
