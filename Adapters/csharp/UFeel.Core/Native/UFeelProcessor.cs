#nullable enable

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UFeel.Interop;
using UFeel.Native;

namespace UFeel
{
    /// <summary>Not thread-safe: result buffers are reused between calls (see <see cref="UFeelSession"/>).</summary>
    public sealed class UFeelProcessor : IDisposable
    {
        private readonly EmotionsReader emotions;
        private readonly EyeTrackingReader eyeTracking;
        private readonly SpeechToTextReader speechToText;
        private readonly MicrophonesReader microphones;
        private readonly HeartRateReader heartRate;
        private readonly FrameReader frames = new FrameReader();

        private IntPtr handle;

        public UFeelProcessor(UFeelPackageLayout layout)
            : this(layout.ModelDirectory, layout.SpeechModelPath)
        {
        }

        // TODO: remove the speech model path when the native library is built without speech support, or allow passing null/empty in that case
        // * -> (currently always requires a valid path).
        public UFeelProcessor(string modelDirectory, string speechModelPath)
        {
            RequirePath(modelDirectory, nameof(modelDirectory));
            RequirePath(speechModelPath, nameof(speechModelPath));

            handle = NativeMethods.ufeel_create(modelDirectory, speechModelPath);
            if (handle == IntPtr.Zero)
                throw new InvalidOperationException("Failed to create the native UFeel processor.");

            try
            {
                emotions = new EmotionsReader();
                eyeTracking = new EyeTrackingReader();
                speechToText = new SpeechToTextReader();
                microphones = new MicrophonesReader();
                heartRate = new HeartRateReader();
            }
            catch
            {
                Dispose();
                throw;
            }
        }

        /// <summary>Image features capture a new camera frame first.</summary>
        public UFeelStatus Update(DetectionType detection)
        {
            return (UFeelStatus)NativeMethods.ufeel_update(Handle, (int)detection);
        }

        public UFeelStatus UpdateFromFrame(DetectionType detection, FrameData frame)
        {
            if (frame is null)
                throw new ArgumentNullException(nameof(frame));

            IntPtr processor = Handle;
            GCHandle pinned = GCHandle.Alloc(frame.Data, GCHandleType.Pinned);
            try
            {
                var nativeFrame = new NativeFrame
                {
                    Data = pinned.AddrOfPinnedObject(),
                    Width = frame.Width,
                    Height = frame.Height,
                    Channels = frame.Channels,
                    Stride = frame.Stride,
                    DataSize = (ulong)frame.Data.Length,
                };

                return (UFeelStatus)NativeMethods.ufeel_update_from_frame(processor, (int)detection, in nativeFrame);
            }
            finally
            {
                pinned.Free();
            }
        }

        /// <summary>Returns <see cref="UFeelStatus.InvalidFrame"/> without a camera.</summary>
        public UFeelStatus UpdateFrame()
        {
            return (UFeelStatus)NativeMethods.ufeel_update_frame(Handle);
        }

        /// <summary>The returned buffer is reused by the next call.</summary>
        public FrameData? GetFrame() => frames.Read(Handle);

        public bool TryGetFrameSize(out int width, out int height)
        {
            bool available = FrameReader.TryGetInfo(Handle, out NativeFrame info);
            width = available ? info.Width : 0;
            height = available ? info.Height : 0;
            return available;
        }

        public bool TryCopyFrameBgra(IntPtr destination, int destinationRowBytes, int width, int height)
        {
            return (UFeelStatus)NativeMethods.ufeel_copy_frame_bgra(
                Handle, destination, destinationRowBytes, width, height) == UFeelStatus.Ok;
        }

        public EmotionResult GetEmotions() => emotions.Read(Handle);

        public EyeTrackingResult GetEyeTracking() => eyeTracking.Read(Handle);

        public SpeechToTextResult GetSpeechToText() => speechToText.Read(Handle);

        public HeartRateResult GetHeartRate() => heartRate.Read(Handle);

