#include <periph/gpio.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_adc.h>
#include <stm32l4xx_ll_dac.h>
#include <stm32l4xx_ll_gpio.h>
#include <stm32l4xx_ll_exti.h>
#include <stm32l4xx_ll_tim.h>
#include <stm32l4xx_ll_system.h>
#include <kernel.h>
#include <semaphore.h>
#include <assert.h>



TITAN_PDEBUG_FILE_MARK;

typedef struct __titan_gpio {
    //stm32l4 architecture
    GPIO_TypeDef *gpio;
    uint32_t pin;

    uint8_t adc;
    uint8_t dac;
    uint8_t pwm;
    
    uint32_t af0;
    uint32_t af1;
    uint32_t af2;
    uint32_t af3;
    uint32_t af4;
    uint32_t af5;
    uint32_t af6;
    uint32_t af7;
    uint32_t af8;
    uint32_t af9;
    uint32_t af10;
    uint32_t af12;
    uint32_t af13;
    uint32_t af14;
} titan_gpio_t;


/* alternate functions */
#define AF_TIM1         (1 << 0)
#define AF_TIM2         (1 << 1)
#define AF_TIM15        (1 << 2)
#define AF_TIM16        (1 << 3)
#define AF_LPTIM1       (1 << 4)
#define AF_LPTIM2       (1 << 5)
#define AF_I2C1         (1 << 6)
#define AF_I2C2         (1 << 7)
#define AF_I2C3         (1 << 8)
#define AF_SPI1         (1 << 9)
#define AF_SPI2         (1 << 10)
#define AF_SPI3         (1 << 11)
#define AF_UART1        (1 << 12)
#define AF_UART2        (1 << 13)
#define AF_UART3        (1 << 14)
#define AF_LPUART1      (1 << 15)
#define AF_QUADSPI      (1 << 16)
#define AF_SAI1         (1 << 17)
#define AF_TSC          (1 << 18)
#define AF_COMP1        (1 << 19)
#define AF_COMP2        (1 << 20)
#define AF_SYSAF        (1 << 21)

static const titan_gpio_t titan_pin_defs[TITAN_GPIO_TOTAL_PINS] = {
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_0,  .adc = 0x15, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM2,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = AF_UART2,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = AF_COMP1,      .af13 = AF_SAI1,       .af14 = AF_TIM2        },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_1,  .adc = 0x16, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM2,        .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = AF_SPI1,       .af6 = 0,             .af7 = AF_UART2,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = AF_TIM15       },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_2,  .adc = 0x17, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM2,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = AF_UART2,      .af8 = AF_LPUART1,    .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = AF_COMP2,      .af13 = 0,             .af14 = AF_TIM15       },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_3,  .adc = 0x18, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM2,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = AF_UART2,      .af8 = AF_LPUART1,    .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = 0,             .af13 = AF_SAI1,       .af14 = AF_TIM15       },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_4,  .adc = 0x19, .dac = 0x11, .pwm = 0x00, .af0 = 0,              .af1 = 0,              .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = AF_SPI3,       .af7 = AF_UART2,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = AF_LPTIM2      },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_5,  .adc = 0x1A, .dac = 0x12, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM2,        .af2 = AF_TIM2,       .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = 0,             .af7 = 0,             .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = AF_LPTIM2      },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_6,  .adc = 0x1B, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = AF_COMP1,      .af7 = AF_UART3,      .af8 = AF_LPUART1,    .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = AF_TIM1,       .af13 = 0,             .af14 = AF_TIM16       },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_7,  .adc = 0x1C, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = AF_I2C3,       .af5 = AF_SPI1,       .af6 = 0,             .af7 = 0,             .af8 = 0,             .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = AF_COMP2,      .af13 = 0,             .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_8,  .adc = 0x00, .dac = 0x00, .pwm = 0x11, .af0 = AF_SYSAF,       .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = AF_LPTIM2      },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_9,  .adc = 0x00, .dac = 0x00, .pwm = 0x12, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = 0,             .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = AF_TIM15       },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_10, .adc = 0x00, .dac = 0x00, .pwm = 0x13, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = 0,             .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_11, .adc = 0x00, .dac = 0x00, .pwm = 0x14, .af0 = 0,              .af1 = AF_TIM1,        .af2 = AF_TIM1,       .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = AF_COMP1,      .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = AF_TIM1,       .af13 = 0,             .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_12, .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_13, .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = AF_SYSAF,       .af1 = 0      ,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = 0,             .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_14, .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = AF_SYSAF,       .af1 = AF_LPTIM1,      .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = 0,             .af6 = 0,             .af7 = 0,             .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOA, .pin = LL_GPIO_PIN_15, .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = AF_SYSAF,       .af1 = AF_TIM2,        .af2 = AF_TIM2,       .af3 = AF_UART2,      .af4 = 0,             .af5 = AF_SPI1,       .af6 = AF_SPI3,       .af7 = AF_UART3,      .af8 = 0,             .af9 = AF_TSC,        .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = 0              },

    {.gpio = GPIOB, .pin = LL_GPIO_PIN_0,  .adc = 0x1F, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = 0,             .af7 = AF_UART3,      .af8 = 0,             .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = AF_COMP1,      .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_1,  .adc = 0x10, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_TIM1,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = AF_UART3,      .af8 = AF_LPUART1,    .af9 = 0,             .af10 = AF_QUADSPI,    .af12 = 0,             .af13 = 0,             .af14 = AF_LPTIM2      },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_3,  .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = AF_SYSAF,       .af1 = AF_TIM2,        .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = AF_SPI1,       .af6 = AF_SPI3,       .af7 = AF_UART1,      .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_4,  .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = AF_SYSAF,       .af1 = 0,              .af2 = 0,             .af3 = 0,             .af4 = AF_I2C3,       .af5 = AF_SPI1,       .af6 = AF_SPI3,       .af7 = AF_UART1,      .af8 = 0,             .af9 = AF_TSC,        .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = 0              },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_5,  .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_LPTIM1,      .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = AF_SPI1,       .af6 = AF_SPI3,       .af7 = AF_UART1,      .af8 = 0,             .af9 = AF_TSC,        .af10 = 0,             .af12 = AF_COMP2,      .af13 = AF_SAI1,       .af14 = AF_TIM16       },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_6,  .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_LPTIM1,      .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = 0,             .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = AF_TSC,        .af10 = 0,             .af12 = 0,             .af13 = AF_SAI1,       .af14 = AF_TIM16       },
    {.gpio = GPIOB, .pin = LL_GPIO_PIN_7,  .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = AF_LPTIM1,      .af2 = 0,             .af3 = 0,             .af4 = AF_I2C1,       .af5 = 0,             .af6 = 0,             .af7 = AF_UART1,      .af8 = 0,             .af9 = AF_TSC,        .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = 0              },
    
    {.gpio = GPIOC, .pin = LL_GPIO_PIN_14, .adc = 0x00, .dac = 0x00, .pwm = 0x00, .af0 = 0,              .af1 = 0,              .af2 = 0,             .af3 = 0,             .af4 = 0,             .af5 = 0,             .af6 = 0,             .af7 = 0,             .af8 = 0,             .af9 = 0,             .af10 = 0,             .af12 = 0,             .af13 = 0,             .af14 = 0              },
};


