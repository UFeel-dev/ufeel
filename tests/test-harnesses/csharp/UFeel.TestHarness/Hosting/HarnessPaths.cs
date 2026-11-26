using System;
using System.IO;
using UFeel;

namespace UFeelTest;

/// <summary>Package locations, from UFEEL_PACKAGE_PATH (path to libufeel_wrapper.so or .dylib, set by build.sh).</summary>
internal sealed record HarnessPaths(string LibraryPath, UFeelPackageLayout Layout)
{
    public static HarnessPaths FromEnvironment()
    {
        string? libraryPath = Environment.GetEnvironmentVariable("UFEEL_PACKAGE_PATH");
        if (string.IsNullOrWhiteSpace(libraryPath))
            throw new InvalidOperationException("UFEEL_PACKAGE_PATH is not set.");

        libraryPath = Path.GetFullPath(libraryPath);
        string packageDirectory = Path.GetDirectoryName(libraryPath)
            ?? throw new InvalidOperationException("Cannot determine the package directory.");

        return new HarnessPaths(libraryPath, UFeelPackageLayout.FromRoot(packageDirectory));
    }
}
