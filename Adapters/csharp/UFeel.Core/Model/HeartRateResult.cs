#nullable enable

namespace UFeel
{
    public enum HeartRateStatus
    {
        JsonParseError = 0,
        JsonParseOutOfRange = 1,
        RequestFailed = 2,
        Valid = 3,
        NoData = 4,
    }

    public readonly struct HeartRateResult
    {
        public HeartRateResult(HeartRateStatus status, uint bpm)
        {
            Status = status;
            Bpm = bpm;
        }

        public HeartRateStatus Status { get; }
        public uint Bpm { get; }
        public bool IsValid => Status == HeartRateStatus.Valid;

        public static HeartRateResult Unavailable(HeartRateStatus status) => new HeartRateResult(status, 0);
    }
}