typedef struct __titan_gpio_setting {
    uint8_t mode;

    //exti settings
    uint32_t exti_line;
    IRQn_Type exti_irq;
    gpio_isr_t exti;
    gpio_isr_param_t exti_param;
    uint32_t param;
    uint8_t bits;
} titan_gpio_setting_t;

static titan_gpio_setting_t titan_pin_settings[TITAN_GPIO_TOTAL_PINS] = {0};

//to track active pins and enable/disable port clock
static uint32_t gpioa_active = 0;
static uint32_t gpiob_active = 0;
static uint32_t gpioc_active = 0;
static uint32_t gpiod_active = 0;

//to track active extis and info on them
static uint32_t exti_active = 0;
static uint32_t exti9_5_active = 0;
static uint32_t exti15_10_active = 0;
static uint32_t exti_pin[16] = {0};

//to track active PWMs
static uint32_t tim1_active = 0;

//to track active ADC channels
static uint32_t adc1_active = 0;
static uint32_t adc1_bits = 0;
static task_t *adc1_waiting_task = 0;
static uint32_t adc1_result = 0;
static semaphore_t adc1_semaphore;
static uint8_t adc1_semaphore_created = 0;

#define ADC_SAMPLING_TIME   LL_ADC_SAMPLINGTIME_47CYCLES_5

static void _gpio_init_gpio(gpio_pin_t pin) {
    switch((uint32_t)titan_pin_defs[pin].gpio) {
        case (uint32_t)GPIOA:
            if(gpioa_active == 0) {
                LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
            }
            gpioa_active |= titan_pin_defs[pin].pin;
            break;

        case (uint32_t)GPIOB:
            if(gpiob_active == 0) {
                LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
            }
            gpiob_active |= titan_pin_defs[pin].pin;
            break;

        case (uint32_t)GPIOC:
            if(gpioc_active == 0) {
                LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
            }
            gpioc_active |= titan_pin_defs[pin].pin;
            break;

        case (uint32_t)GPIOD:
            if(gpiod_active == 0) {
                LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
            }
            gpiod_active |= titan_pin_defs[pin].pin;
            break;
    }
}

static void _gpio_deinit_gpio(gpio_pin_t pin) {
    switch((uint32_t)titan_pin_defs[pin].gpio) {
        case (uint32_t)GPIOA:
            gpioa_active &= ~titan_pin_defs[pin].pin;
            if(gpioa_active == 0) {
                LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
            }
            break;

        case (uint32_t)GPIOB:
            gpiob_active &= ~titan_pin_defs[pin].pin;
            if(gpiob_active == 0) {
                LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
            }
            break;

        case (uint32_t)GPIOC:
            gpioc_active &= ~titan_pin_defs[pin].pin;
            if(gpioc_active == 0) {
                LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
            }
            break;

        case (uint32_t)GPIOD:
            gpiod_active &= ~titan_pin_defs[pin].pin;
            if(gpiod_active == 0) {
                LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
            }
            break;
    }
}

static void _gpio_set_af(GPIO_TypeDef *gpio, uint32_t pin, uint32_t alternate) {
    switch(pin) {
        case LL_GPIO_PIN_0:
        case LL_GPIO_PIN_1:
        case LL_GPIO_PIN_2:
        case LL_GPIO_PIN_3:
        case LL_GPIO_PIN_4:
        case LL_GPIO_PIN_5:
        case LL_GPIO_PIN_6:
        case LL_GPIO_PIN_7:
            LL_GPIO_SetAFPin_0_7(gpio, pin, alternate);
            break;

        case LL_GPIO_PIN_8:
        case LL_GPIO_PIN_9:
        case LL_GPIO_PIN_10:
        case LL_GPIO_PIN_11:
        case LL_GPIO_PIN_12:
        case LL_GPIO_PIN_13:
        case LL_GPIO_PIN_14:
        case LL_GPIO_PIN_15:
            LL_GPIO_SetAFPin_8_15(gpio, pin, alternate);
            break;
    }
}

