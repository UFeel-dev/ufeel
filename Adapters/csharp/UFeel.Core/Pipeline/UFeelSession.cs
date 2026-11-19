#nullable enable

using System;
using System.Collections.Concurrent;
using System.Threading;
using System.Threading.Tasks;

namespace UFeel
{
    /// <summary>
    /// Owns the processor and runs it on a dedicated thread. After <see cref="Start"/>,
    /// reach the processor only through <see cref="ExecuteAsync{T}"/>.
    /// </summary>
    public sealed class UFeelSession : IDisposable
    {
        private const string FrameUnavailableMessage = "Camera frame unavailable";

        private static readonly TimeSpan IdleWait = TimeSpan.FromMilliseconds(20);
        private static readonly TimeSpan FrameRetryWait = TimeSpan.FromMilliseconds(50);
        private static readonly TimeSpan ErrorRetryWait = TimeSpan.FromMilliseconds(500);

        private readonly UFeelProcessor processor;
        private readonly IFrameObserver? frameObserver;
        private readonly LatestFrameCapture capture = new LatestFrameCapture();
        private readonly EyeTrackingSmoother smoother = new EyeTrackingSmoother();
        private readonly ConcurrentQueue<IPendingCommand> commands = new ConcurrentQueue<IPendingCommand>();
        private readonly AutoResetEvent wake = new AutoResetEvent(false);
        private readonly Thread thread;

        private UFeelSnapshot latest = UFeelSnapshot.Initial;
        private int supported = (int)DetectionTypes.All;
        private volatile bool speechEnabled;
        private volatile bool stopping;
        private bool disposed;

        private EmotionResult emotions = UFeelSnapshot.Initial.Emotions;
        private EyeTrackingResult eyeTracking = UFeelSnapshot.Initial.EyeTracking;
        private SpeechToTextResult speechToText = UFeelSnapshot.Initial.SpeechToText;
        private HeartRateResult heartRate = UFeelSnapshot.Initial.HeartRate;
        private DetectionType lastActive;
        private bool frameAvailable;
        private long sequence;

        public UFeelSession(
            UFeelProcessor processor,
            FeatureScheduler? scheduler = null,
            FeatureDemand? demand = null,
            IFrameObserver? frameObserver = null)
        {
            this.processor = processor ?? throw new ArgumentNullException(nameof(processor));
            this.frameObserver = frameObserver;
            Scheduler = scheduler ?? FeatureScheduler.CreateDefault();
            Demand = demand ?? FeatureDemand.CreateDefault();
            thread = new Thread(Run) { IsBackground = true, Name = "UFeel session" };
        }

        /// <summary>Raised on the session thread after each published snapshot.</summary>
        public event Action<UFeelSnapshot>? SnapshotPublished;

        public FeatureScheduler Scheduler { get; }
        public FeatureDemand Demand { get; }

        public UFeelSnapshot Latest => Volatile.Read(ref latest);

        /// <summary>Features built into the native library, discovered while running.</summary>
        public DetectionType SupportedFeatures => (DetectionType)Volatile.Read(ref supported);

        public bool IsSpeechToTextEnabled => speechEnabled;

        public void Start() => thread.Start();

        /// <summary>Runs on the session thread, between two processing steps.</summary>
        public Task<T> ExecuteAsync<T>(Func<UFeelProcessor, T> command)
        {
            if (command is null)
                throw new ArgumentNullException(nameof(command));

            var pending = new PendingCommand<T>(command);
            if (stopping)
            {
                pending.Cancel();
                return pending.Task;
            }

            commands.Enqueue(pending);
            wake.Set();

            // * dispose may have drained the queue between the check above and the enqueue
            if (stopping)
                CancelPendingCommands();

            return pending.Task;
        }

        public Task ExecuteAsync(Action<UFeelProcessor> command)
        {
            if (command is null)
                throw new ArgumentNullException(nameof(command));

            return ExecuteAsync(p =>
            {
                command(p);
                return true;
            });
        }

