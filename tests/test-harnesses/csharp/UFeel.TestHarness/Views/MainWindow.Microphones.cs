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
        var items = new List<ListBoxItem> { new() { Content = UFeelText.DefaultMicrophone(microphones) } };
        items.AddRange(microphones.Select(microphone => new ListBoxItem { Content = microphone.Name, Tag = microphone.Id }));

        showingMicrophones = true;
        MicrophoneList.ItemsSource = items;
        MicrophoneList.SelectedItem =
            items.FirstOrDefault(item => item.Tag is string id && id == session.SelectedMicrophoneId) ?? items[0];
        showingMicrophones = false;
    }

    private async void MicrophoneList_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (showingMicrophones || MicrophoneList.SelectedItem is not ListBoxItem item)
            return;

        string? id = item.Tag as string;
        if (id == session.SelectedMicrophoneId)
            return;

        try
        {
            if (!await session.SelectMicrophoneAsync(id))
            {
                ShowIdleStatus($"Microphone \"{item.Content}\" is no longer available");
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
