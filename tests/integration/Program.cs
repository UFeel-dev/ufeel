using System;
using Avalonia;

namespace UFeelTest;

class Program
{
    public static void Main(string[] args)
    {
        if (args.Length == 2 &&
            string.Equals(args[0], "--feature-test", StringComparison.Ordinal))
        {
            Environment.ExitCode = FeatureTestRunner.Run(args[1]);
            return;
        }

        BuildAvaloniaApp()
            .StartWithClassicDesktopLifetime(args);
    }

    public static AppBuilder BuildAvaloniaApp()
    {
        return AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .UseWaylandWithFallback()
            .LogToTrace();
    }
}