void gpio_init_digital(gpio_pin_t pin, gpio_digital_mode_t mode, gpio_pull_t pull) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == 0);

    _gpio_init_gpio(pin);

    uint32_t mode_real = 0;
    uint32_t output_type_real = 0;
    switch(mode) {
        case GPIO_MODE_INPUT:
            mode_real = LL_GPIO_MODE_INPUT;
            break;

        case GPIO_MODE_OUTPUT_PP:
            mode_real = LL_GPIO_MODE_OUTPUT;
            output_type_real = LL_GPIO_OUTPUT_PUSHPULL;
            break;

        case GPIO_MODE_OUTPUT_OD:
            mode_real = LL_GPIO_MODE_OUTPUT;
            output_type_real = LL_GPIO_OUTPUT_OPENDRAIN;
            break;

        default:
            passert(0);
            break;
    }

    uint32_t pull_real = 0;
    switch(pull) {
        case GPIO_PULL_NOPULL:
            pull_real = LL_GPIO_PULL_NO;
            break;

        case GPIO_PULL_UP:
            pull_real = LL_GPIO_PULL_UP;
            break;

        case GPIO_PULL_DOWN:
            pull_real = LL_GPIO_PULL_DOWN;
            break;

        default:
            passert(0);
            break;
    }


    titan_pin_settings[pin].mode = (uint8_t)mode;
    
    LL_GPIO_SetPinMode(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, mode_real);
    if(mode & (GPIO_MODE_OUTPUT_OD | GPIO_MODE_OUTPUT_PP)) {
        LL_GPIO_SetPinOutputType(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, output_type_real);
        LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_SPEED_FREQ_LOW);
    }
    LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, pull_real);
}

void gpio_init_interrupt(gpio_pin_t pin, gpio_interrupt_mode_t mode, gpio_pull_t pull, gpio_isr_t callback) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == 0);
    passert(callback != 0);

    gpio_init_digital(pin, GPIO_MODE_INPUT, pull);

    if(exti_active == 0) {
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    }
    exti_active |= titan_pin_defs[pin].pin;

    uint32_t exti_source_port;
    switch((uint32_t)titan_pin_defs[pin].gpio) {
        case (uint32_t)GPIOA:
            exti_source_port = LL_SYSCFG_EXTI_PORTA;
            break;

        case (uint32_t)GPIOB:
            exti_source_port = LL_SYSCFG_EXTI_PORTB;
            break;

        case (uint32_t)GPIOC:
            exti_source_port = LL_SYSCFG_EXTI_PORTC;
            break;

        case (uint32_t)GPIOD:
            exti_source_port = LL_SYSCFG_EXTI_PORTD;
            break;

        default:
            passert(0);
            break;
    }

    uint32_t exti_source_line;
    switch((uint32_t)titan_pin_defs[pin].pin) {
        case LL_GPIO_PIN_0:
            exti_source_line = LL_SYSCFG_EXTI_LINE0;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_0;
            titan_pin_settings[pin].exti_irq = EXTI0_IRQn;
            exti_pin[0] = pin;
            break;

        case LL_GPIO_PIN_1:
            exti_source_line = LL_SYSCFG_EXTI_LINE1;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_1;
            titan_pin_settings[pin].exti_irq = EXTI1_IRQn;
            exti_pin[1] = pin;
            break;

        case LL_GPIO_PIN_2:
            exti_source_line = LL_SYSCFG_EXTI_LINE2;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_2;
            titan_pin_settings[pin].exti_irq = EXTI2_IRQn;
            exti_pin[2] = pin;
            break;

        case LL_GPIO_PIN_3:
            exti_source_line = LL_SYSCFG_EXTI_LINE3;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_3;
            titan_pin_settings[pin].exti_irq = EXTI3_IRQn;
            exti_pin[3] = pin;
            break;

        case LL_GPIO_PIN_4:
            exti_source_line = LL_SYSCFG_EXTI_LINE4;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_4;
            titan_pin_settings[pin].exti_irq = EXTI4_IRQn;
            exti_pin[4] = pin;
            break;

        case LL_GPIO_PIN_5:
            exti_source_line = LL_SYSCFG_EXTI_LINE5;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_5;
            titan_pin_settings[pin].exti_irq = EXTI9_5_IRQn;
            exti_pin[5] = pin;
            break;

        case LL_GPIO_PIN_6:
            exti_source_line = LL_SYSCFG_EXTI_LINE6;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_6;
            titan_pin_settings[pin].exti_irq = EXTI9_5_IRQn;
            exti_pin[6] = pin;
            break;

        case LL_GPIO_PIN_7:
            exti_source_line = LL_SYSCFG_EXTI_LINE7;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_7;
            titan_pin_settings[pin].exti_irq = EXTI9_5_IRQn;
            exti_pin[7] = pin;
            break;

        case LL_GPIO_PIN_8:
            exti_source_line = LL_SYSCFG_EXTI_LINE8;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_8;
            titan_pin_settings[pin].exti_irq = EXTI9_5_IRQn;
            exti_pin[8] = pin;
            break;

        case LL_GPIO_PIN_9:
            exti_source_line = LL_SYSCFG_EXTI_LINE9;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_9;
            titan_pin_settings[pin].exti_irq = EXTI9_5_IRQn;
            exti_pin[9] = pin;
            break;

        case LL_GPIO_PIN_10:
            exti_source_line = LL_SYSCFG_EXTI_LINE10;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_10;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[10] = pin;
            break;

        case LL_GPIO_PIN_11:
            exti_source_line = LL_SYSCFG_EXTI_LINE11;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_11;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[11] = pin;
            break;

        case LL_GPIO_PIN_12:
            exti_source_line = LL_SYSCFG_EXTI_LINE12;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_12;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[12] = pin;
            break;

        case LL_GPIO_PIN_13:
            exti_source_line = LL_SYSCFG_EXTI_LINE13;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_13;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[13] = pin;
            break;

        case LL_GPIO_PIN_14:
            exti_source_line = LL_SYSCFG_EXTI_LINE14;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_14;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[14] = pin;
            break;

        case LL_GPIO_PIN_15:
            exti_source_line = LL_SYSCFG_EXTI_LINE15;
            titan_pin_settings[pin].exti_line = LL_EXTI_LINE_15;
            titan_pin_settings[pin].exti_irq = EXTI15_10_IRQn;
            exti_pin[15] = pin;
            break;

        default:
            passert(0);
            break;
    }

    LL_SYSCFG_SetEXTISource(exti_source_port, exti_source_line);

    titan_pin_settings[pin].mode = (uint8_t)mode;
    titan_pin_settings[pin].exti = callback;

    LL_EXTI_EnableIT_0_31(titan_pin_settings[pin].exti_line);
    switch(mode) {
        case GPIO_MODE_INTERRUPT_REDGE:
            LL_EXTI_EnableRisingTrig_0_31(titan_pin_settings[pin].exti_line);
            break;

        case GPIO_MODE_INTERRUPT_FEDGE:
            LL_EXTI_EnableFallingTrig_0_31(titan_pin_settings[pin].exti_line);
            break;

        case GPIO_MODE_INTERRUPT_RFEDGE:
            LL_EXTI_EnableRisingTrig_0_31(titan_pin_settings[pin].exti_line);
            LL_EXTI_EnableFallingTrig_0_31(titan_pin_settings[pin].exti_line);
            break;

        default:
            passert(0);
            break;
    }

    switch(titan_pin_settings[pin].exti_irq) {
        case EXTI0_IRQn:
        case EXTI1_IRQn:
        case EXTI2_IRQn:
        case EXTI3_IRQn:
        case EXTI4_IRQn:
            NVIC_EnableIRQ(titan_pin_settings[pin].exti_irq); 
            NVIC_SetPriority(titan_pin_settings[pin].exti_irq, 0);    //checkAL priority
            break;

        case EXTI9_5_IRQn:
            if(exti9_5_active == 0) {
                NVIC_EnableIRQ(titan_pin_settings[pin].exti_irq); 
                NVIC_SetPriority(titan_pin_settings[pin].exti_irq, 0);    //checkAL priority
            }
            exti9_5_active |= titan_pin_defs[pin].pin;
            break;

        case EXTI15_10_IRQn:
            if(exti15_10_active == 0) {
                NVIC_EnableIRQ(titan_pin_settings[pin].exti_irq); 
                NVIC_SetPriority(titan_pin_settings[pin].exti_irq, 0);    //checkAL priority
            }
            exti15_10_active |= titan_pin_defs[pin].pin;
            break;

        default:
            passert(0);
            break;
    }
}


