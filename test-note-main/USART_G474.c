void UART5_UART_Init(void) 
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Choose the clock source for UART5
    LL_RCC_SetUARTClockSource(LL_RCC_UART5_CLKSOURCE_HSI);

    // Peripheral clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);

    /** UART5 GPIO Configuration
    PC12   ------> UART5_TX
    PD2    ------> UART5_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Basic USART Settings
    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(UART5, &USART_InitStruct);
    
    LL_USART_ConfigAsyncMode(UART5);

    // Advanced FIFO Settings
    LL_USART_SetTXFIFOThreshold(UART5, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(UART5, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_EnableFIFO(UART5);
    LL_USART_DisableOverrunDetect(UART5);

    // Enable reception now. TXE is enabled only when an ACK packet is ready.
    LL_USART_EnableIT_RXNE_RXFNE(UART5); // Interrupt on character receive
    LL_USART_EnableIT_IDLE(UART5); // Interrupt on line idle (end of packet)
    LL_USART_EnableIT_ERROR(UART5); // Interrupt on error

    // UART5 interrupt Configuration in NVIC
    NVIC_SetPriority(UART5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(UART5_IRQn);

    // Turn on UART5
    LL_USART_Enable(UART5);

    // Polling UART5 initialisation
    while((!(LL_USART_IsActiveFlag_TEACK(UART5))) || (!(LL_USART_IsActiveFlag_REACK(UART5))))
    {
    }
}
