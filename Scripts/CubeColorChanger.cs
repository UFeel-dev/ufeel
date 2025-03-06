using UnityEngine;
using System.Collections.Generic;

public class CubeColorChanger : MonoBehaviour
{
    private readonly Dictionary<string, Color> emotionColors = new Dictionary<string, Color>()
    {
        {"Neutral", Color.white},
        {"Happiness", Color.yellow},
        {"Surprise", Color.cyan},
        {"Sadness", Color.blue},
        {"Anger", Color.red},
        {"Disgust", Color.green},
        {"Fear", Color.magenta},
        {"Contempt", new Color(0.5f, 0f, 0.5f)}
    };

    public void SetColor(string emotion)
    {
        if (emotionColors.TryGetValue(emotion, out Color newColor))
        {
            GetComponent<Renderer>().material.color = newColor;
            Debug.Log($"Cube color changed to {newColor} for emotion: {emotion}");
        }
        else
        {
            Debug.LogWarning($"Unrecognized emotion: {emotion}. Defaulting to black.");
            GetComponent<Renderer>().material.color = Color.black;
        }
    }
}
