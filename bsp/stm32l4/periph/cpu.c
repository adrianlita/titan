#include <periph/cpu.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>

void cpu_clock_init(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    LL_RCC_MSI_Enable();
    while(LL_RCC_MSI_IsReady() != 1) {
    };

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_Enable();
    LL_RCC_PLL_EnableDomain_SYS();
    while(LL_RCC_PLL_IsReady() != 1) {
    };

    /* Sysclk activation on the main PLL */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    };

    /* Set APB1 & APB2 prescaler*/
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    /* Update Core Clock */
    SystemCoreClockUpdate();
}

uint32_t cpu_clock_speed(void) {
    return SystemCoreClock;
}

void cpu_boot_reason_get(titan_boot_reason_t *store) {
    //*store = TITAN_BOOT_REASON_UNAVAILABLE;   //use this when there's no way to figure out boot reason

    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        *store = TITAN_BOOT_REASON_WATCHDOG;
    }
    else if ((RCC->CSR & RCC_CSR_BORRSTF) || (RCC->CSR & RCC_CSR_LPWRRSTF) || (RCC->CSR & RCC_CSR_FWRSTF)) {
        *store = TITAN_BOOT_REASON_HARDWARE;
    }
    else if (RCC->CSR & RCC_CSR_SFTRSTF) {
        *store = TITAN_BOOT_REASON_USER;
    }
    else {
        *store = TITAN_BOOT_REASON_NORMAL;
    }

    RCC->CSR |= RCC_CSR_RMVF;
}
