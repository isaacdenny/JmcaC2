using System.Net;
using System.Security.Cryptography;

namespace JmcaC2
{
    public class BeaconClient(IPAddress clientIP, DateTime lastCheckInTime, string clientName)
    {

        // parameterized constructor
        public IPAddress ClientIP { get; init; } = clientIP;
        public string Name { get; init; } = clientName;
        public DateTime LastCheckInTime { get; set; } = lastCheckInTime;

        // public string ClientID 

        public override string ToString()
        {
            TimeSpan checkInTime = DateTime.Now - LastCheckInTime;

            string uptime = $"{checkInTime.Days}d {checkInTime.Hours}h {checkInTime.Minutes}m {checkInTime.Seconds}s";

            return $"{Name,-15} |{ClientIP,-15} | {uptime,-20}";
        }

        // public async void SendTask(string CmdPrefix, string CmdArgs)
        // {

        //     HttpClient client = new HttpClient();

        //     switch (CmdPrefix)
        //     {
        //         case "powershell":
        //             Dictionary<string, string> command = new Dictionary<string, string> { { CmdPrefix, CmdArgs } };
        //             FormUrlEncodedContent content = new FormUrlEncodedContent(command);
        //             var response = await client.PostAsync($"http://{ClientIP}:27015", content);
        //             var responseString = await response.Content.ReadAsStringAsync();

        //             break;

        //         default:
        //             Console.WriteLine("Unable to complete task!");
        //             break;
        //     }


        // Send HTTP request with command

        // Console.WriteLine($"Powershell command {args}");


        // }
    }
}