        public Task SetSpeechToTextEnabledAsync(bool enabled)
        {
            return ExecuteAsync(p =>
            {
                p.SetSpeechToTextEnabled(enabled);
                if (speechEnabled == enabled)
                    return;

                speechEnabled = enabled;
                if (enabled)
                {
                    Demand.Pin(DetectionType.SpeechToText);
                }
                else
                {
                    Demand.Unpin(DetectionType.SpeechToText);
                    speechToText = SpeechToTextResult.Unavailable(SpeechToTextStatus.Disabled);
                    Publish(Latest.Message);
                }
            });
        }

        public void Dispose()
        {
            if (disposed)
                return;

            disposed = true;
            stopping = true;
            wake.Set();

            if (thread.IsAlive)
                thread.Join();

            CancelPendingCommands();

            try
            {
                if (speechEnabled)
                    processor.SetSpeechToTextEnabled(false);
            }
            finally
            {
                processor.Dispose();
                wake.Dispose();
            }
        }

        private void Run()
        {
            while (!stopping)
            {
                try
                {
                    RunCommands();
                    Step();
                }
                catch (Exception exception)
                {
                    Publish(exception.Message);
                    wake.WaitOne(ErrorRetryWait);
                }
            }
        }

        private void RunCommands()
        {
            while (!stopping && commands.TryDequeue(out IPendingCommand pending))
                pending.Run(processor);
        }

        private void CancelPendingCommands()
        {
            while (commands.TryDequeue(out IPendingCommand pending))
                pending.Cancel();
        }

        private void Step()
        {
            DetectionType active = Demand.Active(Clock.Now) & SupportedFeatures;
            bool activeChanged = active != lastActive;
            if (activeChanged)
            {
                ResetInactive(active);
                lastActive = active;
            }

            bool needsFrames = active.Includes(DetectionTypes.Image) || frameObserver != null;
            if (!needsFrames)
                capture.Reset();

            bool wasAvailable = frameAvailable;
            frameAvailable = needsFrames && CaptureFrame();

            DetectionType due = Scheduler.GetDue(
                Clock.Now,
                frameAvailable ? active : active & ~DetectionTypes.Image);

            if (due != DetectionType.None)
            {
                UFeelStatus status = Process(due);
                Publish(status == UFeelStatus.Ok || status == UFeelStatus.UnsupportedFeature
                    ? CurrentFrameMessage(needsFrames)
                    : $"Detection failed ({status}), retrying on the next frame");
                return;
            }

            if (needsFrames && !frameAvailable)
            {
                if (wasAvailable || activeChanged || Latest.Sequence == 0)
                    Publish(FrameUnavailableMessage);

                wake.WaitOne(FrameRetryWait);
                return;
            }

            if (activeChanged)
                Publish(CurrentFrameMessage(needsFrames));

            if (!needsFrames)
                wake.WaitOne(IdleWait);
        }

        private bool CaptureFrame()
        {
            UFeelStatus status = capture.Capture(processor);
            if (status == UFeelStatus.Ok)
            {
                frameObserver?.OnFrameCaptured(processor);
                return true;
            }

            if (status != UFeelStatus.InvalidFrame)
                throw new UFeelException(status, "capture a camera frame");

            emotions = EmotionResult.Unavailable(EmotionStatus.FrameUnavailable);
            eyeTracking = EyeTrackingResult.Unavailable(EyeTrackingStatus.FrameUnavailable);
            smoother.Reset();
            return false;
        }

        private UFeelStatus Process(DetectionType due)
        {
            UFeelStatus status = Update(due);
            if (status == UFeelStatus.Ok)
            {
                ReadResults(due);
                return status;
            }

            if (status != UFeelStatus.UnsupportedFeature)
                return status;

            // * a combined call fails as a whole, so we retry each feature to find the missing ones
            // TODO: update the native library to return a bitmask of unsupported features instead of failing the whole call
            UFeelStatus combined = UFeelStatus.Ok;
            foreach (DetectionType feature in DetectionTypes.Each)
            {
                if (!due.Includes(feature))
                    continue;

                UFeelStatus single = Update(feature);
                if (single == UFeelStatus.Ok)
                    ReadResults(feature);
                else if (single == UFeelStatus.UnsupportedFeature)
                    MarkUnsupported(feature);
                else
                    combined = single;
            }

            return combined;
        }

