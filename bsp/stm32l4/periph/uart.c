#include <periph/uart.h>
#include <periph/cpu.h>
#include <kernel.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_lpuart.h>
#include <stm32l4xx_ll_usart.h>

#include <assert.h>

TITAN_PDEBUG_FILE_MARK;

#define TITAN_UART_TOTAL    5

typedef struct __titan_uart_setting {
    uart_async_rx_t uart_async_rx;
    uart_async_rx_param_t uart_async_rx_param;
    uint32_t param;

    task_t *task;
    uint8_t *tx_buffer;
    uint32_t tx_length;
    uint32_t tx_count;
} titan_uart_setting_t;

static titan_uart_setting_t titan_uart_settings[TITAN_UART_TOTAL];

static uint8_t _uart_to_uart_no(uart_t uart) {
    switch(uart) {
        case (uart_t)USART1:
            return 0;
            break;

        case (uart_t)USART2:
            return 1;
            break;

        case (uart_t)USART3:
            return 2;
            break;

        case (uart_t)UART4:
            return 3;
            break;

        case (uart_t)LPUART1:
            return 4;
            break;

        default:
            passert(0);
            break;
    }
    return 255;
}

static void _uart_irq(uart_t uart, uint8_t uart_no) {
    if(LL_USART_IsActiveFlag_RXNE((USART_TypeDef*)uart) && LL_USART_IsEnabledIT_RXNE((USART_TypeDef*)uart)) {
        volatile uint8_t received_char;
        received_char = LL_USART_ReceiveData8((USART_TypeDef*)uart);
        if(titan_uart_settings[uart_no].uart_async_rx) {
            titan_uart_settings[uart_no].uart_async_rx(received_char);
        }
        else if(titan_uart_settings[uart_no].uart_async_rx_param) {
            titan_uart_settings[uart_no].uart_async_rx_param(titan_uart_settings[uart_no].param, received_char);
        }
    }
    else if(LL_USART_IsActiveFlag_TXE((USART_TypeDef*)uart) && LL_USART_IsEnabledIT_TXE((USART_TypeDef*)uart)) {
        if(titan_uart_settings[uart_no].tx_count == titan_uart_settings[uart_no].tx_length) {
            LL_USART_DisableIT_TXE((USART_TypeDef*)uart);
            LL_USART_EnableIT_TC((USART_TypeDef*)uart);
        }
        else {
            LL_USART_TransmitData8((USART_TypeDef*)uart, titan_uart_settings[uart_no].tx_buffer[titan_uart_settings[uart_no].tx_count]);
            titan_uart_settings[uart_no].tx_count++;
        }
    }
    else if(LL_USART_IsActiveFlag_TC((USART_TypeDef*)uart) && LL_USART_IsEnabledIT_TC((USART_TypeDef*)uart)) {
        LL_USART_ClearFlag_TC((USART_TypeDef*)uart);
        LL_USART_DisableIT_TC((USART_TypeDef*)uart);
        kernel_scheduler_io_wait_finished(titan_uart_settings[uart_no].task);
        kernel_scheduler_trigger();
    }
    else if(LL_USART_IsActiveFlag_ORE((USART_TypeDef*)uart)) {
        LL_USART_ClearFlag_ORE((USART_TypeDef*)uart);
    }
}

