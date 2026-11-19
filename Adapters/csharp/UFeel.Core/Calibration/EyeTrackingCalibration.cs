#nullable enable

using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace UFeel
{
    public sealed class EyeTrackingCalibration
    {
        public static readonly (float X, float Y)[] Targets =
        {
            (0.5F, 0.5F),
            (0.1F, 0.1F),
            (0.1F, 0.9F),
            (0.9F, 0.1F),
            (0.9F, 0.9F),
            (0.1F, 0.5F),
            (0.5F, 0.1F),
            (0.9F, 0.5F),
            (0.5F, 0.9F),
        };

        private static readonly TimeSpan SteadyFaceDuration = TimeSpan.FromSeconds(2);
        private static readonly TimeSpan ReadinessPoll = TimeSpan.FromMilliseconds(100);
        private static readonly TimeSpan MaxTargetDuration = TimeSpan.FromSeconds(4);
        private static readonly TimeSpan ProgressPoll = TimeSpan.FromMilliseconds(200);

        private readonly UFeelSession session;
        private readonly ICalibrationPresenter presenter;
        private readonly CalibrationProfileStore? profiles;
        private int running;

        public EyeTrackingCalibration(
            UFeelSession session,
            ICalibrationPresenter presenter,
            CalibrationProfileStore? profiles = null)
        {
            this.session = session ?? throw new ArgumentNullException(nameof(session));
            this.presenter = presenter ?? throw new ArgumentNullException(nameof(presenter));
            this.profiles = profiles;
        }

        public bool IsRunning => Volatile.Read(ref running) != 0;

        /// <summary>Cancelling keeps the previously active model.</summary>
        public async Task<CalibrationResult> RunAsync(CancellationToken token)
        {
            if (Interlocked.Exchange(ref running, 1) != 0)
                throw new InvalidOperationException("Eye-tracking calibration is already running.");

            bool nativeActive = false;

            // * calibration collects one sample per eye-tracking update, so it must not be throttled.
            session.Demand.Pin(DetectionType.EyeTracking);
            session.Scheduler.SetOverride(DetectionType.EyeTracking, TimeSpan.Zero);

            try
            {
                await session.ExecuteAsync(p => p.BeginEyeTrackingCalibration());
                nativeActive = true;

                presenter.Show();
                presenter.ShowTarget(Targets[0].X, Targets[0].Y);
                await WaitForSteadyFaceAsync(token);

                for (int i = 0; i < Targets.Length; i++)
                    await CollectTargetAsync(i, token);

                CalibrationResult result = await session.ExecuteAsync(Finish);
                nativeActive = false;
                return result;
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                return CalibrationResult.Cancel();
            }
            finally
            {
                session.Scheduler.ClearOverride(DetectionType.EyeTracking);
                session.Demand.Unpin(DetectionType.EyeTracking);

                if (nativeActive)
                    await CancelNativeCalibrationAsync();

                presenter.Hide();
                Volatile.Write(ref running, 0);
            }
        }

        private async Task WaitForSteadyFaceAsync(CancellationToken token)
        {
            long? steadySince = null;

            while (true)
            {
                token.ThrowIfCancellationRequested();

                if (session.Latest.EyeTracking.Status == EyeTrackingStatus.Calibrating)
                {
                    steadySince ??= Clock.Now;
                    presenter.ShowInstruction("Face detected. Keep your face steady and eyes open");
                    presenter.ShowStatus("Hold a steady position before calibration");

                    if (Clock.Since(steadySince.Value) >= SteadyFaceDuration)
                        return;
                }
                else
                {
                    steadySince = null;
                    presenter.ShowInstruction("Position your face in the camera view and keep your eyes open");
                    presenter.ShowStatus("Waiting for a valid face and eye landmarks");
                }

                await Task.Delay(ReadinessPoll, token);
            }
        }

        private async Task CollectTargetAsync(int index, CancellationToken token)
        {
            (float x, float y) = Targets[index];
            string position = $"{index + 1}/{Targets.Length}";

            // * the native target must change with the displayed one, its settle time skips the eye movement
            presenter.ShowTarget(x, y);
            presenter.ShowInstruction($"Look at the target and keep your head still ({position})");
            await session.ExecuteAsync(p => p.SetEyeTrackingCalibrationTarget(x, y));

            long start = Clock.Now;
            while (true)
            {
                CalibrationProgress progress = await session.ExecuteAsync(p => p.GetEyeTrackingCalibrationProgress());
                string samples = $"{progress.SamplesForCurrentTarget}/{progress.RequiredSamplesPerTarget} usable frames";

                presenter.ShowStatus($"Calibration target {position}: {samples}");
                presenter.ShowInstruction($"Keep looking at the target ({samples})");

                TimeSpan elapsed = Clock.Since(start);
                if (progress.CurrentTargetComplete || elapsed >= MaxTargetDuration)
                    return;

                await Task.Delay(ProgressPoll, token);
            }
        }

        private CalibrationResult Finish(UFeelProcessor processor)
        {
            if (!processor.FinishEyeTrackingCalibration())
                return CalibrationResult.Failure(processor.GetEyeTrackingCalibrationDiagnostics());

            bool saved = false;
            if (profiles != null)
            {
                try
                {
                    saved = profiles.Save(processor);
                }
                catch (Exception exception) when (exception is IOException || exception is UnauthorizedAccessException)
                {
                    saved = false;
                }
            }

            return CalibrationResult.Success(saved, profiles != null);
        }

        private async Task CancelNativeCalibrationAsync()
        {
            try
            {
                await session.ExecuteAsync(p => p.CancelEyeTrackingCalibration());
            }
            catch (Exception exception) when (exception is UFeelException || exception is OperationCanceledException)
            {
            }
        }
    }
}
