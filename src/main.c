#include "stm32f4xx.h"
#include <stdint.h>

// ADC configuration
#define ADC_REFERENCE_MV 3300UL
#define ADC_MAX_COUNT    4095UL

#define ADC_EOC          (1UL << 1)
#define ADC_ADON         (1UL << 0)
#define ADC_SWSTART      (1UL << 30)
#define ADC_SCAN         (1UL << 8)
#define ADC_EOCS         (1UL << 10)

// Triple regular simultaneous mode
#define ADC_TRIPLE_MODE  0x16UL

// Raw ADC values: range 0 to 4095
volatile uint32_t voltage_adc = 0;
volatile uint32_t current_adc = 0;
volatile uint32_t temperature1_adc = 0;
volatile uint32_t temperature2_adc = 0;
volatile uint32_t temperature3_adc = 0;
volatile uint32_t temperature4_adc = 0;

// Sensor output voltages in millivolts
volatile uint32_t voltage_sensor_mV = 0;
volatile uint32_t current_sensor_mV = 0;
volatile uint32_t temperature1_sensor_mV = 0;
volatile uint32_t temperature2_sensor_mV = 0;
volatile uint32_t temperature3_sensor_mV = 0;
volatile uint32_t temperature4_sensor_mV = 0;

static void ADC_SetSampleTime(ADC_TypeDef *ADCx, uint32_t channel)
{
    uint32_t shift;

    if (channel <= 9UL) {
        shift = channel * 3UL;
        ADCx->SMPR2 &= ~(7UL << shift);
        ADCx->SMPR2 |=  (7UL << shift);
    } else {
        shift = (channel - 10UL) * 3UL;
        ADCx->SMPR1 &= ~(7UL << shift);
        ADCx->SMPR1 |=  (7UL << shift);
    }
}

static void ADC_ConfigureTwoChannels(ADC_TypeDef *ADCx,
                                     uint32_t channel1,
                                     uint32_t channel2)
{
    ADCx->CR1 = ADC_SCAN;
    ADCx->CR2 = ADC_EOCS;
    ADCx->SQR1 = (1UL << 20); // Two conversions: L = 2 - 1
    ADCx->SQR2 = 0;
    ADCx->SQR3 = (channel1 & 0x1FUL)
               | ((channel2 & 0x1FUL) << 5);

    ADC_SetSampleTime(ADCx, channel1);
    ADC_SetSampleTime(ADCx, channel2);
}

static void ADC123_Init(void)
{
    RCC->AHB1ENR |= (1UL << 0) | (1UL << 1) | (1UL << 2);
    RCC->APB2ENR |= (1UL << 8) | (1UL << 9) | (1UL << 10);
    (void)RCC->AHB1ENR;
    (void)RCC->APB2ENR;

    // PA1, PA2, PA3: analog inputs
    GPIOA->MODER &= ~(0xFCUL);
    GPIOA->MODER |=  (0xFCUL);
    GPIOA->PUPDR &= ~(0xFCUL);

    // PB0, PB1: analog inputs
    GPIOB->MODER &= ~(0x0FUL);
    GPIOB->MODER |=  (0x0FUL);
    GPIOB->PUPDR &= ~(0x0FUL);

    // PC1: analog input
    GPIOC->MODER &= ~(3UL << 2);
    GPIOC->MODER |=  (3UL << 2);
    GPIOC->PUPDR &= ~(3UL << 2);

    // ADC common control register (ADC_CCR)
    volatile uint32_t *ADC_CCR = (volatile uint32_t *)0x40012304UL;
    *ADC_CCR &= ~((0x1FUL) | (3UL << 14) | (1UL << 13));
    *ADC_CCR &= ~(3UL << 16);
    *ADC_CCR |= (1UL << 16); // PCLK2 / 4

    // ADC1: CH1/PA1 voltage; CH8/PB0 temperature 1
    // ADC2: CH2/PA2 current; CH9/PB1 temperature 2
    // ADC3: CH3/PA3 temperature 3; CH11/PC1 temperature 4
    ADC_ConfigureTwoChannels(ADC1, 1, 8);
    ADC_ConfigureTwoChannels(ADC2, 2, 9);
    ADC_ConfigureTwoChannels(ADC3, 3, 11);

    *ADC_CCR = (*ADC_CCR & ~(0x1FUL)) | ADC_TRIPLE_MODE;

    ADC1->CR2 |= ADC_ADON;
    ADC2->CR2 |= ADC_ADON;
    ADC3->CR2 |= ADC_ADON;
}

static void ADC_ReadSixParallel(void)
{
    uint32_t adc1_group1, adc2_group1, adc3_group1;
    uint32_t adc1_group2, adc2_group2, adc3_group2;

    ADC1->SR = 0;
    ADC2->SR = 0;
    ADC3->SR = 0;

    // Group 1: first rank across ADC1/ADC2/ADC3
    ADC1->CR2 |= ADC_SWSTART;

    while (((ADC1->SR & ADC_EOC) == 0) ||
           ((ADC2->SR & ADC_EOC) == 0) ||
           ((ADC3->SR & ADC_EOC) == 0)) {
        // Wait for first-rank conversions.
    }

    adc1_group1 = ADC1->DR & 0xFFFUL;
    adc2_group1 = ADC2->DR & 0xFFFUL;
    adc3_group1 = ADC3->DR & 0xFFFUL;

    // Group 2: second rank across ADC1/ADC2/ADC3
    while (((ADC1->SR & ADC_EOC) == 0) ||
           ((ADC2->SR & ADC_EOC) == 0) ||
           ((ADC3->SR & ADC_EOC) == 0)) {
        // Wait for second-rank conversions.
    }

    adc1_group2 = ADC1->DR & 0xFFFUL;
    adc2_group2 = ADC2->DR & 0xFFFUL;
    adc3_group2 = ADC3->DR & 0xFFFUL;

    voltage_adc     = adc1_group1;
    current_adc     = adc2_group1;
    temperature3_adc = adc3_group1;
    temperature1_adc = adc1_group2;
    temperature2_adc = adc2_group2;
    temperature4_adc = adc3_group2;
}

int main(void)
{
    ADC123_Init();

    while (1) {
        ADC_ReadSixParallel();

        voltage_sensor_mV = (voltage_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;
        current_sensor_mV = (current_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;

        temperature1_sensor_mV =
            (temperature1_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;
        temperature2_sensor_mV =
            (temperature2_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;
        temperature3_sensor_mV =
            (temperature3_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;
        temperature4_sensor_mV =
            (temperature4_adc * ADC_REFERENCE_MV) / ADC_MAX_COUNT;
    }
}
