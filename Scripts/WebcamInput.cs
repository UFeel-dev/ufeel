using System.Linq;
using UnityEngine;
using UnityEngine.UI;

public sealed class WebcamInput : MonoBehaviour
{
    #region Editable attributes

    [SerializeField] string _deviceName = "";
    [SerializeField] Vector2Int _resolution = new Vector2Int(1920, 1080);
    [SerializeField] Texture2D _dummyImage = null;

    [SerializeField] RawImage _previewUI = null;

    #endregion

    #region Internal objects

    WebCamTexture _webcam;
    RenderTexture _buffer;

    #endregion

    #region Public properties

    public Texture Texture
      => _dummyImage != null ? (Texture)_dummyImage : (Texture)_buffer;

    #endregion

    #region MonoBehaviour implementation

    void Start()
    {
        string[] devices = WebCamTexture.devices.Select(device => device.name).ToArray();
        Debug.Log("Available Webcam Devices: " + string.Join(", ", devices));

        if (string.IsNullOrEmpty(_deviceName))
        {
            _deviceName = devices.FirstOrDefault();
        }

        Debug.Log("Selected camera: " + _deviceName);

        _webcam = new WebCamTexture(_deviceName, _resolution.x, _resolution.y);
        _buffer = new RenderTexture(_resolution.x, _resolution.y, 0);
        _webcam.Play();

        if (_webcam.isPlaying)
        {
            Debug.Log("Webcam is playing.");
        }
        else
        {
            Debug.Log("Webcam failed to start.");
        }

        if (_previewUI != null)
        {
            _previewUI.texture = _buffer;
        }
    }


    void OnDestroy()
    {
        if (_webcam != null) Destroy(_webcam);
        if (_buffer != null) Destroy(_buffer);
    }

    void Update()
    {
        if (_dummyImage != null) return;
        if (!_webcam.didUpdateThisFrame) return;

        var aspect1 = (float)_webcam.width / _webcam.height;
        var aspect2 = (float)_resolution.x / _resolution.y;
        var gap = aspect2 / aspect1;

        var vflip = _webcam.videoVerticallyMirrored;
        var scale = new Vector2(gap, vflip ? -1 : 1);
        var offset = new Vector2((1 - gap) / 2, vflip ? 1 : 0);

        Graphics.Blit(_webcam, _buffer, scale, offset);
    }

    #endregion
}
