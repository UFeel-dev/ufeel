#nullable enable

namespace UFeel
{
    public sealed class CalibrationResult
    {
        private CalibrationResult(bool succeeded, bool cancelled, bool profileSaved, CalibrationDiagnostics? diagnostics, string message)
        {
            Succeeded = succeeded;
            Cancelled = cancelled;
            ProfileSaved = profileSaved;
            Diagnostics = diagnostics;
            Message = message;
        }

        public bool Succeeded { get; }
        public bool Cancelled { get; }
        public bool ProfileSaved { get; }

        /// <summary>Set when the native fit failed.</summary>
        public CalibrationDiagnostics? Diagnostics { get; }

        public string Message { get; }

        internal static CalibrationResult Success(bool profileSaved, bool hasProfileStore)
        {
            string message = !hasProfileStore ? "Eye tracking calibrated"
                : profileSaved ? "Eye tracking calibrated and profile saved"
                : "Eye tracking calibrated, but the profile could not be saved";

            return new CalibrationResult(true, false, profileSaved, null, message);
        }

        internal static CalibrationResult Failure(CalibrationDiagnostics diagnostics)
        {
            return new CalibrationResult(false, false, false, diagnostics, UFeelText.CalibrationFailure(diagnostics));
        }

        internal static CalibrationResult Cancel()
        {
            return new CalibrationResult(false, true, false, null, "Eye tracking calibration cancelled");
        }
    }
}
