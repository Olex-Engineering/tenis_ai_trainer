| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

# Tennis Player Assistant Device

Welcome to the Tennis Player Assistant Device project! This innovative creation is designed to revolutionize the way tennis players analyze and improve their game. By combining cutting-edge technology with the dynamics of tennis, our device provides players with invaluable insights and actionable statistics to elevate their performance on the court.

## Project Overview

The Tennis Player Assistant Device is a compact gadget designed to attach seamlessly to your tennis racket. Utilizing advanced gyro and accelerometer sensors, coupled with artificial intelligence, it offers comprehensive analysis of your gameplay. From shot quality to serve efficiency, from identifying technical flaws to suggesting corrective measures, our device is your ultimate companion in mastering the sport.

## Technology Stack

- **Board**: ESP32S3
- **Framework**: esp-idf
- **Main Logic**: Implemented using FreeRTOS tasks and queues. Each internal component features an action queue, accessible via public functions.
- **Communication**: Utilizes Bluetooth Low Energy (BLE) for external communication.
- **Power Management**: Implements light sleep, and deep sleep modes for efficient power usage. Powered by a 3.7V battery. Use external 32.768kHz cristall for BLE Modem sleep.
- **Battery Gauge**: Includes a battery gauge to monitor battery State of Charge (SOC) level.
- **User Interface**: All user inputs are managed through a single touch control interface. Functions include long touch for power on/off, and short touch for session start/end or data collection process start/end in DATA_COLLECTOR mode.
- **User Outputs**: Equipped with a buzzer and LED for user feedback and notifications.

## Configuration Options

- **Device Mode**: Controlled by the IS_DATA_COLLECTOR_DEVICE flag, enabling switching to data collector mode. In this mode, raw data from the gyro and accelerometer is collected and transmitted via BLE to mobile devices and subsequently to AWS S3 for storage and analysis.

## License

Proprietary License

All rights reserved. No part of this software may be reproduced, distributed, or transmitted in any form or by any means, including photocopying, recording, or other electronic or mechanical methods, without the prior written permission of the copyright holder.