        private UFeelStatus Update(DetectionType features)
        {
            if (!features.Includes(DetectionTypes.Image))
                return processor.Update(features);

            FrameData? frame = processor.GetFrame();
            return frame is null ? UFeelStatus.InvalidFrame : processor.UpdateFromFrame(features, frame);
        }

        private void ReadResults(DetectionType features)
        {
            if (features.Includes(DetectionType.Emotions))
                emotions = processor.GetEmotions();

            if (features.Includes(DetectionType.EyeTracking))
                eyeTracking = smoother.Smooth(processor.GetEyeTracking());

            if (features.Includes(DetectionType.SpeechToText))
            {
                speechToText = speechEnabled
                    ? processor.GetSpeechToText()
                    : SpeechToTextResult.Unavailable(SpeechToTextStatus.Disabled);
            }

            if (features.Includes(DetectionType.HeartRate))
                heartRate = processor.GetHeartRate();
        }

        private void ResetInactive(DetectionType active)
        {
            if (!active.Includes(DetectionType.Emotions))
                emotions = UFeelSnapshot.Initial.Emotions;

            if (!active.Includes(DetectionType.EyeTracking))
            {
                eyeTracking = UFeelSnapshot.Initial.EyeTracking;
                smoother.Reset();
            }

            if (!active.Includes(DetectionType.HeartRate))
                heartRate = UFeelSnapshot.Initial.HeartRate;
        }

        private void MarkUnsupported(DetectionType feature)
        {
            int remaining = Volatile.Read(ref supported) & ~(int)feature;
            Volatile.Write(ref supported, remaining);

            switch (feature)
            {
                case DetectionType.Emotions:
                    emotions = EmotionResult.Unavailable(EmotionStatus.UnsupportedFeature);
                    break;
                case DetectionType.EyeTracking:
                    eyeTracking = EyeTrackingResult.Unavailable(EyeTrackingStatus.Disabled);
                    break;
                case DetectionType.SpeechToText:
                    speechToText = SpeechToTextResult.Unavailable(SpeechToTextStatus.Disabled);
                    break;
                case DetectionType.HeartRate:
                    heartRate = HeartRateResult.Unavailable(HeartRateStatus.NoData);
                    break;
            }
        }

        private string? CurrentFrameMessage(bool needsFrames)
        {
            return needsFrames && !frameAvailable ? FrameUnavailableMessage : null;
        }

        private void Publish(string? message)
        {
            var snapshot = new UFeelSnapshot(
                ++sequence,
                frameAvailable,
                message,
                emotions,
                eyeTracking,
                speechToText,
                heartRate);

            Volatile.Write(ref latest, snapshot);
            SnapshotPublished?.Invoke(snapshot);
        }

        private interface IPendingCommand
        {
            void Run(UFeelProcessor processor);
            void Cancel();
        }

        private sealed class PendingCommand<T> : IPendingCommand
        {
            private readonly Func<UFeelProcessor, T> command;
            private readonly TaskCompletionSource<T> completion =
                new TaskCompletionSource<T>(TaskCreationOptions.RunContinuationsAsynchronously);

            public PendingCommand(Func<UFeelProcessor, T> command) => this.command = command;

            public Task<T> Task => completion.Task;

            public void Run(UFeelProcessor processor)
            {
                try
                {
                    completion.TrySetResult(command(processor));
                }
                catch (Exception exception)
                {
                    completion.TrySetException(exception);
                }
            }

            public void Cancel() => completion.TrySetCanceled();
        }
    }
}
