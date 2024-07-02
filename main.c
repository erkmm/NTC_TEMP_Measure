#include <stdio.h>
#include <stdint.h>
#include "NUC029xAN.h"
#include "sys.h"
#include "clk.h"
#include "adc.h"
#include "fmc.h"
#include "uart.h"

#include <math.h>


#define PLL_CLOCK       50000000


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void);
void UART0_Init(void);
void ADC_FunctionTest();
void ADC_IRQHandler(void);
double NTC_Get_Temp();


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

volatile uint32_t g_u32AdcIntFlag;
volatile uint32_t i32BuiltInData = 3;
volatile uint32_t i32ConversionData;


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Enable ADC module clock */
    CLK_EnableModuleClock(ADC_MODULE);

    /* Select UART module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_PLL, CLK_CLKDIV_UART(1));

    /* ADC clock source is 22.1184MHz, set divider to 7, ADC clock is 22.1184/7 MHz */
    CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_CLKDIV_ADC(7));

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set P3 multi-function pins for UART0 RXD and TXD */
    SYS->P3_MFP &= ~(SYS_MFP_P30_Msk | SYS_MFP_P31_Msk);
    SYS->P3_MFP |= SYS_MFP_P30_RXD0 | SYS_MFP_P31_TXD0;

       /* Disable the P1.0 - P1.3 digital input path to avoid the leakage current */
    GPIO_DISABLE_DIGITAL_PATH(P1, 0xF);

    /* Configure the P1.0 - P1.3 ADC analog input pins */
    SYS->P1_MFP &= ~(SYS_MFP_P10_Msk | SYS_MFP_P11_Msk | SYS_MFP_P12_Msk | SYS_MFP_P13_Msk);
    SYS->P1_MFP |= SYS_MFP_P10_AIN0 | SYS_MFP_P11_AIN1 | SYS_MFP_P12_AIN2 | SYS_MFP_P13_AIN3 ;

}

/*---------------------------------------------------------------------------------------------------------*/
/* Init UART                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_Init()
{
    /* Reset IP */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

void ADC_FunctionTest()
{
    //printf("\n|                 ADC single cycle scan mode sample code               |\n");

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
		//printf("\nConversion result of channel 0: 0x%X (%d)\n", i32ConversionData, i32ConversionData);


    /* Enable FMC ISP function to read built-in band-gap A/D conversion result*/

    /* Use Conversion result of Band-gap to calculating AVdd 

    printf("      AVdd           i32BuiltInData                   \n");
    printf("   ---------- = -------------------------             \n");
    printf("      3072          i32ConversionData                 \n");
    printf("                                                      \n");
    printf("AVdd =  3072 * i32BuiltInData / i32ConversionData     \n\n");

    printf("Built-in band-gap A/D conversion result: 0x%X (%d) \n", i32BuiltInData, i32BuiltInData);
    printf("Conversion result of Band-gap:           0x%X (%d) \n\n", i32ConversionData, i32ConversionData);

    printf("AVdd = 3072 * %d / %d = %d mV \n\n", i32BuiltInData, i32ConversionData, 3072*i32BuiltInData/i32ConversionData);*/
}


void ADC_IRQHandler(void)
{
    g_u32AdcIntFlag = 1;
    ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT); /* Clear the A/D interrupt flag */
}


double NTC_Get_Temp(){
    double R0 = 10000.0;
    double B = 3435000.0;
    double K0 = 293.0;
    double AVdd, Vcc, id, R;
	  double result;
	
	
    double v1 = (4096 / (i32ConversionData - 1));
		R = R0 / v1;


	 //result = 1.0 / (1.0/K0 + log(R/R0)/B);
	
		double birBk = 1;
	         birBk /= K0;

	  double lr = log(R/R0);
		
		double r1 = lr / B;
		
		double r2 = birBk + r1;
		
		double r3 = 1;
		       r3 /= r2;
	 
	
		result = r3;
	

		printf("\n\n");
		printf("\nAdc   : %d",i32ConversionData);
		printf("\nV1    : %.2f",v1);
	  printf("\nR     : %.2f",(float) R);
	  printf("\nResult: %.2f",(float) result);
	
	
    return result;
}

void TimeDelay(){
	for (int t=0;t < 1000000;t++);
}

int32_t main(void)
{
	  double K;
		
    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();

    /* Init UART0 for printf */
    UART0_Init();

    printf("System clock rate: %d Hz\n", SystemCoreClock);

    /* ADC function test */
	 while(1){
    ADC_FunctionTest();

		 //i32ConversionData=2000;
		 
    K = NTC_Get_Temp();
	  //printf("\nfound Tempreture is %.2f\n",K);
		TimeDelay();
	 }

    /* Disable ADC IP clock */
    CLK_DisableModuleClock(ADC_MODULE);

    /* Disable External Interrupt */
    NVIC_DisableIRQ(ADC_IRQn);

    printf("Exit ADC sample code\n");

   
	 
	 
}