void gpio_init_interrupt_param(gpio_pin_t pin, gpio_interrupt_mode_t mode, gpio_pull_t pull, gpio_isr_param_t callback, uint32_t param) {
    gpio_init_interrupt(pin, mode, pull, (gpio_isr_t)callback);
    titan_pin_settings[pin].exti_param = callback;
    titan_pin_settings[pin].param = param;
    titan_pin_settings[pin].exti = 0;
}


void gpio_init_analog(gpio_pin_t pin, gpio_analog_mode_t mode, uint32_t bits) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == 0);
    passert((bits == 8) || (bits == 12));

    _gpio_init_gpio(pin);

    uint8_t channel = titan_pin_defs[pin].adc & 0xF;
    if(channel == 0) {
        channel = 16;
    }

    switch(mode) {
        case GPIO_MODE_ANALOG_IN:
            passert(titan_pin_defs[pin].adc != 0);

            if(adc1_active == 0) {
                if(adc1_semaphore_created == 0) {
                    semaphore_create(&adc1_semaphore, 1);
                    adc1_semaphore_created = 1;
                }

                NVIC_SetPriority(ADC1_IRQn, 0);         //checkAL interrupt priority
                NVIC_EnableIRQ(ADC1_IRQn);
                LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC);
                LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_SYNC_PCLK_DIV2);

                adc1_bits = bits;
                switch(bits) {
                    case 8:
                        LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_8B);
                        break;

                    case 12:
                        LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
                        break;
                }

                LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);
                LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_MODE_NONE);

                LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
                LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
                LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);
                LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
                LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);

                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_1, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_2, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_2, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_3, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_3, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_4, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_4, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_5, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_5, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_6, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_6, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_7, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_7, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_8, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_8, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_9, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_9, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_10, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_10, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_11, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_11, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_12, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_12, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_13, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_13, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_14, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_14, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_15, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_15, LL_ADC_SINGLE_ENDED);
                LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_16, ADC_SAMPLING_TIME);
                LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_16, LL_ADC_SINGLE_ENDED);

                LL_ADC_EnableIT_EOC(ADC1);

                LL_ADC_DisableDeepPowerDown(ADC1);
                LL_ADC_EnableInternalRegulator(ADC1);

                LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    
                while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0);

                LL_ADC_Enable(ADC1);
                
                /* Poll for ADC ready to convert */
                while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0);
            }

            adc1_active |= (1 << channel);
            break;

        case GPIO_MODE_ANALOG_OUT:
            passert(titan_pin_defs[pin].dac != 0);

            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DAC1);
            LL_DAC_SetMode(DAC1, LL_DAC_CHANNEL_1, LL_DAC_MODE_NORMAL_OPERATION);
            LL_DAC_SetTriggerSource(DAC1, LL_DAC_CHANNEL_1, LL_DAC_TRIG_SOFTWARE);
            LL_DAC_ConfigOutput(DAC1, LL_DAC_CHANNEL_1, LL_DAC_OUTPUT_MODE_NORMAL, LL_DAC_OUTPUT_BUFFER_ENABLE, LL_DAC_OUTPUT_CONNECT_GPIO);

            LL_DAC_DisableDMAReq(DAC1, LL_DAC_CHANNEL_1);
            LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, 0);

            LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
            LL_DAC_EnableTrigger(DAC1, LL_DAC_CHANNEL_1);
            break;

        default:
            passert(0);
            break;
    }

    titan_pin_settings[pin].mode = mode;
    titan_pin_settings[pin].bits = bits;

    LL_GPIO_SetPinMode(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_PULL_NO);
}

