#nullable enable

namespace UFeel
{
    /// <summary>Called on the thread that started <see cref="EyeTrackingCalibration.RunAsync"/>.</summary>
    public interface ICalibrationPresenter
    {
        void Show();
        void Hide();

        /// <summary>Normalized screen position (0..1, origin top-left).</summary>
        void ShowTarget(float x, float y);

        void ShowInstruction(string text);
        void ShowStatus(string text);
    }
}
