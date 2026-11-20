using System;
using System.IO;
using System.Runtime.InteropServices;
using UFeel;

namespace UFeelTest;

internal static class NativeLibraryLoader
{
    private const string LibraryName = "ufeel_wrapper";

    public static void Use(string libraryPath)
    {
        string fullPath = Path.GetFullPath(libraryPath);

        NativeLibrary.SetDllImportResolver(
            typeof(UFeelProcessor).Assembly,
            (name, _, _) => string.Equals(name, LibraryName, StringComparison.Ordinal)
                ? NativeLibrary.Load(fullPath)
                : IntPtr.Zero);
    }
}