void gpio_init_special(gpio_pin_t pin, gpio_special_function_t function, uint32_t param) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == 0);

    _gpio_init_gpio(pin);

    uint32_t output_type_real;
    uint32_t pull_real;
    uint32_t speed_real;
    LL_GPIO_SetPinMode(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_MODE_ALTERNATE);

    switch(function) {
        case GPIO_SPECIAL_FUNCTION_I2C:
            output_type_real = LL_GPIO_OUTPUT_OPENDRAIN;
            pull_real = LL_GPIO_PULL_UP;
            speed_real = LL_GPIO_SPEED_FREQ_HIGH;
            switch(param) {
                case (uint32_t)I2C1:
                    if(titan_pin_defs[pin].af4 == AF_I2C1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_4);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)I2C2:
                    if(titan_pin_defs[pin].af4 == AF_I2C2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_4);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)I2C3:
                    if(titan_pin_defs[pin].af4 == AF_I2C3) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_4);
                    }
                    else {
                        passert(0);
                    }
                    break;

                default:
                    passert(0);
                    break;
            }

            LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, speed_real);
            LL_GPIO_SetPinOutputType(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, output_type_real);
            LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, pull_real);
            break;

        case GPIO_SPECIAL_FUNCTION_SPI:
            output_type_real = LL_GPIO_OUTPUT_PUSHPULL;
            pull_real = LL_GPIO_PULL_DOWN;
            speed_real = LL_GPIO_SPEED_FREQ_HIGH;
            switch(param) {
                case (uint32_t)SPI1:
                    if(titan_pin_defs[pin].af5 == AF_SPI1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_5);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)SPI2:
                    if(titan_pin_defs[pin].af5 == AF_SPI2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_5);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)SPI3:
                    if(titan_pin_defs[pin].af6 == AF_SPI3) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_6);
                    }
                    else {
                        passert(0);
                    }
                    break;

                default:
                    passert(0);
                    break;
            }

            LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, speed_real);
            LL_GPIO_SetPinOutputType(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, output_type_real);
            LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, pull_real);
            break;

        case GPIO_SPECIAL_FUNCTION_TIMER:
            switch(param) {
                case (uint32_t)TIM1:
                    if(titan_pin_defs[pin].af1 == AF_TIM1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_1);
                    }
                    else if(titan_pin_defs[pin].af2 == AF_TIM1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_2);
                    }
                    else if(titan_pin_defs[pin].af3 == AF_TIM1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_3);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)TIM2:
                    if(titan_pin_defs[pin].af1 == AF_TIM2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_1);
                    }
                    else if(titan_pin_defs[pin].af2 == AF_TIM2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_2);
                    }
                    else if(titan_pin_defs[pin].af14 == AF_TIM2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_14);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)TIM15:
                    if(titan_pin_defs[pin].af14 == AF_TIM15) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_14);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)TIM16:
                    if(titan_pin_defs[pin].af14 == AF_TIM16) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_14);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)LPTIM1:
                    if(titan_pin_defs[pin].af1 == AF_LPTIM1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_1);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)LPTIM2:
                    if(titan_pin_defs[pin].af14 == AF_LPTIM2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_14);
                    }
                    else {
                        passert(0);
                    }
                    break;


                    default:
                        passert(0);
                        break;
            }
            break;

        case GPIO_SPECIAL_FUNCTION_UART:
            output_type_real = LL_GPIO_OUTPUT_PUSHPULL;
            pull_real = LL_GPIO_PULL_UP;
            speed_real = LL_GPIO_SPEED_FREQ_HIGH;
            switch(param) {
                case (uint32_t)USART1:
                    if(titan_pin_defs[pin].af7 == AF_UART1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_7);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)USART2:
                    if(titan_pin_defs[pin].af3 == AF_UART2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_3);
                    }
                    else if(titan_pin_defs[pin].af7 == AF_UART2) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_7);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)USART3:
                    if(titan_pin_defs[pin].af7 == AF_UART3) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_7);
                    }
                    else {
                        passert(0);
                    }
                    break;

                case (uint32_t)LPUART1:
                    if(titan_pin_defs[pin].af8 == AF_LPUART1) {
                        _gpio_set_af(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_AF_8);
                    }
                    else {
                        passert(0);
                    }
                    break;

                default:
                    passert(0);
                    break;
            }

            LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, speed_real);
            LL_GPIO_SetPinOutputType(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, output_type_real);
            LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, pull_real);
            break;

        default:
            passert(0);
            break;
    }
}