void uart_init(uart_t uart, uint32_t baudrate, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_flow_control_t flow_control, uart_async_rx_t rx_callback) {
    uint32_t parity_real;
    uint32_t stop_bits_real;
    uint32_t flow_control_real;
    switch(parity) {
        case UART_PARITY_NONE:
            parity_real = LL_USART_PARITY_NONE;
            break;

        case UART_PARITY_EVEN:
            parity_real = LL_USART_PARITY_EVEN;
            break;

        case UART_PARITY_ODD:
            parity_real = LL_USART_PARITY_ODD;
            break;

        default:
            passert(0);
            break;
    }

    switch(stop_bits) {
        case UART_STOP_BITS_1:
            stop_bits_real = LL_USART_STOPBITS_1;
            break;

        case UART_STOP_BITS_2:
            stop_bits_real = LL_USART_STOPBITS_2;
            break;

        default:
            passert(0);
            break;
    }

    switch(flow_control) {
        case UART_FLOW_CONTROL_NONE:
            flow_control_real = LL_USART_HWCONTROL_NONE;
            break;

        case UART_FLOW_CONTROL_RTS_CTS:
            flow_control_real = LL_USART_HWCONTROL_RTS_CTS;
            break;

        default:
            passert(0);
            break;
    }

    switch(uart) {
        case (uart_t)USART1:
            NVIC_SetPriority(USART1_IRQn, 0);    //checkAL priority
            NVIC_EnableIRQ(USART1_IRQn);

            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
            LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
            break;

        case (uart_t)USART2:
            NVIC_SetPriority(USART2_IRQn, 0);    //checkAL priority
            NVIC_EnableIRQ(USART2_IRQn);

            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
            LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
            break;

        case (uart_t)USART3:
            NVIC_SetPriority(USART3_IRQn, 0);    //checkAL priority
            NVIC_EnableIRQ(USART3_IRQn);

            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
            LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
            break;

        case (uart_t)UART4:
            NVIC_SetPriority(UART4_IRQn, 0);    //checkAL priority
            NVIC_EnableIRQ(UART4_IRQn);

            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4);
            LL_RCC_SetUARTClockSource(LL_RCC_UART4_CLKSOURCE_PCLK1);
            break;

        case (uart_t)LPUART1:
            NVIC_SetPriority(LPUART1_IRQn, 0);    //checkAL priority
            NVIC_EnableIRQ(LPUART1_IRQn);

            LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);
            LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
            break;

        default:
            passert(0);
            break;
    }

    titan_uart_settings[_uart_to_uart_no(uart)].uart_async_rx = rx_callback;

    if(uart != (uart_t)LPUART1) {
        LL_USART_SetTransferDirection((USART_TypeDef*)uart, LL_USART_DIRECTION_TX_RX);

        /* 8 data bit, 1 start bit, 1 stop bit, no parity */
        LL_USART_ConfigCharacter((USART_TypeDef*)uart, LL_USART_DATAWIDTH_8B, parity_real, stop_bits_real);
        LL_USART_SetHWFlowCtrl((USART_TypeDef*)uart, flow_control_real);
        LL_USART_SetOverSampling((USART_TypeDef*)uart, LL_USART_OVERSAMPLING_16);

        /* Set Baudrate to 115200 using APB frequency set to 80000000 Hz */
        /* Frequency available for USART peripheral can also be calculated through LL RCC macro */
        /* Ex :
            Periphclk = LL_RCC_GetUSARTClockFreq(Instance); or LL_RCC_GetUARTClockFreq(Instance); depending on USART/UART instance
        
            In this example, Peripheral Clock is expected to be equal to 80000000 Hz => equal to cpu_clock_speed()
        */
        LL_USART_SetBaudRate((USART_TypeDef*)uart, cpu_clock_speed(), LL_USART_OVERSAMPLING_16, baudrate); 
        LL_USART_Enable((USART_TypeDef*)uart);
        while((!(LL_USART_IsActiveFlag_TEACK((USART_TypeDef*)uart))) || (!(LL_USART_IsActiveFlag_REACK((USART_TypeDef*)uart))));    //checkAL --- e ok daca e facuta la inceput, dar altfel spinlock degeaba
        LL_USART_ClearFlag_ORE((USART_TypeDef*)uart);
        LL_USART_EnableIT_RXNE((USART_TypeDef*)uart);
    }
    else {
        LL_LPUART_SetTransferDirection((USART_TypeDef*)uart, LL_LPUART_DIRECTION_TX_RX);

        /* 8 data bit, 1 start bit, 1 stop bit, no parity */
        LL_LPUART_ConfigCharacter((USART_TypeDef*)uart, LL_LPUART_DATAWIDTH_8B, parity_real, stop_bits_real);
        LL_LPUART_SetHWFlowCtrl((USART_TypeDef*)uart, flow_control_real);

        /* Set Baudrate to 115200 using APB frequency set to 80000000 Hz */
        /* Frequency available for LPUART peripheral can also be calculated through LL RCC macro */
        /* Ex :
            Periphclk = LL_RCC_GetLPUARTClockFreq(Instance); or LL_RCC_GetUARTClockFreq(Instance); depending on LPUART/UART instance
        
            In this example, Peripheral Clock is expected to be equal to 80000000 Hz => equal to cpu_clock_speed()
        */
        LL_LPUART_SetBaudRate((USART_TypeDef*)uart, cpu_clock_speed(), baudrate); 
        LL_LPUART_Enable((USART_TypeDef*)uart);
        LL_LPUART_ClearFlag_ORE((USART_TypeDef*)uart);
        LL_LPUART_EnableIT_RXNE((USART_TypeDef*)uart);
    }
}

