# STM32F4-Based Smart DC Motor Control & Multi-Sensor Telemetry System
 An advanced, closed-loop embedded control system designed on the STM32F407 Discovery platform. This project integrates real-time ultrasonic obstacle tracking, dynamic PWM motor driving with low-speed anti-stall logic, physical analog/digital telemetry, onboard status indication, and interrupt-driven audio-visual alarms.
The primary objective of this project is to design, integrate, and program a comprehensive microcontroller-based embedded system that successfully combines real-time analog sensor reading, physical actuation, visual/auditory environmental feedback, and digital data telemetry. In real-world industrial applications—such as autonomous guided vehicles (AGVs) or smart conveyor belts—systems must simultaneously regulate speed based on user inputs while monitoring their surroundings to prevent collisions.
This project simulates such an industrial system. It features a DC motor whose speed is precisely governed by an analog potentiometer via PWM (Pulse Width Modulation). Concurrently, an ultrasonic distance sensor scans the environment. Depending on the proximity of an obstacle, the system dynamically changes its state, warning the operator through a multi-stage LED indicator panel and an audio buzzer. Furthermore, to ensure complete system observability, the active motor speed is translated into a physical analog voltage via a DAC (Digital-to-Analog Converter) and displayed on a physical dual-panel voltmeter. Simultaneously, structured telemetry data (current mode, speed percentage, and obstacle distance) is transmitted to a computer terminal via a UART interface. The system transitions smoothly between a safe "Setup Mode" and an active "Drive Mode" using a hardware interrupt triggered by a push-button.

# 🎬 System Demonstration





https://github.com/user-attachments/assets/5f5b82c7-a044-4c51-b4ad-900473d25eaf





https://github.com/user-attachments/assets/dbc94b96-eed0-4aec-8925-743ade2d7893



https://github.com/user-attachments/assets/247dd3a2-77fe-4395-8e23-eddeff943a8d






https://github.com/user-attachments/assets/80d96af6-63b3-4526-92bf-ef8bb5c6926a



<img width="609" height="787" alt="image" src="https://github.com/user-attachments/assets/4ea0769b-564a-445d-8aad-5ba830daf94c" /> <img width="630" height="787" alt="image" src="https://github.com/user-attachments/assets/9aaabc28-c1bc-40b4-adcb-76e414090f5e" />
<img width="827" height="913" alt="image" src="https://github.com/user-attachments/assets/3af49166-8045-43fe-8c08-675b495a4b64" />





# What Does This System Do? (Project Purpose)
This project simulates real-world industrial and automotive safety applications—such as Adaptive Cruise Control (ACC) in modern vehicles and Automated Guided Vehicles (AGVs) in automated logistics warehouses.

Instead of simple open-loop motor driving, the system continuously monitors its physical environment, calculates safe operating distances, adjusts motor torque and speed dynamically, and provides real-time multi-channel telemetry (visual, serial, and acoustic) to the operator.

# How It Works?
## System Initialization & Standby:
### Upon boot, the system starts in Setup / Safety Standby Mode. The motor remains completely disabled to prevent unintended startup current spikes.
## Interrupt-Driven Activation:
### Pressing the onboard Blue Push Button (PA0) triggers an External Interrupt (EXTI), safely toggling the system state between Setup Mode and Active Drive Mode.
## Multi-Input Sensing:
### Potentiometer (ADC1): Reads the user's manual speed reference via internal 12-bit ADC (0 – 4095).
### HC-SR04 Ultrasonic Sensor: Continuously calculates distance to obstacles using microsecond timer pulses (10 µs trigger).
## Closed-Loop Decision & Anti-Stall Algorithm:
### Critical Range (less than 5 cm): Triggers an Emergency Stop (PWM = 0), lights up alert LEDs, and activates the Buzzer alarm.
### Warning / Low-Speed Zone (5 – 10 cm): To overcome internal rotor inertia, mechanical friction, and stall conditions of larger DC motors, the system enforces a Minimum PWM Threshold (Offset = 150). This prevents motor lockup, coil overheating, and stall current draw.
### Safe Zone (greater than 10 cm): The target speed is scaled smoothly based on the potentiometer input and obstacle proximity.
## Multi-Channel Telemetry & Observability:
### DAC Telemetry (PA4): Converts 12-bit speed data into an accurate 0 – 3.3V analog signal to drive a digital panel voltmeter.
### Current & Voltage Monitoring: External digital voltmeter and ammeter modules continuously monitor real-time supply rail voltage and motor draw current under load.
### Onboard LED Diagnostics: Uses STM32 Discovery onboard LEDs (PD12–PD15) to indicate live system state and hazard levels.
### UART Serial Telemetry: Streams formatted live telemetry data (Speed, Distance, Voltage) at 115200 Baud to PC serial monitors (e.g., Hercules).

<img width="1156" height="716" alt="image" src="https://github.com/user-attachments/assets/9ae4b590-5d9b-4037-827b-169e10840c71" />

<img width="1285" height="667" alt="image" src="https://github.com/user-attachments/assets/720702bf-a7cf-4fa0-85b6-be1f14a9aa28" />

# Hardware Components:
## Microcontroller: STM32F407G-DISC1 Board (ARM Cortex-M4)
## Actuator Driver: L298N Dual H-Bridge Motor Driver Module
## Motor: High-Torque Brushed DC Motor
## Sensors & Manual Inputs:
### HC-SR04 Ultrasonic Distance Sensor
### 10kΩ Rotational Potentiometer
### Onboard Blue User Push Button
## Monitoring & Indicators:
### Digital Panel Voltmeter & Ammeter Displays
### Active Buzzer Module (Acoustic Alert)
### Onboard Status LEDs (Green, Orange, Red, Blue)
## Power Supply: 12V 1A External DC Adapter (Common GND Architecture)

# Pin Mapping & Peripherals
<img width="1427" height="422" alt="image" src="https://github.com/user-attachments/assets/47171d97-69a1-4e04-b0d2-f9fe528b92d3" />
<img width="1370" height="767" alt="image" src="https://github.com/user-attachments/assets/f7541d26-f031-42a8-9df5-9e5630b8e4e4" />


# Key Engineering Problems Solved:
## Eliminating Power Sag (9V Battery to 12V Adapter):
### Initial testing with a standard 9V block battery resulted in severe voltage drops and MCU resets when the motor drew high startup currents. Solved by integrating a dedicated 12V 1A DC adapter with a unified Common Ground (GND) bus across all boards and meters.
## Mechanical Inertia & Anti-Stall Logic:
### When upgrading to a larger DC motor, PWM values below 80–90 (approx. 1V) failed to generate sufficient torque to overcome static friction, locking the motor. Implemented an automated minimum PWM threshold (Offset = 150) in embedded C to guarantee smooth startup and rotation at close ranges.
## Dual-Voltage Meter Isolation:
### Properly decoupled the panel meter's 5V power supply lines from its 0–3.3V measurement input pin (PA4 DAC output) to ensure accurate readings without overloading the STM32 DAC pin.

