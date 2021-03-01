#ifndef _IOEXPPINMAP_H
#define _IOEXPPINMAP_H

// Pin definitions on the MCP23017
#define GPA0 0
#define GPA1 1
#define GPA2 2
#define GPA3 3
#define GPA4 4
#define GPA5 5
#define GPA6 6
#define GPA7 7
#define GPB0 8
#define GPB1 9
#define GPB2 10
#define GPB3 11
#define GPB4 12
#define GPB5 13
#define GPB6 14
#define GPB7 15

// Opto-Isolated Zone Inputs
#define PIN_OPTO_ZONE_1 27
#define PIN_OPTO_ZONE_2 33
#define PIN_OPTO_ZONE_3 15
#define PIN_OPTO_ZONE_4 32
#define PIN_OPTO_ZONE_5 14
#define PIN_OPTO_ZONE_6 13
#define PIN_OPTO_ZONE_7 GPB7
#define PIN_OPTO_ZONE_8 GPB6

// Dry-contact Zone Inputs
#define PIN_DC_ZONE_1 GPA0
#define PIN_REX_ZONE_1 GPA1
#define PIN_DC_ZONE_2 GPA2
#define PIN_REX_ZONE_2 GPA3
#define PIN_DC_ZONE_3 GPA4
#define PIN_REX_ZONE_3 GPA5
#define PIN_DC_ZONE_4 GPA6
#define PIN_REX_ZONE_4 GPA7

// On-board Relay Outputs
#define PIN_RELAY_1 GPB4
#define PIN_RELAY_2 GPB3
#define PIN_RELAY_3 GPB2
#define PIN_RELAY_4 GPB1

// SPI Outputs
#define PIN_SPI_CS GPB5

// LED Outputs
#define PIN_ARM_LED GPB0
#define PIN_HEARTBEAT_LED 12

// Expansion header pins
#define PIN_EXP_A0_DAC_2_IO 26
#define PIN_EXP_A1_DAC_1_IO 25
#define PIN_EXP_A2_I 34
#define PIN_EXP_A3_I 39
#define PIN_EXP_A4_I 36
#define PIN_EXP_A5_IO 4
#define PIN_EXP_DIO_16 16
#define PIN_EXP_DIO_17 17
#define PIN_EXP_DIO_21 21

// Relay module relays
#define PIN_RM_RELAY1 GPB0
#define PIN_RM_RELAY2 GPB1
#define PIN_RM_RELAY3 GPB2
#define PIN_RM_RELAY4 GPB3
#define PIN_RM_RELAY5 GPB4

// Relay module LEDs
#define PIN_RM_LED1 GPA7
#define PIN_RM_LED2 GPA6
#define PIN_RM_LED3 GPA5
#define PIN_RM_LED4 GPA4
#define PIN_RM_LED5 GPA3

// Relay module detect pins
#define PIN_RM_DET_IN GPB5
#define PIN_RM_DET_OUT GPA2

#endif