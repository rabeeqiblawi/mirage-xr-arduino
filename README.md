# MirageXR Arduino Library

MirageXR is an Arduino library designed to interface with the [MirageXR Electrobits Core Unity package](https://github.com/rabeeqiblawi/miragexr_electrobits_core_unity). It enables seamless communication between Unity-based virtual electronics and real hardware, allowing you to control and monitor Arduino pins from your Unity applications in real time.

## Features

- Exposes digital, analog, and text-based pin values to Unity.
- Supports up to 24 virtual pins.
- Real-time bidirectional communication using WebSockets.
- Simple API for pin mode, digital/analog/text read/write.
- Broadcasts pin states to all connected Unity clients.

## Requirements

This library depends on the following Arduino libraries:
```cpp
#include "Arduino.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
```

Make sure to install these libraries via the Arduino Library Manager or PlatformIO before using MirageXR.

## Usage

1. Clone or download this repository into your Arduino `libraries` folder.
2. Include `miragexr.h` in your Arduino sketch.
3. Instantiate the `MirageXR` class with your WiFi credentials and desired port.
4. Call `start()` in `setup()` and `loop()` in your main `loop()` function.

Example:
```cpp
#include "miragexr.h"

MirageXR mirage("your-ssid", "your-password", 8080);

void setup() {
  Serial.begin(115200);
  mirage.start();
}

void loop() {
  mirage.loop();
  // Your custom logic here
}
```

## Unity Integration

To connect your Unity project, use the [MirageXR Electrobits Core Unity package](https://github.com/rabeeqiblawi/miragexr_electrobits_core_unity). This package provides the necessary scripts and components to communicate with your Arduino device running MirageXR.

## License

MIT License.