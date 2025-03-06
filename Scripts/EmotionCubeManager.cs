using UnityEngine;
using UnityEngine.UI;
using MediaPipe.EmotionDetection;

public class MediaPipeEmotionVisualizer : MonoBehaviour
{
    #region Editable Attributes

    [SerializeField] private WebcamInput _webcam = null;
    // [SerializeField] private RawImage _previewUI = null;
    [Space]
    [SerializeField] private ResourceSet _resources = null;
    [SerializeField] private Shader _shader = null;
    [SerializeField, Range(0, 1)] private float _threshold = 0.5f;

    [SerializeField] private CubeColorChanger cubeColorChanger = null;

    #endregion

    #region Private Members

    private EmotionDetector _detector;
    private Material _material;

    #endregion

    #region MonoBehaviour Implementation

    void Start()
    {
        _detector = new EmotionDetector(_resources);
        _material = new Material(_shader);
    }

    void LateUpdate()
    {
        string detectedEmotion = _detector.ProcessImage(_webcam.Texture, _threshold);

        if (cubeColorChanger != null)
        {
            cubeColorChanger.SetColor(detectedEmotion);
        }

        // _previewUI.texture = _webcam.Texture;
    }

    void OnDestroy()
    {
        if (_detector != null)
        {
            _detector.Dispose();
        }
        if (_material != null)
        {
            Destroy(_material);
        }
    }

    #endregion
}
