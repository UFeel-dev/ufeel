#nullable enable

using System;
using System.Threading;

namespace UFeel
{
    /// <summary>A feature is active while pinned, or for <c>timeout</c> after its last <see cref="Request"/>. Thread-safe.</summary>
    public sealed class FeatureDemand
    {
        private readonly long timeoutTicks;
        private readonly long[] lastRequest = new long[DetectionTypes.Each.Length];
        private readonly int[] pins = new int[DetectionTypes.Each.Length];

        public FeatureDemand(TimeSpan timeout)
        {
            timeoutTicks = timeout.Ticks;
            for (int i = 0; i < lastRequest.Length; i++)
                lastRequest[i] = long.MinValue;
        }

        public static FeatureDemand CreateDefault() => new FeatureDemand(TimeSpan.FromSeconds(2));

        public void Request(DetectionType features)
        {
            long now = Clock.Now;
            for (int i = 0; i < DetectionTypes.Each.Length; i++)
            {
                if (features.Includes(DetectionTypes.Each[i]))
                    Interlocked.Exchange(ref lastRequest[i], now);
            }
        }

        /// <summary>Pins are counted: each needs a matching <see cref="Unpin"/>.</summary>
        public void Pin(DetectionType features) => AddPins(features, 1);

        public void Unpin(DetectionType features) => AddPins(features, -1);

        public DetectionType Active(long timestamp)
        {
            DetectionType active = DetectionType.None;
            for (int i = 0; i < DetectionTypes.Each.Length; i++)
            {
                long last = Interlocked.Read(ref lastRequest[i]);
                bool requested = last != long.MinValue && Clock.Elapsed(last, timestamp).Ticks <= timeoutTicks;

                if (requested || Volatile.Read(ref pins[i]) > 0)
                    active |= DetectionTypes.Each[i];
            }

            return active;
        }

        private void AddPins(DetectionType features, int delta)
        {
            for (int i = 0; i < DetectionTypes.Each.Length; i++)
            {
                if (features.Includes(DetectionTypes.Each[i]))
                    Interlocked.Add(ref pins[i], delta);
            }
        }
    }
}
