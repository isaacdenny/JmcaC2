using System.Net;

namespace JmcaC2
{
    public class BeaconClient(IPAddress clientIP, DateTime lastCheckInTime)
    {

        // parameterized constructor
        public IPAddress ClientIP { get; init; } = clientIP;
        public DateTime LastCheckInTime { get; set; } = lastCheckInTime;
        public override string ToString()
        {
            TimeSpan checkInTime = DateTime.Now - LastCheckInTime;

            string uptime = $"{checkInTime.Days}d {checkInTime.Hours}h {checkInTime.Minutes}m {checkInTime.Seconds}s";

            return $"{ClientIP,-15} | {uptime,-20}";
        }
    }
}