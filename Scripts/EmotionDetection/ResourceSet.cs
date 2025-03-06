using UnityEngine;
using Unity.Barracuda;

namespace MediaPipe.EmotionDetection
{
    [CreateAssetMenu(fileName = "EmotionDetection",
                    menuName = "ScriptableObjects/MediaPipe/EmotionDetection Resource Set")]
    public sealed class ResourceSet : ScriptableObject
    {
        public NNModel model;
        public ComputeShader preprocess;
        public ComputeShader postprocess;
    }
}