void gpio_init_pwm(gpio_pin_t pin, gpio_digital_mode_t mode, gpio_pull_t pull, uint32_t initial_value) {
    passert(titan_pin_defs[pin].pwm != 0);

    uint32_t *timer_active = &tim1_active;
    TIM_TypeDef* timer;
    uint32_t timer_no = (titan_pin_defs[pin].pwm >> 4);
    uint32_t channel = (titan_pin_defs[pin].pwm & 0x0F);

    switch(timer_no) {
        case 1:
            timer = TIM1;
            timer_active = &tim1_active;
            break;

        default:
            passert(0);
            break;
    }

    passert((*timer_active & (1 << channel)) == 0);
    gpio_init_special(pin, GPIO_SPECIAL_FUNCTION_TIMER, (uint32_t)timer);


    uint32_t output_type_real = 0;
    switch(mode) {
        case GPIO_MODE_INPUT:
            passert(0); //this should not be selected
            break;

        case GPIO_MODE_OUTPUT_PP:
            output_type_real = LL_GPIO_OUTPUT_PUSHPULL;
            break;

        case GPIO_MODE_OUTPUT_OD:
            output_type_real = LL_GPIO_OUTPUT_OPENDRAIN;
            break;

        default:
            passert(0);
            break;
    }

    uint32_t pull_real = 0;
    switch(pull) {
        case GPIO_PULL_NOPULL:
            pull_real = LL_GPIO_PULL_NO;
            break;

        case GPIO_PULL_UP:
            pull_real = LL_GPIO_PULL_UP;
            break;

        case GPIO_PULL_DOWN:
            pull_real = LL_GPIO_PULL_DOWN;
            break;

        default:
            passert(0);
            break;
    }

    LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, output_type_real);
    LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, pull_real);

    if(*timer_active == 0) {
        switch(timer_no) {
            case 1:
                LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
                break;
        }

        LL_TIM_SetPrescaler(timer, 79);
        LL_TIM_EnableARRPreload(timer);
        LL_TIM_SetAutoReload(timer, 0xFFFE);
        LL_TIM_EnableCounter(timer);
        LL_TIM_GenerateEvent_UPDATE(timer);
        LL_TIM_EnableAllOutputs(timer);
    }

    initial_value = (initial_value != 0) * 0xFFFF;

    switch(channel) {
        case 1:
            LL_TIM_OC_SetMode(timer, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
            LL_TIM_OC_SetCompareCH1(timer, initial_value);    //write duty
            LL_TIM_OC_EnablePreload(timer, LL_TIM_CHANNEL_CH1);
            LL_TIM_CC_EnableChannel(timer, LL_TIM_CHANNEL_CH1);
            break;

        case 2:
            LL_TIM_OC_SetMode(timer, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
            LL_TIM_OC_SetCompareCH2(timer, initial_value);    //write duty
            LL_TIM_OC_EnablePreload(timer, LL_TIM_CHANNEL_CH2);
            LL_TIM_CC_EnableChannel(timer, LL_TIM_CHANNEL_CH2);
            break;

        case 3:
            LL_TIM_OC_SetMode(timer, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
            LL_TIM_OC_SetCompareCH3(timer, initial_value);    //write duty
            LL_TIM_OC_EnablePreload(timer, LL_TIM_CHANNEL_CH3);
            LL_TIM_CC_EnableChannel(timer, LL_TIM_CHANNEL_CH3);
            break;

        case 4:
            LL_TIM_OC_SetMode(timer, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
            LL_TIM_OC_SetCompareCH4(timer, initial_value);    //write duty
            LL_TIM_OC_EnablePreload(timer, LL_TIM_CHANNEL_CH4);
            LL_TIM_CC_EnableChannel(timer, LL_TIM_CHANNEL_CH4);
            break;
    }

    *timer_active |= (1 << channel);
}

void gpio_deinit(gpio_pin_t pin) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);

    _gpio_deinit_gpio(pin);
    LL_GPIO_SetPinMode(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinSpeed(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin, LL_GPIO_PULL_NO);

    switch(titan_pin_settings[pin].mode) {
        case GPIO_MODE_INTERRUPT_REDGE:
        case GPIO_MODE_INTERRUPT_FEDGE:
        case GPIO_MODE_INTERRUPT_RFEDGE:
            //disable EXTI IRQ
            switch(titan_pin_settings[pin].exti_irq) {
                case EXTI0_IRQn:
                case EXTI1_IRQn:
                case EXTI2_IRQn:
                case EXTI3_IRQn:
                case EXTI4_IRQn:
                    NVIC_DisableIRQ(titan_pin_settings[pin].exti_irq); 
                    break;

                case EXTI9_5_IRQn:
                    exti9_5_active &= ~titan_pin_defs[pin].pin;
                    if(exti9_5_active == 0) {
                        NVIC_DisableIRQ(titan_pin_settings[pin].exti_irq); 
                    }
                    break;

                case EXTI15_10_IRQn:
                    exti15_10_active &= ~titan_pin_defs[pin].pin;
                    if(exti15_10_active == 0) {
                        NVIC_DisableIRQ(titan_pin_settings[pin].exti_irq); 
                    }
                    break;
                
                default:
                    passert(0);
                    break;
            }

            //disable EXTI
            LL_EXTI_DisableIT_0_31(titan_pin_settings[pin].exti_line);
            LL_EXTI_DisableRisingTrig_0_31(titan_pin_settings[pin].exti_line);
            LL_EXTI_DisableFallingTrig_0_31(titan_pin_settings[pin].exti_line);

            exti_active &= ~titan_pin_defs[pin].pin;
            if(exti_active == 0) {
               LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_SYSCFG);   
            }

            exti_pin[pin] = 0;
            titan_pin_settings[pin].exti = 0;
            titan_pin_settings[pin].exti_param = 0;
            titan_pin_settings[pin].exti_irq = (IRQn_Type)0;
            titan_pin_settings[pin].exti_line = 0;
            break;

        case GPIO_SPECIAL_FUNCTION_TIMER: {  //pwm mode
            uint32_t *timer_active = &tim1_active;
            TIM_TypeDef* timer;
            uint32_t timer_no = (titan_pin_defs[pin].pwm >> 4);
            uint32_t channel = (titan_pin_defs[pin].pwm & 0x0F);

            switch(timer_no) {
                case 1:
                    timer = TIM1;
                    timer_active = &tim1_active;
                    break;

                default:
                    passert(0);
                    break;
            }

            switch(channel) {
                case 1:
                    LL_TIM_OC_DisablePreload(timer, LL_TIM_CHANNEL_CH1);
                    LL_TIM_CC_DisableChannel(timer, LL_TIM_CHANNEL_CH1);
                    break;

                case 2:
                    LL_TIM_OC_DisablePreload(timer, LL_TIM_CHANNEL_CH2);
                    LL_TIM_CC_DisableChannel(timer, LL_TIM_CHANNEL_CH2);
                    break;

                case 3:
                    LL_TIM_OC_DisablePreload(timer, LL_TIM_CHANNEL_CH3);
                    LL_TIM_CC_DisableChannel(timer, LL_TIM_CHANNEL_CH3);
                    break;

                case 4:
                    LL_TIM_OC_DisablePreload(timer, LL_TIM_CHANNEL_CH4);
                    LL_TIM_CC_DisableChannel(timer, LL_TIM_CHANNEL_CH4);
                    break;
            }

            *timer_active &= ~(1 << channel);

            //also clear the timer
            if(*timer_active == 0) {
                LL_TIM_DisableARRPreload(timer);
                LL_TIM_DisableCounter(timer);
                LL_TIM_GenerateEvent_UPDATE(timer);
                LL_TIM_DisableAllOutputs(timer);

                switch(timer_no) {
                    case 1:
                        LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM1);
                        break;
                }
            }
        } break;

        case GPIO_MODE_ANALOG_OUT:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_DAC1);
            LL_DAC_Disable(DAC1, LL_DAC_CHANNEL_1);
            LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
            break;

        case GPIO_MODE_ANALOG_IN:
            LL_ADC_Disable(ADC1);
            LL_ADC_DisableIT_EOC(ADC1);
            LL_ADC_DisableInternalRegulator(ADC1);
            LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_ADC);
            NVIC_DisableIRQ(ADC1_IRQn);
            break;
    }

    titan_pin_settings[pin].mode = 0;
}


