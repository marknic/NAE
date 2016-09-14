using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Controls;

namespace NaeDeviceApp
{
    public partial class MainPage : Page
    {
        public const string FEATURE_NAME = "SerialArduino";
    }

    public class Scenario
    {
        public string Title { get; set; }
        public Type ClassType { get; set; }
    }
}
