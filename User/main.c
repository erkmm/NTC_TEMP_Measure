/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 14/10/01 10:36a $
 * @brief    Template project for NUC029 series MCU
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include "NUC029xAN.h"
#include "sys.h"
#include "clk.h"
#include "adc.h"
#include "fmc.h"
#include "uart.h"
#include <math.h>


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/

void SYS_Init(void);
void UART0_Init(void);
void ADC_FunctionTest();
void ADC_IRQHandler(void);
double NTC_Get_Temp();
//changed2

/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

volatile uint32_t g_u32AdcIntFlag;
volatile uint32_t i32BuiltInData;
volatile uint32_t i32ConversionData;


void SYS_Init(void)
{
    //
	/* Unlock protected registers */
    SYS_UnlockReg();

// ------------------------
// CLOCK CONFIGURE
/********************
    /* If the macros do not exist in your project, please refer to the related clk.h in Header folder of the tool package */
    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCON_OSC10K_EN_Msk|CLK_PWRCON_OSC22M_EN_Msk|CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC10K_STB_Msk|CLK_CLKSTATUS_OSC22M_STB_Msk|CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Disable PLL first to avoid unstable when setting PLL */
    CLK_DisablePLL();

    /* Set PLL frequency */
    CLK->PLLCON = (CLK->PLLCON & ~(0x000FFFFFUL)) | 0x0000C22EUL;

    /* Waiting for PLL ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_PLL_STB_Msk);

    /* Set HCLK clock */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));


    /* Enable IP clock */
    CLK_EnableModuleClock(ADC_MODULE);
    CLK_EnableModuleClock(EBI_MODULE);
    CLK_EnableModuleClock(ISP_MODULE);
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(WDT_MODULE);
    CLK_EnableModuleClock(WWDT_MODULE);

    /* Set IP clock */
    CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_CLKDIV_ADC(1));
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HIRC, CLK_CLKDIV_UART(1));
    CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDT_S_LIRC, MODULE_NoMsk);
    CLK_SetModuleClock(WWDT_MODULE, CLK_CLKSEL2_WWDT_S_HCLK_DIV2048, MODULE_NoMsk);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();


// ------------------------
// PIN CONFIGURE
/* Set P3 multi-function pins for UART0 RXD and TXD  */
    SYS->P0_MFP  = 0x00000000;
    SYS->P1_MFP |= (SYS_MFP_P10_AIN0);
    SYS->P2_MFP  = 0x00000000;
    SYS->P3_MFP |= (SYS_MFP_P30_RXD |SYS_MFP_P31_TXD);
    SYS->P4_MFP |= (SYS_MFP_P47_ICE_DAT | SYS_MFP_P46_ICE_CLK);
    /* Lock protected registers */

    SYS_LockReg();

}

void UART0_Init()
{
    /* Reset IP */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

void ADC_FunctionTest()
{
	/* Enable ADC converter */

    ADC_POWER_ON(ADC);

    /* Set input mode as single-end, continuous mode, and select channel P1.0 (band-gap voltage) */
    ADC_Open(ADC, ADC_ADCR_DIFFEN_SINGLE_END, ADC_ADCR_ADMD_SINGLE, 0x1);


    /* Clear the A/D interrupt flag for safe */
    ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT);

    ADC_START_CONV(ADC);

    /* Wait ADC conversion done */
    while(!ADC_GET_INT_FLAG(ADC, ADC_ADF_INT));

    /* Disable the A/D interrupt */

    /* Get the conversion result of the channel P1.0 */
    i32ConversionData = ADC_GET_CONVERSION_DATA(ADC, 0);
}

void ADC_IRQHandler(void)
{
    g_u32AdcIntFlag = 1;
    ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT); /* Clear the A/D interrupt flag */
}


void TimeDelay(){
	for (int t=0;t < 1000000;t++);
}

double NTC_Get_Temp(){
    double R0 = 10000.0;
    double B = 3435.0;
    double K0 = 298.0;
    double res, felc, felb, fel;
	double result;


    float v1 = 4096.0-(i32ConversionData);
	float v2 = v1 /(i32ConversionData);
	fel = log(v2);
	felb=fel/B;
	felc=1-K0*felb;

	result=K0/felc;
	float R= R0/v2;

		printf("\n\n");
		printf("\n ADC     : %d",i32ConversionData);
		printf("\n V2      : %.4f",(float) v2);
	    printf("\n fel     : %.4f",(float) fel);
	    printf("\n R       : %.4f",(float) R);
	    printf("\n Result  : %.2f",(float) result);
		printf("\n RoomTemp: %.2f C",(float) result-273);

    return result;
}

int main()
{
 /* Init System, IP clock and multi-function I/O. */
 SYS_Init();

  /* Init UART0 for printf */
  UART0_Init();

  printf("System clock rate: %d Hz\n", SystemCoreClock);

  /* ADC function test */
 while(1){
  ADC_FunctionTest();

  NTC_Get_Temp();
  TimeDelay();
 }

  /* Disable ADC IP clock */
  CLK_DisableModuleClock(ADC_MODULE);

   /* Disable External Interrupt */
   NVIC_DisableIRQ(ADC_IRQn);

   printf("Exit ADC sample code\n");
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
