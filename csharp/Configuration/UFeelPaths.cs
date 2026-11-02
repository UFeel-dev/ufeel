using System;
using System.IO;

namespace UFeelTest;

internal sealed record UFeelPaths(
    string LibraryPath,
    string ModelDirectory,
    string SpeechModelPath,
    string CalibrationProfilePath)
{
    public static UFeelPaths Resolve()
    {
        string? libraryPath =
            Environment.GetEnvironmentVariable("UFEEL_PACKAGE_PATH");

        if (string.IsNullOrWhiteSpace(libraryPath))
            throw new InvalidOperationException(
                "UFEEL_PACKAGE_PATH is not set.");

        libraryPath = Path.GetFullPath(libraryPath);

        string packageDirectory =
            Path.GetDirectoryName(libraryPath)
            ?? throw new InvalidOperationException(
                "Cannot determine the package directory.");

        string speechModelPath = Path.Combine(
            packageDirectory,
            "models/SpeechToText/",
            "vosk-model-small-fr-0.22");
            // "vosk-model-small-en-us-0.15");
            // "vosk-model-small-de-0.15");
        string modelDirectory = Path.Combine(packageDirectory, "models");

        if (!Directory.Exists(modelDirectory))
            throw new DirectoryNotFoundException(
                $"Model directory not found: {modelDirectory}");
        if (!Directory.Exists(speechModelPath))
            throw new DirectoryNotFoundException(
                $"Vosk model directory not found: {speechModelPath}");

        string localDataDirectory = Environment.GetFolderPath(
            Environment.SpecialFolder.LocalApplicationData);
        if (string.IsNullOrWhiteSpace(localDataDirectory))
            throw new InvalidOperationException(
                "Cannot determine the per-user application data directory for the eyeTracking profile.");

        string calibrationProfilePath = Path.Combine(
            localDataDirectory,
            "UFeel",
            "eyeTracking-profile-v3.txt");

        return new UFeelPaths(
            libraryPath,
            modelDirectory,
            speechModelPath,
            calibrationProfilePath);
    }
}