uint32_t gpio_digital_read(gpio_pin_t pin) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);

    return LL_GPIO_IsInputPinSet(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin);
}

void gpio_digital_write(gpio_pin_t pin, uint32_t value) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);

    if(value) {
        LL_GPIO_SetOutputPin(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin);
    }
    else {
        LL_GPIO_ResetOutputPin(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin);
    }
}

void gpio_digital_toggle(gpio_pin_t pin) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);

    LL_GPIO_TogglePin(titan_pin_defs[pin].gpio, titan_pin_defs[pin].pin);
}

uint32_t gpio_analog_read(gpio_pin_t pin) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == GPIO_MODE_ANALOG_IN);

    semaphore_lock(&adc1_semaphore);
    uint32_t ll_channel = LL_ADC_CHANNEL_1;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    adc1_waiting_task = (task_t*)kernel_current_task;

    uint8_t channel = titan_pin_defs[pin].adc & 0xF;
    switch(channel) {
        case 0:
            ll_channel = LL_ADC_CHANNEL_16;
            break;

        case 1:
            ll_channel = LL_ADC_CHANNEL_1;
            break;

        case 2:
            ll_channel = LL_ADC_CHANNEL_2;
            break;

        case 3:
            ll_channel = LL_ADC_CHANNEL_3;
            break;

        case 4:
            ll_channel = LL_ADC_CHANNEL_4;
            break;

        case 5:
            ll_channel = LL_ADC_CHANNEL_5;
            break;

        case 6:
            ll_channel = LL_ADC_CHANNEL_6;
            break;

        case 7:
            ll_channel = LL_ADC_CHANNEL_7;
            break;

        case 8:
            ll_channel = LL_ADC_CHANNEL_8;
            break;

        case 9:
            ll_channel = LL_ADC_CHANNEL_9;
            break;

        case 10:
            ll_channel = LL_ADC_CHANNEL_10;
            break;

        case 11:
            ll_channel = LL_ADC_CHANNEL_11;
            break;

        case 12:
            ll_channel = LL_ADC_CHANNEL_12;
            break;

        case 13:
            ll_channel = LL_ADC_CHANNEL_13;
            break;

        case 14:
            ll_channel = LL_ADC_CHANNEL_14;
            break;

        case 15:
            ll_channel = LL_ADC_CHANNEL_15;
            break;
    }
    
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, ll_channel);
    LL_ADC_REG_StartConversion(ADC1);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);

    uint32_t result = adc1_result;
    if(adc1_bits < titan_pin_settings[pin].bits) {
        result <<= (titan_pin_settings[pin].bits - adc1_bits);
    }
    else if(adc1_bits > titan_pin_settings[pin].bits) {
        result >>= (adc1_bits - titan_pin_settings[pin].bits);
    }

    semaphore_unlock(&adc1_semaphore);
    return result;
}

void gpio_analog_write(gpio_pin_t pin, uint32_t value) {
    passert(pin < TITAN_GPIO_TOTAL_PINS);
    passert(titan_pin_settings[pin].mode == GPIO_MODE_ANALOG_OUT);

    if(titan_pin_settings[pin].bits == 8) {
        LL_DAC_ConvertData8RightAligned(DAC1, LL_DAC_CHANNEL_1, value);
    }
    else {
        LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, value);
    }

    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_1);
}

