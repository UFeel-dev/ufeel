using System.Collections.Generic;
using Unity.Barracuda;
using UnityEngine;
using System.Linq;

namespace MediaPipe.EmotionDetection
{
  public sealed partial class EmotionDetector : System.IDisposable
  {
      #region Public accessors

      public int ImageSize
        => _size;

      #endregion

      #region Public methods

      public EmotionDetector(ResourceSet resources)
        => AllocateObjects(resources);

      public void Dispose()
        => DeallocateObjects();

      public string ProcessImage(Texture image, float threshold)
        => RunModel(image, threshold);

      public ComputeBuffer DetectionBuffer
        => _buffers.post;

      #endregion

      #region Compile-time constants

      const int MaxDetection = 8;

      #endregion

      #region Private objects

      ResourceSet _resources;
      IWorker _worker;
      int _size;

      (ComputeBuffer preprocess,
      ComputeBuffer feature,
      ComputeBuffer post,
      ComputeBuffer counter,
      ComputeBuffer countRead) _buffers;

      readonly static string[] Labels =
        { "Neutral", "Happiness", "Surprise", "Sadness",
          "Anger", "Disgust", "Fear", "Contempt"};

      void AllocateObjects(ResourceSet resources)
      {
          var model = ModelLoader.Load(resources.model);
          _worker = model.CreateWorker();

          _resources = resources;

          _size = model.inputs[0].shape[6];

          _buffers.preprocess = new ComputeBuffer
            (_size * _size * 1, sizeof(float));

          _buffers.post = new ComputeBuffer
            (MaxDetection, Detection.Size, ComputeBufferType.Append);

          _buffers.counter = new ComputeBuffer
            (1, sizeof(uint), ComputeBufferType.Counter);

          _buffers.countRead = new ComputeBuffer
            (1, sizeof(uint), ComputeBufferType.Raw);
      }

      void DeallocateObjects()
      {
          _buffers.preprocess?.Dispose();
          _buffers.preprocess = null;

          _buffers.post?.Dispose();
          _buffers.post = null;

          _buffers.counter?.Dispose();
          _buffers.counter = null;

          _buffers.countRead?.Dispose();
          _buffers.countRead = null;

          _worker?.Dispose();
          _worker = null;
      }

      #endregion

      #region Neural network inference function

      string RunModel(Texture source, float threshold)
      {
          var pre = _resources.preprocess;
          pre.SetInt("_ImageSize", _size);
          pre.SetTexture(0, "_Texture", source);
          pre.SetBuffer(0, "_Tensor", _buffers.preprocess);
          pre.Dispatch(0, _size / 8, _size / 8, 1);

          using (var tensor = new Tensor(1, _size, _size, 1, _buffers.preprocess))
              _worker.Execute(tensor);

          var probs = _worker.PeekOutput().AsFloats().Select(x => Mathf.Exp(x));
          var sum = probs.Sum();
          var lines = Labels.Zip(probs, (l, p) => $"{l,-12}: {p / sum:0.00}");

          Debug.Log(string.Join("\n", lines));

          var normalizedProbs = probs.Select(p => p / sum).ToArray();
          int maxIndex = System.Array.IndexOf(normalizedProbs, normalizedProbs.Max());

          // TODO: put back the actual handling -> remove the next few hardcoded lines, uncomment return

          // if (Labels[1] == "Happiness" && normalizedProbs[1] == 0.02f)
          // {
          //     return "Sadness";
          // }
          // else
          // {
          //     return Labels[1];
          // }

          Debug.Log("Normalized prob of being happy: " + normalizedProbs[1]);

          if (normalizedProbs[1] < 0.0251f)
          {
              return "Sadness";
          }
          else
          {
              return Labels[1];
          }

          // return Labels[maxIndex];
      }

      #endregion

  }
}
