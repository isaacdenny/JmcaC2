using System.Net;

namespace JmcaC2
{
    public class Program
    {
        static HttpListener listener = new HttpListener();
        static bool programRunning = true;
        static bool serverRunning = true;
        static int port = 27015;

        static List<BeaconClient> Clients = new List<BeaconClient>();

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

                cmd = cmd.Trim();

                // process command
                switch (cmd)
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
                    case "listeners":
                        ViewConnections();
                        break;
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
        public static void ViewConnections()
        {
            if (Clients == null)
            {
                Console.WriteLine("No Active Connections!");
                return;
            }

            Console.WriteLine($"{"Client IP",-15} | {"Last CheckIn",-20}");
            foreach (BeaconClient Client in Clients)
            {

                Console.WriteLine(Client);

                // TODO: MAKE RED if configured last-checkin time > sleep time 
            }
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
                    if (request.HttpMethod == "GET")
                    {
                        // Handle GET request for beacon tasks
                        Console.WriteLine($"Received request for tasks from {request.RemoteEndPoint.Address}");

                        string responseString = "No tasks";
                        byte[] buffer = System.Text.Encoding.UTF8.GetBytes(responseString);
                        response.ContentLength64 = buffer.Length;

                        var output = response.OutputStream;
                        output.Write(buffer, 0, buffer.Length);
                        output.Close();

                        Clients.Add(new BeaconClient(request.RemoteEndPoint.Address, DateTime.Now));

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