void uart_init_param(uart_t uart, uint32_t baudrate, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_flow_control_t flow_control, uart_async_rx_param_t rx_callback, uint32_t param) {
    uart_init(uart, baudrate, parity, stop_bits, flow_control, (uart_async_rx_t)rx_callback);
    uint8_t uart_no = _uart_to_uart_no(uart);
    titan_uart_settings[uart_no].uart_async_rx_param = rx_callback;
    titan_uart_settings[uart_no].param = param;
    titan_uart_settings[uart_no].uart_async_rx = 0;
}

void uart_deinit(uart_t uart) {
    switch(uart) {
        case (uart_t)USART1:
            NVIC_DisableIRQ(USART1_IRQn);
            LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART1);
            LL_USART_Disable((USART_TypeDef*)uart);
            LL_USART_DisableIT_RXNE((USART_TypeDef*)uart);
            break;

        case (uart_t)USART2:
            NVIC_DisableIRQ(USART2_IRQn);
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);
            LL_USART_Disable((USART_TypeDef*)uart);
            LL_USART_DisableIT_RXNE((USART_TypeDef*)uart);
            break;

        case (uart_t)USART3:
            NVIC_DisableIRQ(USART3_IRQn);
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3);
            LL_USART_Disable((USART_TypeDef*)uart);
            LL_USART_DisableIT_RXNE((USART_TypeDef*)uart);
            break;

        case (uart_t)UART4:
            NVIC_DisableIRQ(UART4_IRQn);
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART4);
            LL_USART_Disable((USART_TypeDef*)uart);
            LL_USART_DisableIT_RXNE((USART_TypeDef*)uart);
            break;

        case (uart_t)LPUART1:
            NVIC_DisableIRQ(LPUART1_IRQn);
            LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_LPUART1);
            LL_LPUART_Disable((USART_TypeDef*)uart);
            LL_LPUART_DisableIT_RXNE((USART_TypeDef*)uart);
            break;

        default:
            passert(0);
            break;
    }
}

void uart_write(uart_t uart, uint8_t *data, uint32_t length) {
    uint8_t uart_no = _uart_to_uart_no(uart);

    titan_uart_settings[uart_no].tx_buffer = data;
    titan_uart_settings[uart_no].tx_length = length;
    titan_uart_settings[uart_no].tx_count = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_uart_settings[uart_no].task = (task_t*)kernel_current_task;
    LL_USART_EnableIT_TXE((USART_TypeDef*)uart);
    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
}

void USART1_IRQHandler(void) {
    _uart_irq((uart_t)USART1, 0);
}

void USART2_IRQHandler(void) {
    _uart_irq((uart_t)USART2, 1);
}

void USART3_IRQHandler(void) {
    _uart_irq((uart_t)USART3, 2);
}

void UART4_IRQHandler(void) {
    _uart_irq((uart_t)UART4, 3);
}

void LPUART1_IRQHandler(void) {
    _uart_irq((uart_t)LPUART1, 4);
}
