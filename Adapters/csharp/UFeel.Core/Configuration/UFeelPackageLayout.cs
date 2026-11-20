#nullable enable

using System;
using System.IO;
using System.Linq;

namespace UFeel
{
    public enum SpeechLanguage
    {
        French,
        English,
        German,
    }

    public sealed class UFeelPackageLayout
    {
        private UFeelPackageLayout(string rootDirectory, string modelDirectory, string speechModelPath)
        {
            RootDirectory = rootDirectory;
            ModelDirectory = modelDirectory;
            SpeechModelPath = speechModelPath;
        }

        public string RootDirectory { get; }

        /// <summary>The root itself when the package has no models/ (e.g. heart-rate only).</summary>
        public string ModelDirectory { get; }

        /// <summary>Falls back to <see cref="ModelDirectory"/>: the native side needs a non-empty path even without speech. (note: this is a workaround and will be removed in the future)</summary>
        public string SpeechModelPath { get; }

        public static UFeelPackageLayout FromRoot(string rootDirectory, SpeechLanguage language = SpeechLanguage.French)
        {
            if (string.IsNullOrWhiteSpace(rootDirectory))
                throw new ArgumentException("A package directory is required.", nameof(rootDirectory));

            string root = Path.GetFullPath(rootDirectory);
            if (!Directory.Exists(root))
                throw new DirectoryNotFoundException($"UFeel package directory not found: {root}");

            string models = Path.Combine(root, "models");
            if (!Directory.Exists(models))
                models = root;

            return new UFeelPackageLayout(root, models, FindSpeechModel(models, language) ?? models);
        }

        private static string? FindSpeechModel(string modelDirectory, SpeechLanguage language)
        {
            string speechDirectory = Path.Combine(modelDirectory, "SpeechToText");
            if (!Directory.Exists(speechDirectory))
                return null;

            // e.g. vosk-model-small-fr-0.22, vosk-model-small-en-us-0.15
            return Directory
                .GetDirectories(speechDirectory, $"vosk-model-*-{LanguageCode(language)}-*")
                .OrderBy(path => path, StringComparer.Ordinal)
                .LastOrDefault();
        }

        private static string LanguageCode(SpeechLanguage language)
        {
            return language switch
            {
                SpeechLanguage.French => "fr",
                SpeechLanguage.English => "en",
                SpeechLanguage.German => "de",
                _ => throw new ArgumentOutOfRangeException(nameof(language), language, null),
            };
        }
    }
}
