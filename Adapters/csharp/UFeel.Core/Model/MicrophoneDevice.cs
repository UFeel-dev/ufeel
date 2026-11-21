#nullable enable

using System;

namespace UFeel
{
    /// <summary>A microphone usable for speech-to-text, identified by its name.</summary>
    public sealed class MicrophoneDevice
    {
        public MicrophoneDevice(string name, bool isDefault)
        {
            Name = name ?? throw new ArgumentNullException(nameof(name));
            IsDefault = isDefault;
        }

        public string Name { get; }

        /// <summary>The microphone used when none is selected.</summary>
        public bool IsDefault { get; }
    }
}
