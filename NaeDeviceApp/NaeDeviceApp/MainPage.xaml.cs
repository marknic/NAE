using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Devices.SerialCommunication;
using Windows.Networking;
using Windows.Networking.Sockets;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.System;
using Windows.System.Threading;
using Windows.UI.Xaml.Data;


namespace NaeDeviceApp
{

    //public class DeviceConfiguration
    //{
    //    public string telemetryFileName { get; set; }
    //    public string udpTargetIpAddress { get; set; }
    //    public int udpTargetPortNumber { get; set; }
    //}

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage
    {
        public static MainPage Current;

        //private const string ConfigurationFileName = "config.json";

        private const double MilesPerKilometer = 0.62137119;
        private const string UdpServerIpAddress = "10.0.0.6";
        private const string UdpServerPortAddressSend = "11000";
        private const string UdpServerPortAddressListen = "11001";

        private const uint GpsReadBufferLength = 2048;  //128;
        private const uint TelemetryReadBufferLength = 2048; //192;

        private const int SerialBaudRateGps = 9600;
        private const int SerialBaudRateTelemetry = 38400;
        private const int SerialBaudRateUserText = 38400;

        //private static DeviceConfiguration _deviceConfigurationData;

        private string _localTelemetryFile;

        private readonly GpsData _gpsData = new GpsData();

        private SerialDevice _gpsDevice;
        private SerialDevice _telemetryDevice;
        private SerialDevice _userTextDevice;

        private CancellationTokenSource _gpsReadCancellationTokenSource;
        private CancellationTokenSource _telemetryReadCancellationTokenSource;

        private readonly object _readCancelLock = new object();
        private readonly object _gpsReadCancelLock = new object();
        private readonly object _telemetryReadCancelLock = new object();

        private CancellationTokenSource _writeCancellationTokenSource;
        private readonly object _writeCancelLock = new object();

        private DatagramSocket _udpDatagramSocket;
        private DataWriter _socketDataWriter;
        private bool _socketErrorMessageDisplayed;


        public string DateTimeSystem => string.Format("{0}T{1}Z", DateTime.Now.ToString("yyyy-MM-dd"),
                 DateTime.Now.ToString("HH:mm:ss.fff"));

        private readonly Queue<string> _userMessageQueue;

        private double _decibels;

        private int _serialConnectionCount;


        // GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  
        // GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  GPS READ  

        private void ResetGpsReadCancellationTokenSource()
        {
            // Create a new cancellation token source so that can cancel all the tokens again
            _gpsReadCancellationTokenSource = new CancellationTokenSource();

            // Hook the cancellation callback (called whenever Task.cancel is called)
            _gpsReadCancellationTokenSource.Token.Register(NotifyGpsReadCancelingTask);
        }

        private static void NotifyGpsReadCancelingTask()
        {
            Debug.WriteLine("GPS Read Cancellation Called!");
        }

        private async Task ListenGps()
        {
            while (true)
            {
                await ReadGps();
            }
        }

        private async Task ReadGps()
        {
            DataReader dataReader = null;

            try
            {
                dataReader = new DataReader(_gpsDevice.InputStream)
                {
                    InputStreamOptions = InputStreamOptions.Partial
                };
                
                var gpsData = await ReadAsync(dataReader, _gpsReadCancellationTokenSource.Token, GpsReadBufferLength);

                _gpsData.ProcessData(gpsData);

            }
            catch (Exception exception)
            {
                Debug.WriteLine("ReadGps: " + exception.Message);
            }
            finally
            {
                dataReader?.DetachStream();
            }
        }


        private void CancelGpsReadTask()
        {
            lock (_gpsReadCancelLock)
            {
                if (_gpsReadCancellationTokenSource != null)
                {
                    if (!_gpsReadCancellationTokenSource.IsCancellationRequested)
                    {
                        _gpsReadCancellationTokenSource.Cancel();

                        // Existing IO already has a local copy of the old cancellation token so this reset won't affect it
                        ResetGpsReadCancellationTokenSource();
                    }
                }
            }
        }




        // TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  
        // TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  TELEMETRY READ  

        private void ResetTelemetryReadCancellationTokenSource()
        {
            // Create a new cancellation token source so that can cancel all the tokens again
            _telemetryReadCancellationTokenSource = new CancellationTokenSource();

            // Hook the cancellation callback (called whenever Task.cancel is called)
            _telemetryReadCancellationTokenSource.Token.Register(NotifyTelemetryReadCancelingTask);
        }


        private static void NotifyTelemetryReadCancelingTask()
        {
            Debug.WriteLine("Telemetry Read Cancellation Called!");
        }



        private async Task ReadTelemetry()
        {
            DataReader dataReader = null;

            try
            {
                dataReader = new DataReader(_telemetryDevice.InputStream)
                {
                    InputStreamOptions = InputStreamOptions.Partial
                };

                var telemetryData = await ReadAsync(dataReader, _telemetryReadCancellationTokenSource.Token, TelemetryReadBufferLength);

                //telemetryData.DebugWriteLineEscChars("Telem1>|", "|");

                TelemetryData.ProcessData(telemetryData);
            }
            catch (Exception exception)
            {
                Debug.WriteLine("ReadTelemetry: " + exception.Message);
            }
            finally
            {
                dataReader?.DetachStream();
            }
        }


        private async Task ListenTelemetry()
        {
            while (true)
            {
                await ReadTelemetry();
            }
        }

        private void CancelTelemetryReadTask()
        {
            lock (_telemetryReadCancelLock)
            {
                if (_telemetryReadCancellationTokenSource != null)
                {
                    if (!_telemetryReadCancellationTokenSource.IsCancellationRequested)
                    {
                        _telemetryReadCancellationTokenSource.Cancel();

                        // Existing IO already has a local copy of the old cancellation token so this reset won't affect it
                        ResetTelemetryReadCancellationTokenSource();
                    }
                }
            }
        }


        // USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  
        // USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  USER MSG WRITE  


        /// <summary>
        /// Checks the User Message Queue: if a message exists, dequeue it and
        /// and send it to the user message display
        /// </summary>
        /// <param name="timer">timer object</param>
        private void UserMessageTimerCallback(ThreadPoolTimer timer)
        {
            if (_userMessageQueue.Any())
            {
                var msg = _userMessageQueue.Dequeue();

                Debug.WriteLine($"User Message:'{msg}'");

                SendUserMessageToDisplay(msg);
            }
        }




        private void ResetWriteCancellationTokenSource()
        {
            // Create a new cancellation token source so that can cancel all the tokens again
            _writeCancellationTokenSource = new CancellationTokenSource();

            // Hook the cancellation callback (called whenever Task.cancel is called)
            _writeCancellationTokenSource.Token.Register(NotifyWriteCancelingTask);
        }


        private static void NotifyWriteCancelingTask()
        {
            Debug.WriteLine("Write Cancellation Called!");
        }







        private async void SendUserMessageToDisplay(string userMessage)
        {
            DataWriter userTextDataWriter = null;

            //var msgToDisplay = UserMessage.PrepUserMessage(userMessage);

            try
            {
                userTextDataWriter = new DataWriter(_userTextDevice.OutputStream);

                userTextDataWriter.WriteString(userMessage);

                await WriteAsync(userTextDataWriter, _writeCancellationTokenSource.Token);
            }
            catch (Exception exception)
            {
                Debug.WriteLine("SendUserMessageToDisplay: " + exception.Message);
            }
            finally
            {
                userTextDataWriter?.DetachStream();
            }
        }



        // UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  
        // UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  UDP TELEMETRY  


        private void SetupUdpConnection()
        {
            try
            { 
                var hostName = new HostName(UdpServerIpAddress);

                _udpDatagramSocket = new DatagramSocket();

                _udpDatagramSocket.MessageReceived += SocketMessageReceived;

                var taskUdp =
                    Task.Factory.StartNew(async () => await _udpDatagramSocket.ConnectAsync(hostName, UdpServerPortAddressSend));
                Task.WaitAll(taskUdp);

                // Create the UDP data writer
                _socketDataWriter = new DataWriter(_udpDatagramSocket.OutputStream);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Startup Error setting up UDP send (output) connection: {0}", ex.Message);
            }
        }



        //private double _maxSpeed = 0.0;

        private void PeriodicTelemetrySendTimerCallback(ThreadPoolTimer timer)
        {
            var speedInMph = _gpsData.SpeedInKph * MilesPerKilometer;

            //if (speedInMph > _maxSpeed + 100.0)
            //{
            //    _maxSpeed += 100;

            //    SendUserMessageToDisplay($"Speed just passed {_maxSpeed} Miles per Hour!");
            //}
            _decibels = SoundPressure.CalculateDecibelLevel(TelemetryData.SoundAmp);

            // timestamp, gpsLat, gpsLon, gpsAltitude, gpsSpeedKph, gpsSpeedMph, gpsDirection, gpsFix, gpsSatellites, atmTemp, atmHumidity, atmPressure, atmAltitude, imuLinAccelX, imuLinAccelY, imuLinAccelZ, _imuHeading, _imuPitch, _imuRoll, _soundAmp, _devVoltage, _devCurrent, checksum
            var telemetry =
                $"$:{DateTimeSystem},{_gpsData.LatitudeDegrees:0.0000000},{_gpsData.LongitudeDegrees:0.0000000},{_gpsData.Altitude:0.0},{_gpsData.SpeedInKph:0.0},{speedInMph:0.0},{_gpsData.Direction:0.0},{_gpsData.Fix:0},{_gpsData.Satellites:0},{TelemetryData.AtmTemp:0.0},{TelemetryData.AtmHumidity:0.0},{TelemetryData.AtmPressure:0.0},{TelemetryData.SensorAltitudeMeters:0.0},{TelemetryData.ImuLinAccelX:0.00},{TelemetryData.ImuLinAccelY:0.00},{TelemetryData.ImuLinAccelZ:0.00},{TelemetryData.ImuHeading:0.0},{TelemetryData.ImuPitch:0.0},{TelemetryData.ImuRoll:0.0},{_decibels:0.0},{TelemetryData.DevVoltage:0.0},{TelemetryData.DevCurrent:0}";

            //Current.Coordinates.Text = $"{_gpsData.LatitudeDegrees:0.0000000} / {_gpsData.LongitudeDegrees:0.0000000}";

            telemetry = MessageValidation.GenerateStringWithCheckValue(telemetry);

            Debug.WriteLine(telemetry);

            //try
            //{
            //    WriteLocalTelemetry(_localTelemetryFile, telemetry);
            //}
            //catch (Exception ex)
            //{
            //    Debug.Write($"Exception while attempting to write to telemetry file.");
            //}

            // Write the locally buffered data to the network.
            try
            {
                _socketDataWriter.WriteString(telemetry);

                var storeTask = Task.Run(async () => await _socketDataWriter.StoreAsync());
                storeTask.GetAwaiter().GetResult();

            }
            catch (Exception exception)
            {
                Debug.WriteLine("_socketDataWriter exception: " + exception.Message);
            }
        }


        private static string FormatUserMessage(string userMessage, int speed)
        {
            var trimmedMessage = userMessage.Remove(0, 3);

            while (trimmedMessage.Contains("  "))
            {
                trimmedMessage = trimmedMessage.Replace("  ", " ");
            }

            var origin = string.Empty;

            if (trimmedMessage.Contains('`'))
            {
                var delimiterPos = trimmedMessage.IndexOf('`');

                origin = trimmedMessage.Substring(delimiterPos + 1);

                trimmedMessage = trimmedMessage.Remove(delimiterPos);
            }

            var msgComponents = trimmedMessage.Split(' ');

            var formattedMsg = string.Empty;

            var tmpLen = 0;

            foreach (var msgComponent in msgComponents)
            {
                if (msgComponent.Length + tmpLen <= 20)
                {
                    formattedMsg = $"{formattedMsg} {msgComponent}";

                    tmpLen += msgComponent.Length + 1;
                }
                else
                {
                    formattedMsg = $"{formattedMsg}`{msgComponent}";

                    tmpLen = msgComponent.Length;
                }
            }

            formattedMsg = formattedMsg.Trim();

            if (origin.Length > 16)
            {
                origin = origin.Substring(0, 16);
            }

            var speedString = $"{speed}";
            var padding = "".PadLeft(20 - origin.Length - speedString.Length);

            formattedMsg = $"{formattedMsg}``{speedString}{padding}{origin}\n";

            return formattedMsg;
        }



        private async void SocketMessageReceived(DatagramSocket sender, DatagramSocketMessageReceivedEventArgs args)
        {
            try
            { 
                //Read the message that was received from the UDP echo server.
                var streamIn = args.GetDataStream().AsStreamForRead();

                var reader = new StreamReader(streamIn);

                var message = await reader.ReadLineAsync();

                var validMessage = MessageValidation.ValidateMessage(message);

                if (validMessage != null)
                {
                    var cmd = validMessage[1];

                    if (validMessage[0] == '{')
                    {

                        switch (cmd)
                        {
                            case 'X': // SHUTDOWN!!!
                                if (validMessage[3] == '!')
                                {
                                    ShutdownHelper(ShutdownKind.Shutdown);
                                }
                                break;

                            case 'R': // REBOOT!!!
                                if (validMessage[3] == '!')
                                {
                                    ShutdownHelper(ShutdownKind.Restart);
                                }
                                break;

                            case 'U': // User message
                                Debug.WriteLine($"----userMessage----");

                                var speedInMph = Convert.ToInt32(_gpsData.SpeedInKph * MilesPerKilometer);

                                validMessage = validMessage.Trim();

                                var userMessage = FormatUserMessage(validMessage, speedInMph);
                                
                                Debug.WriteLine($"userMessage: {userMessage}");

                                // >>>>>>>>>>>>>>>>>>  add message to text display queue  <<<<<<<<<<<<<<<<<<<<
                                _userMessageQueue.Enqueue(userMessage);

                                break;
                        }
                    }
                }

                Debug.WriteLine("Message Returned: " + message);
            }
            catch (Exception ex)
            {
                if (!_socketErrorMessageDisplayed)
                {
                    _socketErrorMessageDisplayed = true;
                    Debug.WriteLine("Error in SocketMessageReceived: {0}", ex.Message);
                }
            }
        }



        // COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  
        // COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  COMMON READ/WRITE  


        private async Task WriteLocalTelemetry(string filename, string content)
        {
            // saves the string 'content' to a file 'filename' in the app's local storage folder
            var fileBytes = System.Text.Encoding.UTF8.GetBytes(content.ToCharArray());

            // create a file with the given filename in the local folder; replace any existing file with the same name
            var file = await Windows.Storage.ApplicationData.Current.LocalFolder.CreateFileAsync(filename, CreationCollisionOption.OpenIfExists);

            // write the char array created from the content string into the file
            using (var stream = await file.OpenStreamForWriteAsync())
            {
                stream.Write(fileBytes, 0, fileBytes.Length);
            }
        }


        private async Task PeriodicGetSerialDataCallback()
        {
            if (_gpsDevice != null)
            {
                await ReadGps();
            }

            if (_telemetryDevice != null)
            {
                await ReadTelemetry();
            }
        }

        /// <summary>
        /// Write to the output stream using a task 
        /// </summary>
        /// <param name="dataWriterObject">writer object</param>
        /// <param name="cancellationToken">cancellation token</param>
        private async Task WriteAsync(IDataWriter dataWriterObject, CancellationToken cancellationToken)
        {
            Task<uint> storeAsyncTask;

            // Don't start any IO if we canceled the task
            lock (_writeCancelLock)
            {
                cancellationToken.ThrowIfCancellationRequested();

                // Cancellation Token will be used so we can stop the task operation explicitly
                // The completion function should still be called so that we can properly handle a canceled task
                storeAsyncTask = dataWriterObject.StoreAsync().AsTask(cancellationToken);
            }

            var bytesWritten = await storeAsyncTask;

            Debug.WriteLine("Write completed - {0} bytes written.", bytesWritten);
        }


        private void CancelWriteTask()
        {
            lock (_writeCancelLock)
            {
                if (_writeCancellationTokenSource != null)
                {
                    if (!_writeCancellationTokenSource.IsCancellationRequested)
                    {
                        _writeCancellationTokenSource.Cancel();

                        // Existing IO already has a local copy of the old cancellation token so this reset won't affect it
                        ResetWriteCancellationTokenSource();
                    }
                }
            }
        }


        /// <summary>
        /// Read from the input output stream using a task 
        /// </summary>
        /// <param name="dataReaderObject"></param>
        /// <param name="cancellationToken"></param>
        /// <param name="readBufferLength"></param>
        private static async Task<string> ReadAsync(IDataReader dataReaderObject, CancellationToken cancellationToken, uint readBufferLength)
        {
            var dataOut = string.Empty;
            var byteBuffer = new byte[readBufferLength];

            cancellationToken.ThrowIfCancellationRequested();

            // Cancellation Token will be used so we can stop the task operation explicitly
            // The completion function should still be called so that we can properly handle a canceled task
            var loadAsyncTask = dataReaderObject.LoadAsync(readBufferLength).AsTask(cancellationToken);


            var bytesRead = await loadAsyncTask;

            if (bytesRead > 0)
            {
                dataOut = dataReaderObject.ReadString(bytesRead);

                //dataReaderObject.ReadBytes(byteBuffer);

                //dataOut = Encoding.UTF8.GetString(byteBuffer, 0, byteBuffer.Length);
            }

            return dataOut;
        }


        //private async Task<string> ReadAsync(IDataReader dataReaderObject, CancellationToken cancellationToken, uint readBufferLength)
        //{
        //    Task<uint> loadAsyncTask;

        //    var dataOut = string.Empty;
        //    var byteBuffer = new byte[readBufferLength];

        //    // Don't start any IO if we canceled the task
        //    lock (_readCancelLock)
        //    {
        //        cancellationToken.ThrowIfCancellationRequested();

        //        // Cancellation Token will be used so we can stop the task operation explicitly
        //        // The completion function should still be called so that we can properly handle a canceled task
        //        loadAsyncTask = dataReaderObject.LoadAsync(readBufferLength).AsTask(cancellationToken);
        //    }

        //    var bytesRead = await loadAsyncTask;

        //    if (bytesRead > 0)
        //    {
        //        dataReaderObject.ReadBytes(byteBuffer);

        //        dataOut = Encoding.UTF8.GetString(byteBuffer, 0, byteBuffer.Length);
        //    }

        //    return dataOut;
        //}


        //private async Task PeriodicUpdateScreenTimerCallback(ThreadPoolTimer timer) //object o, object e)
        //{
        //    var speedInMph = _gpsData.SpeedInKph*MilesPerKilometer;
        //    var tempInF = TelemetryData.AtmTemp*1.8 + 32;

        //    // we have to update UI in UI thread only
        //    await CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
        //        {
        //            var speedInMph = _gpsData.SpeedInKph * MilesPerKilometer;
        //            var tempInF = TelemetryData.AtmTemp * 1.8 + 32;
        //            Coordinates.Text = $"{_gpsData.LatitudeDegrees:0.0000000} / {_gpsData.LongitudeDegrees:0.0000000}";
        //            Speed.Text = $"{speedInMph:0.0}";
        //            Direction.Text = $"{_gpsData.Direction:0.0}";
        //            Direction.Text = $"{TelemetryData.ImuHeading:0.0}";
        //            Acceleration.Text = $"{TelemetryData.ImuLinAccelX:0.00}";


        //            Temp.Text = $"{TelemetryData.AtmTemp:0.0} C / {tempInF:0.0} F";
        //            Humidity.Text = $"{TelemetryData.AtmHumidity:0.0}";
        //            Pressure.Text = $"{TelemetryData.AtmPressure:0.0}";
        //            Altitude.Text = $"{TelemetryData.SensorAltitudeMeters:0.0}";


        //            Fix.Text = $"{_gpsData.Fix:0}";
        //            Satellites.Text = $"{_gpsData.Satellites:0}";
        //            Sound.Text = $"{_decibels:0.0}";
        //            Voltage.Text = $"{TelemetryData.DevVoltage:0.0}";
        //        });
        //}


        // TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN 
        // TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN  TEARDOWN 

        private void CancelAllIoTasks()
        {
            CancelTelemetryReadTask();
            CancelGpsReadTask();
            CancelWriteTask();
        }

        public void Dispose()
        {
            if (_gpsReadCancellationTokenSource != null)
            {
                _gpsReadCancellationTokenSource.Dispose();
                _gpsReadCancellationTokenSource = null;
            }

            if (_telemetryReadCancellationTokenSource != null)
            {
                _telemetryReadCancellationTokenSource.Dispose();
                _telemetryReadCancellationTokenSource = null;
            }

            if (_writeCancellationTokenSource != null)
            {
                _writeCancellationTokenSource.Dispose();
                _writeCancellationTokenSource = null;
            }
        }


        /// <summary>
        /// Shutdown or Reboot the Device
        /// More Details:
        /// https://msdn.microsoft.com/en-us/library/windows/apps/mt694883.aspx
        /// </summary>
        /// <param name="kind"></param>
        private void ShutdownHelper(ShutdownKind kind)
        {
            CancelAllIoTasks();

            new Task(() =>
            {
                ShutdownManager.BeginShutdown(kind, TimeSpan.FromSeconds(0));
            }).Start();
        }



        // INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  
        // INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  INITIALIZATION  

        ///// <summary>
        ///// reads the configuration file and converts the JSON object to a C# data object
        ///// </summary>
        ///// <param name="filename">Name of the configuration file</param>
        ///// <returns></returns>
        //public static void ReadConfigurationFile(string filename)
        //{
        //    // access the local folder
        //    var local = ApplicationData.Current.LocalFolder;

        //    Task<Stream> task = null;
        //    try
        //    {
        //        // open the file 'filename' for reading
        //        task = local.OpenStreamForReadAsync(filename);
        //        Task.WaitAll(task);
        //    }
        //    catch (Exception ex)
        //    {
        //        Debug.WriteLine($"Error opening file stream: {ex.Message}");
        //    }

        //    if (task != null)
        //    {
        //        // copy the file contents into the string 'text'
        //        string text;

        //        using (var reader = new StreamReader(task.Result))
        //        {
        //            text = reader.ReadToEnd();
        //        }

        //        try
        //        {
        //            _deviceConfigurationData = JsonConvert.DeserializeObject<DeviceConfiguration>(text);
        //        }
        //        catch (Exception ex)
        //        {
        //            Debug.WriteLine($"Error reading configuration data (JSON): {ex.Message}");
        //        }
        //    }
        //}


        private void SetupSerialConnections()
        {
            var deviceSelector = SerialDevice.GetDeviceSelectorFromUsbVidPid(0x10C4, 0xEA60);

            var task = Task.Run(async () => await DeviceInformation.FindAllAsync(deviceSelector));
            task.Wait();
            var deviceInformationCollection = task.Result;

            if (deviceInformationCollection.Any())
            {
                var count = deviceInformationCollection.Count;

                if (count != 3)
                {
                    Debug.WriteLine("Error: There are only {0} serial devices recognized.", count);
                }

                foreach (var deviceInfo in deviceInformationCollection)
                {
                    Debug.WriteLine("DeviceID: " + deviceInfo.Id);
                    Debug.WriteLine("Device Name: " + deviceInfo.Name);

                    var deviceTask = Task.Run(async () => await SerialDevice.FromIdAsync(deviceInfo.Id));
                    deviceTask.Wait();

                    var deviceId = deviceInfo.Id.ToUpper();

                    deviceTask.Result.Parity = SerialParity.None;
                    deviceTask.Result.StopBits = SerialStopBitCount.One;
                    deviceTask.Result.DataBits = 8;
                    deviceTask.Result.Handshake = SerialHandshake.None;
                    deviceTask.Result.WriteTimeout = TimeSpan.FromMilliseconds(1000);
                    deviceTask.Result.ReadTimeout = TimeSpan.FromMilliseconds(500);

                    if (deviceId.Contains("A&0&3#"))
                    {
                        _gpsDevice = deviceTask.Result;

                        //_gpsDevice.ErrorReceived += _gpsDevice_ErrorReceived;

                        _gpsDevice.BaudRate = SerialBaudRateGps;
                    }
                    else if (deviceId.Contains("A&0&4#"))
                    {
                        _userTextDevice = deviceTask.Result;

                        _userTextDevice.BaudRate = SerialBaudRateUserText;
                    }
                    else if (deviceId.Contains("0001"))
                    {
                        _telemetryDevice = deviceTask.Result;

                        _telemetryDevice.BaudRate = SerialBaudRateTelemetry;
                    }

                    _serialConnectionCount++;
                }
            }

            Debug.WriteLine($"Serial Connection Count: {_serialConnectionCount}");
        }

        private void _gpsDevice_ErrorReceived(SerialDevice sender, ErrorReceivedEventArgs args)
        {
            Debug.WriteLine($">>>>>>> GPS ERROR RECEIVED: {args.Error}");
        }

        //private DispatcherTimer _displayUpdateTimer;

        private async Task SetupTimers()
        {
            ThreadPoolTimer.CreatePeriodicTimer(PeriodicTelemetrySendTimerCallback,
                TimeSpan.FromMilliseconds(500));

            //ThreadPoolTimer.CreatePeriodicTimer(
            //    async (source) =>
            //    {
            //        // we have to update UI in UI thread only
            //        await CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
            //        () =>
            //        {
            //            var speedInMph = _gpsData.SpeedInKph * MilesPerKilometer;
            //            var tempInF = TelemetryData.AtmTemp * 1.8 + 32;
            //            Coordinates.Text = $"{_gpsData.LatitudeDegrees:0.0000000} / {_gpsData.LongitudeDegrees:0.0000000}";
            //            Speed.Text = $"{speedInMph:0.0}";
            //            Direction.Text = $"{_gpsData.Direction:0.0}";
            //            Direction.Text = $"{TelemetryData.ImuHeading:0.0}";
            //            Acceleration.Text = $"{TelemetryData.ImuLinAccelX:0.00}";


            //            Temp.Text = $"{TelemetryData.AtmTemp:0.0} C / {tempInF:0.0} F";
            //            Humidity.Text = $"{TelemetryData.AtmHumidity:0.0}";
            //            Pressure.Text = $"{TelemetryData.AtmPressure:0.0}";
            //            Altitude.Text = $"{TelemetryData.SensorAltitudeMeters:0.0}";


            //            Fix.Text = $"{_gpsData.Fix:0}";
            //            Satellites.Text = $"{_gpsData.Satellites:0}";
            //            Sound.Text = $"{_decibels:0.0}";
            //            Voltage.Text = $"{TelemetryData.DevVoltage:0.0}";
            //        }
            //        );
            //    }, TimeSpan.FromMilliseconds(1000));



            //_displayUpdateTimer = new DispatcherTimer {Interval = TimeSpan.FromMilliseconds(1000)};
            //_displayUpdateTimer.Tick += PeriodicUpdateScreenTimerCallback;
            //_displayUpdateTimer.Start();



            ThreadPoolTimer.CreatePeriodicTimer(UserMessageTimerCallback,
                TimeSpan.FromMilliseconds(1500));

            while (true)
            {
                await PeriodicGetSerialDataCallback();
            }

            //ThreadPoolTimer.CreatePeriodicTimer(PeriodicGetSerialDataCallback,
            //    TimeSpan.FromMilliseconds(50));
        }


        // MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  
        // MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  MAIN  

        public List<Scenario> Scenarios { get; } = new List<Scenario> { };

        public MainPage()
        {
            InitializeComponent();

            Current = this;

            _localTelemetryFile = "Telemetry.csv";

            //var width = Window.Current.Bounds.Width;
            //var height = Window.Current.Bounds.Height;

            Debug.WriteLine("DateTime: " + DateTimeSystem);
            Debug.WriteLine("Starting...");

            ResetWriteCancellationTokenSource();
            ResetGpsReadCancellationTokenSource();
            ResetTelemetryReadCancellationTokenSource();

            _userMessageQueue = new Queue<string>();

            SetupUdpConnection();

            SetupSerialConnections();

           //var task = SetupTimers();
           //Task.WaitAll(task);

            Task task = Task.Factory.StartNew(async () => { await SetupTimers(); } );
            Task.WaitAll(task);


            //var timerTask = Task.Run(async () => await SetupTimers());
            //timerTask.Wait();
        }
    }


    public class ScenarioBindingConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            var s = value as Scenario;

            if (s != null) return (MainPage.Current.Scenarios.IndexOf(s) + 1) + ") " + s.Title;

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            return true;
        }
    }
}
