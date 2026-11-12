using System;

namespace UFeelTest;

internal static class Program
{
    public static void Main(string[] args)
    {
        if (args.Length != 2 ||
            !string.Equals(args[0], "--feature-test", StringComparison.Ordinal))
        {
            throw new ArgumentException(
                "Expected '--feature-test <all|feature-name>'.");
        }

        Environment.ExitCode = FeatureTestRunner.Run(args[1]);
    }
}
