
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media.Imaging;
using Avalonia.Threading;

namespace UFeelTest;

public partial class MainWindow : Window
{
    private readonly UFeelProcessor processor;
    private readonly CancellationTokenSource cancellation = new();
    private readonly SemaphoreSlim processorGate = new(1, 1);

    private readonly Task processTask;
    private bool speechEnabled;
    private bool closing;

    public MainWindow()
    {
        InitializeComponent();

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
            "models",
            "vosk-model-small-fr-0.22");

        if (!Directory.Exists(speechModelPath))
            throw new DirectoryNotFoundException(
                $"Vosk model directory not found: {speechModelPath}");

        processor = new UFeelProcessor(libraryPath, speechModelPath);

        Closed += OnClosed;
        processTask = ProcessLoopAsync(cancellation.Token);
    }

    private async Task ProcessLoopAsync(CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            Bitmap? bitmap = null;

            try
            {
                bool updated;
                Dictionary<string, float> emotions;
                Dictionary<string, bool> directions;
                string speech;

                await processorGate.WaitAsync(token);

                try
                {
                    updated = processor.Update();

                    if (!updated)
                    {
                        emotions = new();
                        directions = new();
                        speech = string.Empty;
                    }
                    else
                    {
                        FrameData? frame = processor.GetFrame();

                        if (frame is not null)
                            bitmap = FrameBitmapFactory.Create(frame);

                        emotions = processor.GetEmotions();
                        directions = processor.GetDirections();
                        speech = speechEnabled
                            ? processor.GetSpeech()
                            : string.Empty;
                    }
                }
                finally
                {
                    processorGate.Release();
                }

                if (!updated)
                {
                    await Task.Delay(50, token);
                    continue;
                }

                Bitmap? nextBitmap = bitmap;
                bitmap = null;

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (closing)
                    {
                        nextBitmap?.Dispose();
                        return;
                    }

                    if (nextBitmap is not null)
                    {
                        Bitmap? oldBitmap = FrameImage.Source as Bitmap;
                        FrameImage.Source = nextBitmap;
                        oldBitmap?.Dispose();
                    }

                    EmotionsText.Text = FormatEmotions(emotions);
                    DirectionsText.Text = FormatDirections(directions);
                    SpeechText.Text = string.IsNullOrWhiteSpace(speech)
                        ? "No speech detected"
                        : speech;
                });

                await Task.Delay(33, token);
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                break;
            }
            catch (Exception exception)
            {
                bitmap?.Dispose();

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (!closing)
                        StatusText.Text = exception.Message;
                });

                try
                {
                    await Task.Delay(500, token);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
            }
            finally
            {
                bitmap?.Dispose();
            }
        }
    }

    private static string FormatEmotions(
        Dictionary<string, float> emotions)
    {
        if (emotions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            emotions.OrderByDescending(pair => pair.Value)
                .Select(pair => $"{pair.Key}: {pair.Value:F3}"));
    }

    private static string FormatDirections(
        Dictionary<string, bool> directions)
    {
        if (directions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            directions.Select(pair => $"{pair.Key}: {pair.Value}"));
    }

    private async void SpeechButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        bool requestedState = !speechEnabled;

        try
        {
            await processorGate.WaitAsync(cancellation.Token);

            try
            {
                processor.SetSpeechEnabled(requestedState);
                speechEnabled = requestedState;
            }
            finally
            {
                processorGate.Release();
            }

            SpeechButton.Content = speechEnabled
                ? "Disable speech"
                : "Enable speech";
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            StatusText.Text = exception.Message;
        }
    }

    private async void CalibrateButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        try
        {
            await processorGate.WaitAsync(cancellation.Token);

            try
            {
                processor.CalibrateDirections();
            }
            finally
            {
                processorGate.Release();
            }

            StatusText.Text = "Eye tracking calibrated";
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            StatusText.Text = exception.Message;
        }
    }

    private async void OnClosed(object? sender, EventArgs e)
    {
        closing = true;
        cancellation.Cancel();

        await processTask;

        try
        {
            await processorGate.WaitAsync();

            try
            {
                processor.SetSpeechEnabled(false);
                processor.Dispose();
            }
            finally
            {
                processorGate.Release();
            }
        }
        finally
        {
            (FrameImage.Source as Bitmap)?.Dispose();
            FrameImage.Source = null;

            processorGate.Dispose();
            cancellation.Dispose();
        }
    }
}
