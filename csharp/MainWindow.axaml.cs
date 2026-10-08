using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using System.Runtime.InteropServices;

namespace UFeelTest;

public partial class MainWindow : Window
{
    private readonly UFeelProcessor processor;
    private readonly CancellationTokenSource cancellation = new();

    private bool speechEnabled;

    public MainWindow()
    {
        InitializeComponent();

        string? libraryPath =
            Environment.GetEnvironmentVariable("UFEEL_PACKAGE_PATH");

        if (string.IsNullOrEmpty(libraryPath))
        {
            throw new InvalidOperationException(
                "UFEEL_PACKAGE_PATH is not set.");
        }

        processor = new UFeelProcessor(libraryPath);

        Closed += OnClosed;

        _ = ProcessLoop();
    }

    private async Task ProcessLoop()
    {
        while (!cancellation.IsCancellationRequested)
        {
            try
            {
                if (!processor.Update())
                {
                    await Task.Delay(100, cancellation.Token);
                    continue;
                }

                FrameData? frame = processor.GetFrame();
                Dictionary<string, float> emotions =
                    processor.GetEmotions();
                Dictionary<string, bool> directions =
                    processor.GetDirections();

                string speech = speechEnabled
                    ? processor.GetSpeech()
                    : string.Empty;

                if (frame != null)
                {
                    Bitmap bitmap = CreateBitmap(frame);

                    await Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        Bitmap? old = FrameImage.Source as Bitmap;

                        FrameImage.Source = bitmap;
                        old?.Dispose();

                        EmotionsText.Text =
                            FormatEmotions(emotions);

                        DirectionsText.Text =
                            FormatDirections(directions);

                        SpeechText.Text =
                            string.IsNullOrEmpty(speech)
                                ? "No speech"
                                : speech;
                    });
                }

                await Task.Delay(33, cancellation.Token);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception exception)
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    SpeechText.Text = exception.Message;
                });

                await Task.Delay(500);
            }
        }
    }

    private static Bitmap CreateBitmap(FrameData frame)
    {
        var bitmap = new WriteableBitmap(
            new PixelSize(frame.Width, frame.Height),
            new Vector(96, 96),
            PixelFormat.Rgba8888,
            AlphaFormat.Opaque);

        byte[] rgba = new byte[
            checked(frame.Width * frame.Height * 4)];

        for (int y = 0; y < frame.Height; y++)
        {
            int sourceRow = y * frame.Stride;
            int destinationRow = y * frame.Width * 4;

            for (int x = 0; x < frame.Width; x++)
            {
                int source = sourceRow + x * 3;
                int destination = destinationRow + x * 4;

                rgba[destination] = frame.Data[source + 2];
                rgba[destination + 1] = frame.Data[source + 1];
                rgba[destination + 2] = frame.Data[source];
                rgba[destination + 3] = 255;
            }
        }

        using ILockedFramebuffer locked = bitmap.Lock();

        for (int y = 0; y < frame.Height; y++)
        {
            Marshal.Copy(
                rgba,
                y * frame.Width * 4,
                locked.Address + y * locked.RowBytes,
                frame.Width * 4);
        }

        return bitmap;
    }

    private static string FormatEmotions(
        Dictionary<string, float> emotions)
    {
        if (emotions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            emotions
                .OrderByDescending(pair => pair.Value)
                .Select(pair =>
                    $"{pair.Key}: {pair.Value:F3}"));
    }

    private static string FormatDirections(
        Dictionary<string, bool> directions)
    {
        if (directions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            directions.Select(pair =>
                $"{pair.Key}: {pair.Value}"));
    }

    private void SpeechButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        speechEnabled = !speechEnabled;

        processor.SetSpeechEnabled(speechEnabled);

        SpeechButton.Content = speechEnabled
            ? "Disable speech"
            : "Enable speech";
    }

    private void CalibrateButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        processor.CalibrateDirections();
    }

    private void OnClosed(
        object? sender,
        EventArgs e)
    {
        cancellation.Cancel();
        processor.SetSpeechEnabled(false);
        processor.Dispose();
    }
}
