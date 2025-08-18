This repository contains the STM32 project directories for our second final project.
The entry points of the applications are located at:

/External_CAN/Core/Src/main.c
/Internal_CAN/Core/Src/main.c

## External_CAN
Equipped with rain detection and light sensors.
Determines rainfall amount and automatically adjusts wiper speed accordingly.
Monitors external light conditions to control the headlamp operation.
Processes the data and transmits it to the infotainment system via CAN.

## Internal_CAN
Equipped with temperature & humidity and CO₂ sensors.
Provides the internal temperature and humidity of the vehicle.
Measures CO₂ concentration inside the vehicle and sends warning signals if dangerous levels are detected.
Processes the data and transmits it to the infotainment system via CAN.
