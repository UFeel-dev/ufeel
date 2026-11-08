namespace UFeel;

public sealed class FrameData
{
    public byte[] Data { get; }
    public int Width { get; }
    public int Height { get; }
    public int Channels { get; }
    public int Stride { get; }

    public FrameData(
        byte[] data, int width, int height, int channels, int stride)
    {
        Data = data;
        Width = width;
        Height = height;
        Channels = channels;
        Stride = stride;
    }
}
