/*
** EPITECH PROJECT, 2026
** lib_ufeel
** File description:
** Program
*/

using System;
using System.Runtime.InteropServices;

class Program
{
    static void Main()
    {
        IntPtr processor = UFeelNative.ufeel_create();
        Console.WriteLine("Coucou");

        while (true)
        {
            IntPtr frame = UFeelNative.ufeel_debug_get_frame(processor);

            if (frame == IntPtr.Zero)
                continue;

            // DetectEmotion(processor, frame);
            DetectEyeTracking(processor, frame);

            UFeelNative.ufeel_debug_destroy_frame(frame);

            int key = UFeelNative.ufeel_debug_wait_key(1);
            if (key == 27 || key == 'q')
                break;
        }
        UFeelNative.ufeel_destroy(processor);
    }

    static void DetectEmotion(IntPtr processor, IntPtr frame)
    {
        var emotions = UFeelNative.ufeel_get_emotions(processor, out int size);

        var dict = new Dictionary<string, float>();

        int structSize = Marshal.SizeOf<UFeelNative.UFeelPair>();

        for (int i = 0; i < size; i++)
        {
            IntPtr itemPtr = emotions + i * structSize;

            UFeelNative.UFeelPair pair = Marshal.PtrToStructure<UFeelNative.UFeelPair>(itemPtr);


            string? key = Marshal.PtrToStringAnsi(pair.key);

            if (string.IsNullOrEmpty(key))
            {
                Console.WriteLine("Error key shouldn't be null");
                return;
            }

            dict[key] = pair.value;
        }

        UFeelNative.ufeel_debug_show_emotions(frame, emotions, size);
        UFeelNative.ufeel_free_emotions(emotions, size);
        return;
    }

    static void DetectEyeTracking(IntPtr processor, IntPtr frame)
    {
        var directions = UFeelNative.ufeel_get_directions(processor, out int size);

        var dict = new Dictionary<string, bool>();

        int structSize = Marshal.SizeOf<UFeelNative.UFeelBoolPair>();

        for (int i = 0; i < size; i++)
        {
            IntPtr itemPtr = directions + i * structSize;

            UFeelNative.UFeelBoolPair pair = Marshal.PtrToStructure<UFeelNative.UFeelBoolPair>(itemPtr);


            string? key = Marshal.PtrToStringAnsi(pair.key);

            if (string.IsNullOrEmpty(key))
            {
                Console.WriteLine("Error key shouldn't be null");
                return;
            }

            dict[key] = pair.value;
        }

        // UFeelNative.ufeel_debug_show_directions(frame, directions, size);
        UFeelNative.ufeel_free_directions(directions, size);
        return;
    }
}