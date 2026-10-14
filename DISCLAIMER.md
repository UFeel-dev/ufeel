# Disclaimer

> [← Back to README](README.md)

## Scope

This project provides software tools and libraries for advanced input systems, including but not limited to:

- Eye tracking
- Emotion recognition
- Voice and audio-based input
- Heart rate processing

This list is not exhaustive and may be updated in the future.

System performance depends on external hardware, software configuration, and environmental conditions.

## Hardware Dependency

All input data is captured using third-party hardware. This includes, but is not limited to:

- Microphones
- Cameras
- Heart rate sensors

This list is not exhaustive.

The project does not design, manufacture, calibrate, or control such hardware. Hardware quality, configuration, and environment directly affect system behavior.

As a result:

- Different devices may produce materially different results
- Low-quality, misconfigured, or poorly calibrated hardware may reduce accuracy
- Environmental factors may introduce noise, latency, distortion, or bias

These effects are inherent to the hardware and environment and do not constitute software defects.

## Examples of Hardware and Environmental Impact

Non-exhaustive examples include:

- Voice recognition accuracy affected by microphone quality, background noise, compression, and sampling rate
- Eye tracking accuracy affected by camera resolution, frame rate, lighting, positioning, and user physiology
- Emotion recognition affected by image quality, occlusions, lighting, facial visibility, and individual or cultural variation
- Physiological signal processing affected by sensor precision, skin contact, motion artifacts, and calibration

This list is not exhaustive.

## No Guarantee of Accuracy or Fitness

The project provides no guarantee of:

- Detection correctness
- Real-time performance
- Consistency across devices
- Freedom from bias or error
- Suitability for medical, psychological, diagnostic, legal, or regulatory purposes

All outputs must be treated as approximate signals and probabilistic estimates, not authoritative measurements.

## Prohibited and High-Risk Use

This project must not be used in:

- Life-threatening or safety-critical systems
- Military or weapons-related applications
- Medical diagnosis, treatment, or monitoring
- Emergency response systems
- Systems where failure could cause physical harm, significant financial loss, or legal consequences

This list is not exhaustive.

If uncertain, contact the project maintainers before proceeding.

## Responsibility of Integrators and Users

Anyone integrating or deploying this project is responsible for:

- Selecting appropriate hardware
- Validating performance on target devices
- Assessing risks associated with incorrect or degraded input
- Implementing safeguards, fallbacks, and human oversight where appropriate
- Informing end users about limitations and potential inaccuracies

This project does not assume responsibility for consequences arising from improper deployment, misuse, or reliance on system outputs.

## Intended Use

The project is intended for:

- Research
- Prototyping
- Experimental interaction systems
- Non-critical software features

This list is not exhaustive.

If uncertain about your intended use, contact the project maintainers.

## Acknowledgment of Limitations

Variability in results is expected. Performance depends on hardware quality, environmental conditions, and implementation decisions made by integrators.

By using this project, you acknowledge these limitations and accept responsibility for evaluating suitability within your specific context.
