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
        public int Index;
        public string Cmd;
        public string Data;
        public TaskStatus Status;

        public BeaconTask(string name, int idx, string cmd, string data)
        {
            this.Name = name;
            this.Index = idx;
            this.Cmd = cmd;
            this.Data = data;
            this.Status = TaskStatus.NotStarted;
        }

        public override string ToString()
        {
            return this.Index + "|" + this.Cmd + "|" + this.Data;
        }
    }
}