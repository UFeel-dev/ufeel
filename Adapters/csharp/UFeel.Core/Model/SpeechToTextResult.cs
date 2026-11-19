#nullable enable

namespace UFeel
{
    public enum SpeechToTextStatus
    {
        Disabled = 0,
        NoSpeech = 1,
        Valid = 2,
    }

    /// <summary>The native side clears the text 3 s after the last recognized speech.</summary>
    public readonly struct SpeechToTextResult
    {
        private readonly string? text;

        public SpeechToTextResult(SpeechToTextStatus status, string text)
        {
            Status = status;
            this.text = text;
        }

        public SpeechToTextStatus Status { get; }
        public string Text => text ?? string.Empty;
        public bool IsValid => Status == SpeechToTextStatus.Valid;

        public static SpeechToTextResult Unavailable(SpeechToTextStatus status)
        {
            return new SpeechToTextResult(status, string.Empty);
        }
    }
}
