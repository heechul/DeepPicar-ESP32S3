# Build instruction

## Components

- DRV8835 Motor Driver IC
- ESP32S3 MCU
- Bread board jumper wires

## Pinouts 

### DRV8835
![drv8835-pinout.jpg](./docs/drv8835-pinout.jpg)

### ESP32S3
![esp32s3-pinout.jpg](./docs/esp32s3-pinout.jpg)

## Connection guideline

```
    ESP32S3                 DRV8835
    GND             -->     GND             # Common ground
    3V3             -->     3V3             # 3.3V power to the motor driver IC
    D0 (GPIO1)      -->     GPIO5           # Left/Right direction
    D1 (GPIO2)      -->     GPIO6           # Forward/Reverse direction
    D2 (GPIO3)      -->     GPIO12          # Steering PWM
    D3 (GPIO4)      -->     GPIO13          # Throttle PWM
```

## References

- DRV8835 documentation: https://www.pololu.com/product/2753
- ESP32S3 documentation: https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/

