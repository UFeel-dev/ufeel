#nullable enable

using System;

namespace UFeel
{
    public sealed class MicrophoneDevice
    {
        public MicrophoneDevice(string id, string name, bool isDefault)
        {
            Id = id ?? throw new ArgumentNullException(nameof(id));
            Name = name ?? throw new ArgumentNullException(nameof(name));
            IsDefault = isDefault;
        }

        /// <summary>Stable across runs, used to select the microphone.</summary>
        public string Id { get; }

        public string Name { get; }

        /// <summary>The system default microphone, used when none is selected.</summary>
        public bool IsDefault { get; }
    }
}
