namespace JmcaC2
{
    public enum TaskStatus
    {
        NotStarted,
        InProgress,
        Completed
    }
    internal class BeaconTask
    {
        public string Name;
        public string Cmd;
        public string Data;
        public TaskStatus Status;

        public BeaconTask(string name, string cmd, string data)
        {
            this.Name = name;
            this.Cmd = cmd;
            this.Data = data;
            this.Status = TaskStatus.NotStarted;
        }

        public override string ToString()
        {
            return this.Cmd + "|" + this.Data;
        }
    }
}