using System.Net;
using System.Security.Cryptography;

namespace JmcaC2
{
    public class BeaconClient(IPAddress clientIP, DateTime lastCheckInTime, string clientName)
    {

        // parameterized constructor
        public IPAddress ClientIP { get; init; } = clientIP;
        public DateTime LastCheckInTime { get; set; } = lastCheckInTime;
        public string Name { get; init; } = clientName;

        // public string ClientID 

        public override string ToString()
        {
            TimeSpan checkInTime = DateTime.Now - LastCheckInTime;

            string uptime = $"{checkInTime.Days}d {checkInTime.Hours}h {checkInTime.Minutes}m {checkInTime.Seconds}s";

            return $"{Name,-15} |{ClientIP,-15} | {uptime,-20}";
        }
    }
}