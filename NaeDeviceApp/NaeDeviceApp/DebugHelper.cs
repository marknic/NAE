using System.Diagnostics;

namespace NaeDeviceApp
{
    public static class DebugHelper
    {
        public static void DebugWriteLineEscChars(this string data, string header, string trailer)
        {
            var modifiedLine = data.Replace("\n", "\\n");
            modifiedLine = modifiedLine.Replace("\r", "\\r");
            modifiedLine = modifiedLine.Replace("\t", "\\t");

            Debug.WriteLine($"{header}{modifiedLine}{trailer}");
        }
    }
}
