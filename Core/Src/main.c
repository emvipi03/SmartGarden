#include "stm32f10x.h"

/* Private variables ---------------------------------------------------------*/
uint16_t a[2];

/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
void GPIO_Init(void);
void DMA_Init(void);
void ADC1_Init(void);
void TIM1_Init(void);

int main(void)
{
    /* Initialize system */
    SystemClock_Config();
    GPIO_Init();
    DMA_Init();
    ADC1_Init();
    TIM1_Init();

    // Start TIM1 PWM
    TIM1->CCR1 = 0;
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CCER |= TIM_CCER_CC1E;

    while (1)
    {
        // Start ADC conversion
        ADC1->CR2 |= ADC_CR2_SWSTART;
        while (!(DMA1->ISR & DMA_ISR_TCIF1)); // Wait for DMA transfer complete
        DMA1->IFCR |= DMA_IFCR_CTCIF1; // Clear DMA transfer complete flag

        // Read sensor values
        if (a[1] > 3000)
        {
            GPIOA->ODR |= GPIO_ODR_ODR4;
            TIM1->CCR1 = 75;
        }
        else if (a[1] < 1500)
        {
            GPIOA->ODR &= ~GPIO_ODR_ODR4;
            TIM1->CCR1 = 125;
        }

        if (a[0] > 3200)
        {
            GPIOA->ODR |= GPIO_ODR_ODR3;
        }
        else
        {
            GPIOA->ODR &= ~GPIO_ODR_ODR3;
        }

        if (GPIOA->IDR & GPIO_IDR_IDR2)
        {
            GPIOC->ODR &= ~GPIO_ODR_ODR13;
        }
        else
        {
            GPIOC->ODR |= GPIO_ODR_ODR13;
        }
    }
}

void SystemClock_Config(void)
{
    // Enable HSI
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // Configure PLL
    RCC->CFGR |= RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL9;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Select PLL as system clock source
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Update SystemCoreClock variable
    SystemCoreClockUpdate();
}

void GPIO_Init(void)
{
    // Enable GPIOA and GPIOC clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    // Configure PA1 as alternate function output push-pull for TIM1 CH1
    GPIOA->CRL &= ~GPIO_CRL_MODE1;
    GPIOA->CRL |= GPIO_CRL_MODE1_1 | GPIO_CRL_MODE1_0; // Output mode, max speed 50 MHz
    GPIOA->CRL &= ~GPIO_CRL_CNF1;
    GPIOA->CRL |= GPIO_CRL_CNF1_1; // Alternate function output push-pull

    // Configure PA2 as input floating
    GPIOA->CRL &= ~GPIO_CRL_MODE2;
    GPIOA->CRL &= ~GPIO_CRL_CNF2;
    GPIOA->CRL |= GPIO_CRL_CNF2_0; // Input floating

    // Configure PA3 and PA4 as general purpose output push-pull
    GPIOA->CRL &= ~GPIO_CRL_MODE3;
    GPIOA->CRL |= GPIO_CRL_MODE3_1; // Output mode, max speed 2 MHz
    GPIOA->CRL &= ~GPIO_CRL_CNF3;
    GPIOA->CRL &= ~GPIO_CRL_MODE4;
    GPIOA->CRL |= GPIO_CRL_MODE4_1; // Output mode, max speed 2 MHz
    GPIOA->CRL &= ~GPIO_CRL_CNF4;

    // Configure PC13 as general purpose output push-pull
    GPIOC->CRH &= ~GPIO_CRH_MODE13;
    GPIOC->CRH |= GPIO_CRH_MODE13_1; // Output mode, max speed 2 MHz
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
}

void DMA_Init(void)
{
    // Enable DMA1 clock
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    // Configure DMA for ADC1
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t)a;
    DMA1_Channel1->CNDTR = 2;
    DMA1_Channel1->CCR = DMA_CCR1_MINC | DMA_CCR1_PSIZE_0 | DMA_CCR1_MSIZE_0 | DMA_CCR1_CIRC | DMA_CCR1_TCIE;
    DMA1_Channel1->CCR |= DMA_CCR1_EN;
}

void ADC1_Init(void)
{
    // Enable ADC1 and GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN;

    // Configure PA0 as analog input
    GPIOA->CRL &= ~GPIO_CRL_MODE0;
    GPIOA->CRL &= ~GPIO_CRL_CNF0;

    // Configure ADC1
    ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC
    ADC1->SQR3 = (0 << 0) | (1 << 5); // Set ADC channels 0 and 1
    ADC1->SMPR2 |= ADC_SMPR2_SMP0; // Set sampling time for channel 0
    ADC1->SMPR2 |= ADC_SMPR2_SMP1; // Set sampling time for channel 1
    ADC1->CR1 |= ADC_CR1_SCAN; // Enable scan mode
    ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_CONT; // Enable DMA and continuous conversion mode
    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL); // Wait for calibration to finish
    ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC again
}

void TIM1_Init(void)
{
    // Enable TIM1 clock
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // Configure TIM1
    TIM1->PSC = 15; // Prescaler value
    TIM1->ARR = 9999; // Auto-reload value
    TIM1->CCR1 = 0; // Initial PWM value

    // Configure TIM1 CH1 as PWM mode 1
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->CR1 |= TIM_CR1_ARPE;
}

void Error_Handler(void)
{
    // Infinite loop in case of error
    while (1)
    {
    }
}
