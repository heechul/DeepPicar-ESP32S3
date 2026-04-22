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

See [BUILD.md](./BUILD.md) for ESP32-S3 and motor driver connection details.

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

### Examples

Basic manual control:
```bash
python deeppicar.py
```

Enable DNN inference at 50% throttle with 40Hz control frequency:
```bash
python deeppicar.py -d -t 50 -f 40
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
| `n` | On-device DNN |

### Data Recording

Press `r` to start/stop recording. The system records for up to 1000 frames then auto-stops. Output files:
- `out-video.avi` - Video recording
- `out-key.csv` - Timestamped key/steering data

If you want to use Google Cola, compress all the recorded files into a single zip file, say Dataset.zip for Colab.

```
$ zip Dataset.zip out-*
updating: out-key.csv (deflated 81%)
updating: out-video.avi (deflated 3%)
```

If you train locally, put all the recorded video and csv files on the 'dataset' folder. 

## Training

Use `RunAll-v2.ipynb` to train custom models. 

Training notebook includes:
- Data loading and preprocessing
- Model architecture selection
- Quantization for embedded deployment
- Model conversion to TFLite format

If you use Colab, upload a zip file (e.g., Dataset.zip you created above), which will be used to train the model. 

## Deployment to ESP32S3

Use PlatformIO. [TBD]

## Models

Pre-trained models available in `models/` directory:

| Model | Resolution | Quantization | Use Case |
|-------|-----------|--------------|----------|
| opt-k2-160x60x3-T1 | 160×60×3 | int8 | Balanced accuracy/speed |
| opt-k2-160x120x3-T1 | 160×120×3 | int8 | High accuracy |
| opt-k2-80x60x3-T1 | 80×60×3 | int8 | High speed, edge devices |
| opt-k2-40x30x3-T1 | 40×30×3 | int8 | Ultra-low latency |
| pilotnet-dg-160x60x3-T1-r1.0 | 160×60×3 | int8 | DG-tuned model (devel) |

Model files:
- `.tflite` - TensorFlow Lite format (recommended)
- `.h5` - Keras/TensorFlow format
- `.cc` - C++ source for embedded systems

## Troubleshooting

### Hardware Connection

**Error: Cannot open camera**
- Verify ESP32 URL in `params.py` (default: http://192.168.4.1)
- Check Wi-Fi connection
- Use `camera-null.py` for testing without camera hardware

**No actuator response**
- Verify motor driver connections (see [BUILD.md](./BUILD.md))
- Use `actuator-null.py` for testing without motor driver hardware

### Performance

**High latency / dropped frames**
- Reduce image resolution with smaller model
- Lower control frequency with `-f` option
- Use smaller model (e.g., 80×60 instead of 160×120)
- Enable OpenVINO for GPU acceleration

**Poor inference accuracy**
- Verify lighting conditions match training data
- Retrain model with domain-specific data

## Model Viewer

Visualize and analyze TFLite models:
- Online: https://netron.app (upload .tflite file)
- Inspect layer structure, weights, and operations

## Advanced Topics

### Extending to New Hardware

1. Create new camera module: `camera-{name}.py`
2. Create new actuator module: `actuator-{name}.py`
3. Update `params.py` to reference new modules
4. Implement init(), read_frame()/set_control(), stop() functions

## References

- TensorFlow Lite: https://www.tensorflow.org/lite
- OpenVINO: https://github.com/openvinotoolkit/openvino
- Netron Model Viewer: https://netron.app
