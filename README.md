# DeepPicar-ESP32S3

Autonomous driving platform based on ESP32-S3 with TensorFlow Lite neural networks. Supports on-device inference with multiple inference engines (TFLite, TensorFlow, OpenVINO).

## Installation

### Prerequisites
- Python 3.8+ (tested with 3.12)
- ESP32-S3 board (hardware control)
- Virtual environment (recommended)

### Setup

1. Clone the repository:
```bash
git clone <repository-url>
cd DeepPicar-ESP32S3
```

2. Create and activate virtual environment:
```bash
python -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate
```

3. Install dependencies:
```bash
pip install -r requirements.txt
```

## Hardware Setup

See [BUILD.md](src/BUILD.md) for ESP32-S3 and motor driver connection details.

## Driving

Launch `deeppicar.py` to control DeepPicar:

```bash
python deeppicar.py [options]
```

### Command-Line Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `-d, --dnn` | flag | False | Enable DNN inference |
| `-p, --prob_dnn` | float | 1.0 | Probability of using DNN (0.0-1.0) |
| `-t, --throttle` | int | 0 | Throttle percentage (0-100) |
| `--turnthresh` | int | 10 | Turn angle threshold (0-30 degrees) |
| `-n, --ncpu` | int | 2 | Number of CPU cores for inference |
| `-f, --hz` | int | 20 | Control frequency (Hz) |
| `--fpvvideo` | flag | False | Record FPV video of DNN driving |
| `--use` | str | tflite | Inference engine: tflite, tf, or openvino |
| `--pre` | str | resize | Preprocessing: resize or crop |
| `--int8` | flag | True | Use int8 quantized model |
| `--use_LET` | flag | False | Use LiteRT interpreter |

### Examples

Basic manual control:
```bash
python deeppicar.py
```

Enable DNN inference at 50% throttle with 40Hz control frequency:
```bash
python deeppicar.py -d -t 50 -f 40
```

Use OpenVINO with CPU cores optimization:
```bash
python deeppicar.py -d --use openvino -n 4
```

### Runtime Controls

At runtime, press keys to control the vehicle:

| Key | Action |
|-----|--------|
| `j` | Left turn |
| `l` | Right turn |
| `k` | Center/straight |
| `a` | Acceleration (forward) |
| `z` | Reverse |
| `s` | Stop |
| `d` | Toggle DNN enable/disable |
| `r` | Toggle data recording |
| `q` | Quit |

### Data Recording

Press `r` to start/stop recording. The system records for up to 1000 frames then auto-stops. Output files:
- `out-video.avi` - Video recording
- `out-key.csv` - Timestamped key/steering data

## Training

Use `RunAll-v2.ipynb` to train custom models. Customize model dimensions in params.py:

```python
img_width = 160      # Image width (pixels)
img_height = 60      # Image height (pixels)
img_channels = 3     # Color channels (1=grayscale, 3=RGB)
temporal_context = 1 # Temporal frames (for LSTM models)
```

Training notebook includes:
- Data loading and preprocessing
- Model architecture selection (CNN, ResNet, MobileNet)
- Quantization for embedded deployment
- Model conversion to TFLite format

## Models

Pre-trained models available in `models/` directory:

| Model | Resolution | Quantization | Use Case |
|-------|-----------|--------------|----------|
| opt-k2-160x60x3-T1 | 160×60×3 | int8 | Balanced accuracy/speed |
| opt-k2-160x120x3-T1 | 160×120×3 | int8 | High accuracy |
| opt-k2-80x60x3-T1 | 80×60×3 | int8 | High speed, edge devices |
| opt-160x60x1 | 160×60×1 | int8 | Grayscale, minimal compute |
| opt-k2-40x30x3-T1 | 40×30×3 | int8 | Ultra-low latency |
| pilotnet-dg-160x60x3-T1-r1.0 | 160×60×3 | int8 | DG-tuned model (devel) |

Model files:
- `.tflite` - TensorFlow Lite format (recommended)
- `.h5` - Keras/TensorFlow format
- `.cc` - C++ source for embedded systems

### Model Selection

Select model in `params.py`:
```python
model_name = "opt-k2"  # Model family
# Model file auto-resolves to: models/opt-k2-{width}x{height}x{channels}-T{temporal}.tflite
```

## Inference Engines

### TFLite (Default)
- Best compatibility, smallest binary
- CPU acceleration via XNNPACK
- Fallback if tflite_runtime unavailable

### TensorFlow
- Full TF features, larger overhead
- Use with `--use tf`
- Good for complex post-processing

### OpenVINO (devel branch)
- Intel optimized inference
- Multi-platform support (CPU, GPU, VPU)
- Use with `--use openvino`

## Troubleshooting

### Model Loading Issues

**Error: "No module named 'tensorflow'"**
```bash
pip install tensorflow
```

### Hardware Connection

**Error: Cannot open camera**
- Verify ESP32 URL in `params.py` (default: http://192.168.4.1)
- Check Wi-Fi connection
- Use `camera-null.py` for testing without camera hardware

**No actuator response**
- Verify motor driver connections (see [BUILD.md](src/BUILD.md))
- Use `actuator-null.py` for testing without motor driver hardware

### Performance

**High latency / dropped frames**
- Reduce image resolution with smaller model
- Lower control frequency with `-f` option
- Use smaller model (e.g., 80×60 instead of 160×120)
- Enable OpenVINO for GPU acceleration

**Poor inference accuracy**
- Verify lighting conditions match training data
- Check `--pre crop` vs `--pre resize` preprocessing
- Retrain model with domain-specific data

## Model Viewer

Visualize and analyze TFLite models:
- Online: https://netron.app (upload .tflite file)
- Inspect layer structure, weights, and operations

## Advanced Topics

### int8 vs float Quantization

**int8 (Quantized)**
- Pros: 4× smaller, 2-4× faster, less memory
- Cons: Slightly reduced accuracy (typically <1%)
- Recommended for embedded/edge devices

**float32**
- Pros: Maximum accuracy, no quantization loss
- Cons: 4× larger, slower inference
- Use for research/development

Switch in `params.py` or model selection.

### Custom Preprocessing

Edit `get_image_resize()` or `get_image_crop()` functions in `deeppicar.py` for:
- Histogram equalization
- Noise reduction
- Augmentation

### Extending to New Hardware

1. Create new camera module: `camera-{name}.py`
2. Create new actuator module: `actuator-{name}.py`
3. Update `params.py` to reference new modules
4. Implement init(), read_frame()/set_control(), stop() functions

## References

- TensorFlow Lite: https://www.tensorflow.org/lite
- OpenVINO: https://github.com/openvinotoolkit/openvino
- Netron Model Viewer: https://netron.app