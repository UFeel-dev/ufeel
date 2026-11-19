#nullable enable

using System;
using System.IO;

namespace UFeel
{
    public enum ProfileLoadResult
    {
        Missing,
        Loaded,
        Invalid,
    }

    public sealed class CalibrationProfileStore
    {
        public const string FileName = "eyeTracking-profile-v3.txt";

        public CalibrationProfileStore(string path)
        {
            if (string.IsNullOrWhiteSpace(path))
                throw new ArgumentException("A profile path is required.", nameof(path));

            Path = System.IO.Path.GetFullPath(path);
        }

        public string Path { get; }

        /// <summary>Shared by every UFeel host (~/.local/share/UFeel on Linux).</summary>
        public static CalibrationProfileStore CreateDefault()
        {
            string directory = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            if (string.IsNullOrWhiteSpace(directory))
                throw new InvalidOperationException("Cannot determine the per-user application data directory.");

            return new CalibrationProfileStore(System.IO.Path.Combine(directory, "UFeel", FileName));
        }

        public ProfileLoadResult Load(UFeelProcessor processor)
        {
            if (processor is null)
                throw new ArgumentNullException(nameof(processor));

            if (!File.Exists(Path))
                return ProfileLoadResult.Missing;

            return processor.LoadEyeTrackingCalibrationProfile(Path)
                ? ProfileLoadResult.Loaded
                : ProfileLoadResult.Invalid;
        }

        public bool Save(UFeelProcessor processor)
        {
            if (processor is null)
                throw new ArgumentNullException(nameof(processor));

            string? directory = System.IO.Path.GetDirectoryName(Path);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);

            return processor.SaveEyeTrackingCalibrationProfile(Path);
        }
    }
}
