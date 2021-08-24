#ifndef DM_PLATFORM_h
#define DM_PLATFORM_h

// Determine type of system
#if defined (__AVR__)
  #define DM_TOOLCHAIN_ARDUINO
#elif defined(TOOLCHAIN_ARM) || defined(TOOLCHAIN_ARM_MICRO)
  #define DM_TOOLCHAIN_MBED
#else
  //#error Only Arduino and Mbed toolchains are supported
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "stm32wb55xx.h"
#endif

#define delay(ms) HAL_Delay(ms)

// Arduino
#if defined (DM_TOOLCHAIN_ARDUINO)
  
  // Mandatory includes for Arduino
  #include <Arduino.h>
  #include <avr/pgmspace.h>
  #include <SPI.h>
  #include <Wire.h>
  // Clear bit, Set bit, High pulse and Low pulse macros
  #define gbi(reg, _bitmask) ((*reg) & (_bitmask))
  #define cbi(reg, _bitmask) *reg &= ~_bitmask
  #define sbi(reg, _bitmask) *reg |= _bitmask
  #define pulse_high(reg, _bitmask) sbi(reg, _bitmask); cbi(reg, _bitmask);
  #define pulse_low(reg, _bitmask) cbi(reg, _bitmask); sbi(reg, _bitmask);
  #define delay_us(us) delayMicroseconds(us)
  // Map of mandatory pin names, from Arduino names to D* and A*
  #define D0   0
  #define D1   1
  #define D2   2
  #define D3   3
  #define D4   4
  #define D5   5
  #define D6   6
  #define D7   7
  #define D8   8
  #define D9   9
  #define D10 10
  #define D11 11
  #define D12 12
  #define D13 13

  
  // Needed typedefs, not normally present in the Arduino environment
  #ifndef uint8_t
    #define uint8_t unsigned char
  #endif
  #ifndef int8_t
    #define int8_t signed char
  #endif
  #ifndef uint16_t
    #define uint16_t unsigned short
  #endif
  #ifndef uint32_t
    #define uint32_t unsigned long
  #endif
  #ifndef regtype
    #define regtype volatile unsigned char
  #endif
  #ifndef regsize
    #define regsize unsigned char
  #endif

// Mbed
#elif defined(DM_TOOLCHAIN_MBED)

  // Mandatory includes for Mbed
  #include "mbed.h"

  // Clear bit, Set bit, High pulse, Low pulse, Boundary limits and Delay macros
  #define sbi(reg, _bitmask) (*(reg) = 1)
  #define cbi(reg, _bitmask) (*(reg) = 0)
  #define pulse_high(reg, _bitmask) do { *(reg) = 1; *(reg) = 0; } while(0)
  #define pulse_low(reg, _bitmask) do { *(reg) = 0; *(reg) = 1; } while(0)
  #define constrain(amt,low,high) ((amt)<=(low)?(low):((amt)>(high)?(high):(amt)))
  #define delay(ms) wait_ms(ms)
  #define delay_us(us) wait_us(us)
  // Map of mandatory pin names, from Arduino names to D* and A*
  #if defined(__LPC407x_8x_177x_8x_H__)
    #define D0   p10
    #define D1   p9
    #define D2   p31
    #define D3   p32
    #define D4   p33
    #define D5   p37
    #define D6   p38
    #define D7   p34
    #define D8   p8
    #define D9   p39
    #define D10  p14
    #define D11  p11
    #define D12  p12
    #define D13  p13
    
    #define A0   p15
    #define A1   p16
    #define A2   p17
    #define A3   p18
    #define A4   p19
    #define A5   p20
  #endif
  // Special handling for the LPC1549 LPCXpresso board
#ifdef LPC15XX_H
  #define D5 P0_11
#endif
#endif

#endif /* DM_PLATFORM_h */
