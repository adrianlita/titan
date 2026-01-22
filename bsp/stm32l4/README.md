## GPIO

GPIO pins are named PA0, PA1, etc

Interrupts can only be used once per pin.
For example, you can use PA0, PA1, PA2, etc in parallel, but not PA0 and PB0 in parallel.

PWM functionality is available only on TIM1 and TIM3.

# DAC

DAC can be either 8bit or 12bit. 8bit DAC has a faster settling time

# ADC

ADC can be either 8bit or 12bit.
When ADC is not initialized
Next inits will pass, but the actual resolution will be the first, and bits will either be added or substracted.

## TIMERS

Available timers are TIM2 (32bit), TIM6, TIM15, TIM16

TIM1 and TIM3 are used for PWM, so using them directly as a standard timer will result in errors.

Note: TIM6 is usually reserved for DAC operation

LPTIM1, LPTIM2 not used as timers
