#nullable enable

using System;

namespace UFeel.Native
{
    internal sealed class NativeResultBuffer : IDisposable
    {
        private readonly Action<IntPtr> free;

        public NativeResultBuffer(Func<IntPtr> create, Action<IntPtr> free, string name)
        {
            this.free = free;
            Pointer = create();

            if (Pointer == IntPtr.Zero)
                throw new InvalidOperationException($"Failed to allocate the native {name} result.");
        }

        public IntPtr Pointer { get; private set; }

        public void Dispose()
        {
            if (Pointer == IntPtr.Zero)
                return;

            free(Pointer);
            Pointer = IntPtr.Zero;
        }
    }
}
