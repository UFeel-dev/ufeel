using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia.Controls;
using Avalonia.Interactivity;
using UFeel;

namespace UFeelTest;

public partial class MainWindow
{
    private bool showingMicrophones;

    private void RefreshMicrophonesButton_OnClick(object? sender, RoutedEventArgs e) => RefreshMicrophones();

    private async void RefreshMicrophones()
    {
        try
        {
            ShowMicrophones(await session.GetMicrophonesAsync());
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            ShowIdleStatus(exception.Message);
        }
    }

    private void ShowMicrophones(IReadOnlyList<MicrophoneDevice> microphones)
    {
        var items = new List<ComboBoxItem> { new() { Content = UFeelText.DefaultMicrophone(microphones) } };
        items.AddRange(microphones.Select(microphone => new ComboBoxItem { Content = microphone.Name, Tag = microphone.Name }));

        showingMicrophones = true;
        MicrophoneSelector.ItemsSource = items;
        MicrophoneSelector.SelectedItem =
            items.FirstOrDefault(item => item.Tag is string name && name == session.SelectedMicrophone) ?? items[0];
        showingMicrophones = false;
    }

    private async void MicrophoneSelector_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (showingMicrophones || MicrophoneSelector.SelectedItem is not ComboBoxItem item)
            return;

        string? name = item.Tag as string;
        if (name == session.SelectedMicrophone)
            return;

        try
        {
            if (!await session.SelectMicrophoneAsync(name))
            {
                ShowIdleStatus($"Microphone \"{name}\" is no longer available");
                RefreshMicrophones();
            }
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            ShowIdleStatus(exception.Message);
        }
    }
}
