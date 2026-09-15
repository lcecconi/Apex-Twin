# Apex-Twin

Apex twin is a twin module system for go-kart live telemetry data logging, displaying and analysis. 
The system is built around two modules: The **Apex-Track** live telemetry module and the **Apex-Dash** Dashboard Module.

## Architecture
The twin module approach allows for full flexibility in the positioning of the tracking unit and antennas as well as its powering, while retaining a lightweight battery powered display unit that can be installed on the steering wheel, with no need for wiring between modules. 

### Apex-Track

Apex-Track is a live telemetry module for go-karts. It is responsible for collecting data from different sensors, such as IMU, GPS, RPM, ..., logging it and sending it to the Apex-Dash module for real-time display and post-session analysis.

### Apex-Dash

Apex-Dash is a live display dashboard module built around a 4" RLCD screen and an ESP32-S3 processor. It is responsible for receiving the data collected by the Apex-Track module for real-time display and post session analysis.
