namespace NaeDeviceApp
{
    public class DateTimeTracker
    {
        public int Year { get; private set; }
        public int Month { get; private set; }
        public int Day { get; private set; }
        public int Hour { get; private set; }
        public int Minute { get; private set; }
        public int Second { get; private set; }

        public DateTimeTracker(int year, int month, int day, int hour, int minute, int second)
        {
            Year = year;
            Month = month;
            Day = day;
            Hour = hour;
            Minute = minute;
            Second = second;
        }

        public void SetDate(int year, int month, int day)
        {
            Year = year;
            Month = month;
            Day = day;
        }

        public void SetTime(int hour, int minute, int second)
        {
            Hour = hour;
            Minute = minute;
            Second = second;
        }

        public string PrintDateTime()
        {
            return $"{Year}-{Month:D2}-{Day:D2}T{Hour:D2}:{Minute:D2}:{Second:D2}";
        }

    }
}