        public void SetSpeechToTextEnabled(bool enabled)
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_set_speech_to_text_enabled(Handle, enabled ? 1 : 0),
                "change the speech-to-text state");
        }

        /// <summary>Restarts a running capture to rescan the devices. Empty without speech-to-text support.</summary>
        public IReadOnlyList<MicrophoneDevice> GetMicrophones() => microphones.Read(Handle);

        /// <summary>Null selects the default microphone. False when no listed microphone has this id.</summary>
        public bool SelectMicrophone(string? id)
        {
            var status = (UFeelStatus)NativeMethods.ufeel_set_microphone(Handle, id);
            if (status == UFeelStatus.InvalidArgument)
                return false;

            UFeelException.ThrowIfFailed(status, "select the microphone");
            return true;
        }

        /// <summary>True once after a microphone was added or removed, or the system default changed.</summary>
        public bool HaveMicrophonesChanged()
        {
            var status = (UFeelStatus)NativeMethods.ufeel_get_microphones_changed(Handle, out int changed);
            if (status == UFeelStatus.UnsupportedFeature)
                return false;

            UFeelException.ThrowIfFailed(status, "check the microphones");
            return changed != 0;
        }

        public void BeginEyeTrackingCalibration()
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_begin_eye_tracking_calibration(Handle),
                "begin eye-tracking calibration");
        }

        /// <summary>Normalized screen coordinates (0..1).</summary>
        public void SetEyeTrackingCalibrationTarget(float screenX, float screenY)
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_set_eye_tracking_calibration_target(Handle, screenX, screenY),
                "set the eye-tracking calibration target");
        }

        /// <summary>False when the samples were not good enough (see diagnostics).</summary>
        public bool FinishEyeTrackingCalibration()
        {
            var status = (UFeelStatus)NativeMethods.ufeel_finish_eye_tracking_calibration(Handle);
            if (status == UFeelStatus.EyeTrackingCalibrationFailed)
                return false;

            UFeelException.ThrowIfFailed(status, "finish eye-tracking calibration");
            return true;
        }

        public void CancelEyeTrackingCalibration()
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_cancel_eye_tracking_calibration(Handle),
                "cancel eye-tracking calibration");
        }

        public CalibrationProgress GetEyeTrackingCalibrationProgress()
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_get_eye_tracking_calibration_progress(
                    Handle, out NativeEyeTrackingCalibrationProgress progress),
                "retrieve eye-tracking calibration progress");

            return new CalibrationProgress(
                progress.TargetsWithEnoughSamples,
                progress.TargetCount,
                progress.SamplesForCurrentTarget,
                progress.RequiredSamplesPerTarget);
        }

        public CalibrationDiagnostics GetEyeTrackingCalibrationDiagnostics()
        {
            UFeelException.ThrowIfFailed(
                (UFeelStatus)NativeMethods.ufeel_get_eye_tracking_calibration_diagnostics(
                    Handle, out NativeEyeTrackingCalibrationDiagnostics d),
                "retrieve eye-tracking calibration diagnostics");

            return new CalibrationDiagnostics
            {
                FailureReason = (CalibrationFailureReason)d.FailureReason,
                InputTargetCount = d.InputTargetCount,
                TargetsWithEnoughRawSamples = d.TargetsWithEnoughRawSamples,
                TargetsWithEnoughRetainedSamples = d.TargetsWithEnoughRetainedSamples,
                ValidTargetCount = d.ValidTargetCount,
                MinimumRetainedSamples = d.MinimumRetainedSamples,
                MaximumRetainedSamples = d.MaximumRetainedSamples,
                HorizontalSpan = d.HorizontalSpan,
                VerticalSpan = d.VerticalSpan,
                MeanCrossValidationError = d.MeanCrossValidationError,
                MaximumCrossValidationError = d.MaximumCrossValidationError,
                CrossValidationTargetsEvaluated = d.CrossValidationTargetsEvaluated,
                MeanErrorExceeded = d.MeanErrorExceeded != 0,
                MaximumErrorExceeded = d.MaximumErrorExceeded != 0,
                RegressionFailures = d.RegressionFailures,
                InvalidTargetCount = d.InvalidTargetCount,
                TargetsWithInsufficientRawSamples = d.TargetsWithInsufficientRawSamples,
                TargetsWithInsufficientRetainedSamples = d.TargetsWithInsufficientRetainedSamples,
            };
        }

        /// <summary>False when the native side could not write the file.</summary>
        public bool SaveEyeTrackingCalibrationProfile(string path)
        {
            RequirePath(path, nameof(path));
            return ProfileResult(
                (UFeelStatus)NativeMethods.ufeel_save_eye_tracking_calibration_profile(Handle, path),
                "save the eye-tracking calibration profile");
        }

        /// <summary>False when the file is missing or invalid.</summary>
        public bool LoadEyeTrackingCalibrationProfile(string path)
        {
            RequirePath(path, nameof(path));
            return ProfileResult(
                (UFeelStatus)NativeMethods.ufeel_load_eye_tracking_calibration_profile(Handle, path),
                "load the eye-tracking calibration profile");
        }

        public void Dispose()
        {
            heartRate?.Dispose();
            microphones?.Dispose();
            speechToText?.Dispose();
            eyeTracking?.Dispose();
            emotions?.Dispose();

            if (handle != IntPtr.Zero)
            {
                NativeMethods.ufeel_destroy(handle);
                handle = IntPtr.Zero;
            }
        }

        private IntPtr Handle
        {
            get
            {
                if (handle == IntPtr.Zero)
                    throw new ObjectDisposedException(nameof(UFeelProcessor));

                return handle;
            }
        }

        private static bool ProfileResult(UFeelStatus status, string operation)
        {
            if (status == UFeelStatus.EyeTrackingProfileFailed)
                return false;

            UFeelException.ThrowIfFailed(status, operation);
            return true;
        }

        private static void RequirePath(string path, string parameterName)
        {
            if (string.IsNullOrWhiteSpace(path))
                throw new ArgumentException("A non-empty path is required.", parameterName);
        }
    }
}