void gpio_pwm_write_frequency(gpio_pin_t pin, uint32_t frequency) {
    uint32_t timer = (titan_pin_defs[pin].pwm >> 4);

    switch(timer) {
        case 1:
            timer = (uint32_t)TIM1;
            break;

        default:
            passert(0);
            break;
    }
    
    uint32_t prescaler = 1;
    uint32_t autoreload = SystemCoreClock / frequency;
    if(autoreload > 65535) {
        prescaler = (autoreload / 65536) + 1;
        autoreload = SystemCoreClock / prescaler / frequency;
    }

    //TimerClock =  SystemCoreClock / Prescaler - 1
    //ARR = TimerClock/Frequency * Prescaler + 1

    LL_TIM_SetPrescaler((TIM_TypeDef*)timer, prescaler - 1);
    LL_TIM_SetAutoReload((TIM_TypeDef*)timer, autoreload);
}

void gpio_pwm_write_duty(gpio_pin_t pin, uint32_t duty) {
    uint32_t timer = (titan_pin_defs[pin].pwm >> 4);
    uint32_t channel = (titan_pin_defs[pin].pwm & 0x0F);

    switch(timer) {
        case 1:
            timer = (uint32_t)TIM1;
            break;

        default:
            passert(0);
            break;
    }

    switch(channel) {
        case 1:
            LL_TIM_OC_SetCompareCH1((TIM_TypeDef*)timer, duty);
            break;

        case 2:
            LL_TIM_OC_SetCompareCH2((TIM_TypeDef*)timer, duty);
            break;

        case 3:
            LL_TIM_OC_SetCompareCH3((TIM_TypeDef*)timer, duty);
            break;

        case 4:
            LL_TIM_OC_SetCompareCH4((TIM_TypeDef*)timer, duty);
            break;
    }
}

uint32_t gpio_pwm_get_max_duty(gpio_pin_t pin) {
    uint32_t timer = (titan_pin_defs[pin].pwm >> 4);
    
    switch(timer) {
        case 1:
            timer = (uint32_t)TIM1;
            break;

        default:
            passert(0);
            break;
    }

    return LL_TIM_GetAutoReload((TIM_TypeDef*)timer) + 1;
}


void EXTI0_IRQHandler(void) {
    if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[0]].exti_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[0]].exti_line);

        if(titan_pin_settings[exti_pin[0]].exti) {
            titan_pin_settings[exti_pin[0]].exti();
        }
        else if(titan_pin_settings[exti_pin[0]].exti_param) {
            titan_pin_settings[exti_pin[0]].exti_param(titan_pin_settings[exti_pin[0]].param);
        }
    }
}

void EXTI1_IRQHandler(void) {
    if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[1]].exti_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[1]].exti_line);

        if(titan_pin_settings[exti_pin[1]].exti) {
            titan_pin_settings[exti_pin[1]].exti();
        }
        else if(titan_pin_settings[exti_pin[1]].exti_param) {
            titan_pin_settings[exti_pin[1]].exti_param(titan_pin_settings[exti_pin[1]].param);
        }
    }
}

void EXTI2_IRQHandler(void) {
    if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[2]].exti_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[2]].exti_line);

        if(titan_pin_settings[exti_pin[2]].exti) {
            titan_pin_settings[exti_pin[2]].exti();
        }
        else if(titan_pin_settings[exti_pin[2]].exti_param) {
            titan_pin_settings[exti_pin[2]].exti_param(titan_pin_settings[exti_pin[2]].param);
        }
    }
}

void EXTI3_IRQHandler(void) {
    if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[3]].exti_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[3]].exti_line);

        if(titan_pin_settings[exti_pin[3]].exti) {
            titan_pin_settings[exti_pin[3]].exti();
        }
        else if(titan_pin_settings[exti_pin[3]].exti_param) {
            titan_pin_settings[exti_pin[3]].exti_param(titan_pin_settings[exti_pin[3]].param);
        }
    }
}

void EXTI4_IRQHandler(void) {
    if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[4]].exti_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[4]].exti_line);

        if(titan_pin_settings[exti_pin[4]].exti) {
            titan_pin_settings[exti_pin[4]].exti();
        }
        else if(titan_pin_settings[exti_pin[4]].exti_param) {
            titan_pin_settings[exti_pin[4]].exti_param(titan_pin_settings[exti_pin[4]].param);
        }
    }
}


void EXTI9_5_IRQHandler(void) {
    for(uint8_t i = 5; i < 10; i++) {
        if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[i]].exti_line) != RESET)
        {
            LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[i]].exti_line);

            if(titan_pin_settings[exti_pin[i]].exti) {
                titan_pin_settings[exti_pin[i]].exti();
            }
            else if(titan_pin_settings[exti_pin[i]].exti_param) {
                titan_pin_settings[exti_pin[i]].exti_param(titan_pin_settings[exti_pin[i]].param);
            }
        }
    }
}

void EXTI15_10_IRQHandler(void) {
    for(uint8_t i = 10; i < 16; i++) {
        if(LL_EXTI_IsActiveFlag_0_31(titan_pin_settings[exti_pin[i]].exti_line) != RESET)
        {
            LL_EXTI_ClearFlag_0_31(titan_pin_settings[exti_pin[i]].exti_line);

            if(titan_pin_settings[exti_pin[i]].exti) {
                titan_pin_settings[exti_pin[i]].exti();
            }
            else if(titan_pin_settings[exti_pin[i]].exti_param) {
                titan_pin_settings[exti_pin[i]].exti_param(titan_pin_settings[exti_pin[i]].param);
            }
        }
    }
}

void TIM1_CC_IRQHandler(void) {
    passert(0);
}

void ADC1_IRQHandler(void) {
    if(LL_ADC_IsActiveFlag_EOC(ADC1) != 0) {
        LL_ADC_ClearFlag_EOC(ADC1);

        adc1_result = LL_ADC_REG_ReadConversionData12(ADC1);
        kernel_scheduler_io_wait_finished(adc1_waiting_task);
        kernel_scheduler_trigger();
    }
}
