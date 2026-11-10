# Embedded Smart Circuit Breaker PHM Prototype

An STM32F4-based embedded monitoring prototype for collecting electrical and thermal sensor signals from a miniature circuit breaker (MCB). The project focuses on the embedded acquisition layer of a Prognostics and Health Management (PHM) workflow.

## Project scope

The completed embedded prototype configures three STM32 ADC peripherals and acquires six analog input channels. It uses direct register-level configuration in C and triple regular simultaneous mode.

### Implemented in the firmware

- Enables ADC1, ADC2 and ADC3 clocks.
- Configures six analog input pins.
- Sets up two regular sequence ranks on each ADC.
- Uses triple regular simultaneous mode to sample three channels in each rank.
- Stores six 12-bit raw ADC readings.
- Converts raw counts to estimated ADC-pin millivolts using the configured reference constant.
- Provides named variables for one voltage-sensor output, one current-sensor output and four temperature-sensor outputs.

## Hardware and channel mapping

| ADC | Channel | Pin | Signal |
|---|---:|---|---|
| ADC1 | 1 | PA1 | Voltage-sensor output |
| ADC2 | 2 | PA2 | Current-sensor output |
| ADC3 | 3 | PA3 | Temperature sensor 3 |
| ADC1 | 8 | PB0 | Temperature sensor 1 |
| ADC2 | 9 | PB1 | Temperature sensor 2 |
| ADC3 | 11 | PC1 | Temperature sensor 4 |

The sensor channels are logical assignments from the project progress report. Confirm final wiring and the exact STM32F4 part before hardware testing.

## Repository layout

- `src/main.c` — STM32F4 ADC acquisition firmware snapshot.
- `docs/` — Add final circuit diagrams, calibration records and project report here.

## Build notes

- IDE: Keil µVision 5.
- Language: C.
- Device header used by the firmware: `stm32f4xx.h`.
- Select the exact STM32F4 device and matching CMSIS/device pack in the Keil project.
- Configure the system and ADC clocks for the selected board and verify the ADC common-register settings against that device's reference manual.

## Validation notes and engineering limits

The firmware's count-to-millivolt conversion uses a nominal 3300 mV reference and a 12-bit maximum count of 4095. Treat this as an estimate until the actual ADC reference voltage and analog front-end are measured and calibrated.

The current acquisition firmware does **not** by itself convert sensor outputs into breaker voltage, current or temperature engineering units. Sensor-specific transfer functions, physical sensor calibration, filtering, data logging and health-index computation require their own implementation and validation.

The broader PHM roadmap includes Thermal, Electrical and Overall Health Indicators, degradation trending, maintenance recommendations and possible Remaining Useful Life (RUL) analysis. Do not interpret these roadmap items as validated features of this firmware snapshot.

## Safety

This is a low-voltage laboratory monitoring prototype, not a certified protective device. Never energize exposed mains conductors or operate an opened MCB under live conditions. Use isolated, current-limited low-voltage test arrangements and qualified supervision for any hazardous electrical work.

## Attribution

Project progress report: Embedded Systems Design (BEEE213L), Fall Semester 2026–27. Team project.
