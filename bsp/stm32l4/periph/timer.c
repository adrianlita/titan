#include <periph/timer.h>
#include <periph/cpu.h>
#include <kernel.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_lptim.h>
#include <stm32l4xx_ll_tim.h>

#include <assert.h>

TITAN_PDEBUG_FILE_MARK;

#define TITAN_TIMER_TOTAL    4

typedef struct __titan_timer_setting {
    timer_isr_t callback;
    timer_isr_param_t callback_param;
    uint32_t param;
    uint32_t prescaler;
    uint32_t period;
} titan_timer_setting_t;

static titan_timer_setting_t titan_timer_settings[TITAN_TIMER_TOTAL];

static uint8_t _timer_to_timer_no(timer_t timer) {
    switch(timer) {
        case (timer_t)TIM2:
            return 0;
            break;

        case (timer_t)TIM6:
            return 1;
            break;

        case (timer_t)TIM15:
            return 2;
            break;

        case (timer_t)TIM16:
            return 3;
            break;

        default:
            passert(0);
            break;
    }
    return 255;
}

static void _timer_irq(timer_t timer, uint8_t timer_no) {
    if(titan_timer_settings[timer_no].callback) {
        titan_timer_settings[timer_no].callback();
    }
    else if(titan_timer_settings[timer_no].callback_param) {
        titan_timer_settings[timer_no].callback_param(titan_timer_settings[timer_no].param);
    }
}

void timer_init(timer_t timer, uint32_t prescaler, timer_isr_t callback) {
    passert(prescaler);

    switch(timer) {
        case (timer_t)TIM2:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
            NVIC_SetPriority(TIM2_IRQn, 0);        //checkAL
            NVIC_EnableIRQ(TIM2_IRQn);
            break;

        case (timer_t)TIM6:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);
            NVIC_SetPriority(TIM6_DAC_IRQn, 0);        //checkAL
            NVIC_EnableIRQ(TIM6_DAC_IRQn);
            break;

        case (timer_t)TIM15:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM15);
            NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 0);        //checkAL
            NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
            break;

        case (timer_t)TIM16:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM16); 
            NVIC_SetPriority(TIM1_UP_TIM16_IRQn, 0);        //checkAL
            NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
            break;

        default:
            passert(0);
            break;
    }

    uint8_t timer_no = _timer_to_timer_no(timer);

    titan_timer_settings[timer_no].prescaler = prescaler;
    titan_timer_settings[timer_no].callback = callback;
    titan_timer_settings[timer_no].period = 0;

    LL_TIM_SetPrescaler((TIM_TypeDef*)timer, prescaler - 1);
    LL_TIM_EnableIT_UPDATE((TIM_TypeDef*)timer);
}

void timer_init_param(timer_t timer, uint32_t prescaler, timer_isr_param_t callback, uint32_t param) {
    uint8_t timer_no = _timer_to_timer_no(timer);
    timer_init(timer, prescaler, 0);
    titan_timer_settings[timer_no].callback_param = callback;
    titan_timer_settings[timer_no].param = param;
}

void timer_deinit(timer_t timer) {
    switch(timer) {
        case (timer_t)TIM2:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_TIM2);
            NVIC_DisableIRQ(TIM2_IRQn);
            break;

        case (timer_t)TIM6:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_TIM6);
            NVIC_DisableIRQ(TIM6_DAC_IRQn);
            break;

        case (timer_t)TIM15:
            LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM15);
            NVIC_DisableIRQ(TIM1_BRK_TIM15_IRQn);
            break;

        case (timer_t)TIM16:
            LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM16); 
            NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);
            break;

        default:
            passert(0);
            break;
    }

    uint8_t timer_no = _timer_to_timer_no(timer);
    titan_timer_settings[timer_no].callback_param = 0;
    titan_timer_settings[timer_no].callback = 0;
}

void timer_start(timer_t timer, uint32_t period) {
    passert(((TIM_TypeDef*)timer == TIM2) || ((TIM_TypeDef*)timer == TIM6) || ((TIM_TypeDef*)timer == TIM15) || ((TIM_TypeDef*)timer == TIM16));
    passert(period);

    if((TIM_TypeDef*)timer != TIM2) {
        passert(period <= 65536);
    }

    LL_TIM_SetAutoReload((TIM_TypeDef*)timer, period - 1);
    LL_TIM_EnableCounter((TIM_TypeDef*)timer);
}

void timer_stop(timer_t timer) {
    passert(((TIM_TypeDef*)timer == TIM2) || ((TIM_TypeDef*)timer == TIM6) || ((TIM_TypeDef*)timer == TIM15) || ((TIM_TypeDef*)timer == TIM16));

    LL_TIM_DisableCounter((TIM_TypeDef*)timer);
}

void timer_change_period(timer_t timer, uint32_t new_period) {
    passert(((TIM_TypeDef*)timer == TIM2) || ((TIM_TypeDef*)timer == TIM6) || ((TIM_TypeDef*)timer == TIM15) || ((TIM_TypeDef*)timer == TIM16));
    passert(new_period);

    if((TIM_TypeDef*)timer != TIM2) {
        passert(new_period <= 65536);
    }

    LL_TIM_SetAutoReload((TIM_TypeDef*)timer, new_period - 1);
}

void timer_clear(timer_t timer) {
    passert(((TIM_TypeDef*)timer == TIM2) || ((TIM_TypeDef*)timer == TIM6) || ((TIM_TypeDef*)timer == TIM15) || ((TIM_TypeDef*)timer == TIM16));

    LL_TIM_SetCounter((TIM_TypeDef*)timer, 0);
}

uint32_t timer_read(timer_t timer) {
    passert(((TIM_TypeDef*)timer == TIM2) || ((TIM_TypeDef*)timer == TIM6) || ((TIM_TypeDef*)timer == TIM15) || ((TIM_TypeDef*)timer == TIM16));

    return LL_TIM_GetCounter((TIM_TypeDef*)timer);
}


void TIM2_IRQHandler(void) {
    if(LL_TIM_IsActiveFlag_UPDATE(TIM2) == 1) {
        _timer_irq((timer_t)TIM2, 0);
        LL_TIM_ClearFlag_UPDATE(TIM2);
    }
}

void TIM6_DAC_IRQHandler(void) {
    if(LL_TIM_IsActiveFlag_UPDATE(TIM6) == 1) {
        _timer_irq((timer_t)TIM6, 1);
        LL_TIM_ClearFlag_UPDATE(TIM6);
    }
}

void TIM1_BRK_TIM15_IRQHandler(void) {
    if(LL_TIM_IsActiveFlag_UPDATE(TIM15) == 1) {
        _timer_irq((timer_t)TIM15, 2);
        LL_TIM_ClearFlag_UPDATE(TIM15);
    }
}

void TIM1_UP_TIM16_IRQHandler(void) {
    if(LL_TIM_IsActiveFlag_UPDATE(TIM16) == 1) {
        _timer_irq((timer_t)TIM16, 3);
        LL_TIM_ClearFlag_UPDATE(TIM16);
    }
}
