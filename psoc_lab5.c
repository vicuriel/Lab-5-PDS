#include "project.h"
#include <stdio.h>
#define OFFSET 2.04
#define LONGITUD  512

/* Defines for DMA_1 */
#define DMA_1_BYTES_PER_BURST 2
#define DMA_1_REQUEST_PER_BURST 1
#define DMA_1_SRC_BASE (CYDEV_PERIPH_BASE)
#define DMA_1_DST_BASE (CYDEV_SRAM_BASE)

/* Add an explicit reference to the floating point printf library to allow
the usage of floating point conversion specifier */
#if defined (__GNUC__)
    asm (".global _printf_float");
#endif

int16 datos[LONGITUD];
float datos_float[LONGITUD];
float datos_filtrados[LONGITUD];

uint8 estado = 0u;

//uint8 flag_fin = 0u;

uint8 errorStatus = 0u;
uint k, j, i = 0;

//coeficientes del filtro alpha beta alpha,  las frecuencias de 0.2pi (1khz). Frecuencia de muestreo de 5khz
float h[] = { 1.6180,   -1.0000,    1.6180};

//coeficientes del filto fir pasa altos, frecuencia de corte en 0.4pi
//float u[] = {0.0060,    0.0133,   -0.0501,   -0.2598,    0.5951,   -0.2598,   -0.0501,    0.0133,    0.0060};

//coeficientes del filtro fir pasa bajos en 0.4pi(1kHz cuando se muestrea a 5kHz)
float u[] = {-0.0061,   -0.0136,    0.0512,    0.2657,    0.4057,    0.2657,    0.0512,   -0.0136,   -0.0061};

//coeficientes del filtro iir pasa banda  de 0.2pi a 0.4pi (de 500hz hasta 1kHz si la frecuencia de muestreo es de 5kHz)
float num[] = {0.0003,    0.0000,   -0.0015,    0.0000,    0.0029,    0.0000,   -0.0029,    0.0000,    0.0015,    0.0000,   -0.0003};
float den[] = {1.0000,   -5.5397,   16.3975,  -32.2094,   46.1195,  -49.7632,   41.0058,  -25.4542,   11.5138,   -3.4549,    0.5558};


/* Variable declarations for DMA_1 */
/* Move these variable declarations to the top of the function */
uint8 DMA_1_Chan;
uint8 DMA_1_TD[1];

char aux[20];

CY_ISR(Adquisicion);
CY_ISR(RxIsr);


void my_Start(void);
void dmaConfig(void);

int main(void)
{     
    my_Start();         /*Rutina para inicializar los componentes*/
    dmaConfig(); 
    
    //Punteros a la rutinas de interrupción  
    #if(INTERRUPT_CODE_ENABLED == ENABLED)
    isr_1_StartEx(Adquisicion);    
    isr_rx_StartEx(RxIsr);
    #endif /* INTERRUPT_CODE_ENABLED == ENABLED */
    
    CyDelay(50);  
    CyGlobalIntEnable; /* Enable global interrupts. */
   //Se habilita la interrupción del puerto serie
    isr_rx_Enable();
    //No se habilita la int de fin de conversión
    isr_1_Disable();
    
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    for(;;) 
    {
        switch(estado) {
            case '0': //es un cero
                //este es un estado vacio, default, no hacemos nada y el psoc queda colgado hasta recibir una orden por el uart
                break;
            
            case 'I': // se habilita la digitalizacion
                
                //Se habilita la digitalización y la Interrupcion del DAM
                ADC_SAR_1_Start(); //se habilita la digitalizacion del ADC
                isr_1_Enable(); //se pone en enable la interrupcion 
                
                estado = '0'; //cero
                break;
            
            case 'X':   //se filtra la senal
                //aca va codigo para filtrar
                    Led_Write(~Led_Read());
                // se convierten los datos recuperados del adc a voltios en formato float para procesarlos
                for(j=0;j<LONGITUD;j++){
                   datos_float[j] = ADC_SAR_1_CountsTo_Volts(datos[j]);
                    //Chart_1_Plot(ADC_SAR_1_CountsTo_Volts(datos[j]));
                }
                
                // filtro alpha beta alpha, se realiza la convolucion, atenua frecuencias en 0.4pi
                    
                    datos_filtrados[0] = 0;// estos se deben al retardo de muestras que se necesita
                    datos_filtrados[1] = 0;
                    for(j=2;j<LONGITUD;j++){
                        datos_filtrados[j] = h[0]*datos_float[j] + h[1]*datos_float[j-1] + h[2]*datos_float[j-2];
                    }
                    
                estado = '0'; //cero
                break;
            
            case 'P':   //se envia al psoc la senal original
                //aca va codigo para enviar senal original por uart
                    // se convierten los datos recuperados del adc a voltios en formato float para procesarlos
                for(j=0;j<sizeof(datos_float)/sizeof(datos_float[0]);j++){
                   // printf("%f\r\n",datos_float[j]);
                    Chart_1_Plot(datos_float[j]);
                }
                
                estado = '0'; //cero
            break;
            
            case 'F':   //se envia al psoc ls senal filtada
                //aca va codigo para enviar senal filtrada por uart
                for(j=0;j<sizeof(datos_filtrados)/sizeof(datos_filtrados[0]);j++){
                   // printf("%f\r\n",datos_filtrados[j]);
                    Chart_1_Plot(datos_filtrados[j]);
                }
                
                estado = '0'; // cero
            break;
        }
    }
}// fin main

void my_Start(){
//Se incializan los componentes
    ADC_SAR_1_Start();
    Clock_1_Start();
    Opamp_1_Start();
    Opamp_2_Start();
    PGA_1_Start();
//Se inicializa el puerto serial    
    UART_Start(); 
    //Se para el convertidor
    ADC_SAR_1_Stop();
}

//Las rutinas de interrupción que se utilizan
CY_ISR(Adquisicion){   
    ADC_SAR_1_Stop();
    estado = 'X'; 
    //CyDmaChEnable(DMA_1_Chan, 1);//Se habilita de vuelta el canal
    isr_1_ClearPending(); 
}

CY_ISR(RxIsr)
{
    uint8 rxStatus;         
    uint8 rxData;           
    
    do
    {
        /* Read receiver status register */
        rxStatus = UART_RXSTATUS_REG;

        if((rxStatus & (UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR |
                        UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN)) != 0u)
        {
            /* ERROR handling. */
            errorStatus |= rxStatus & ( UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR | 
                                        UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN);
        }
        
        if((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
        {
            /* Read data from the RX data register */
            rxData = UART_RXDATA_REG;
            if (rxData == 'I'){
               // Led_Write(~Led_Read());
                estado = 'I';
            }else if (rxData == 'P'){
                estado = 'P';
            }else if (rxData == 'F'){
                estado = 'F';
            }else{
                estado = '0';   
            }
            
            if(errorStatus == 0u)
            {
                /* Send data backward */
                //UART_TXDATA_REG = rxData;
            }
        }
    }while((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u);
    //isr_rx_ClearPending(); 
}

void dmaConfig(void){

/* DMA Configuration for DMA_1 */
DMA_1_Chan = DMA_1_DmaInitialize(DMA_1_BYTES_PER_BURST, DMA_1_REQUEST_PER_BURST, 
    HI16(DMA_1_SRC_BASE), HI16(DMA_1_DST_BASE));
DMA_1_TD[0] = CyDmaTdAllocate();
CyDmaTdSetConfiguration(DMA_1_TD[0], 2*LONGITUD, DMA_1_TD[0], DMA_1__TD_TERMOUT_EN | CY_DMA_TD_INC_DST_ADR);
CyDmaTdSetAddress(DMA_1_TD[0], LO16((uint32)ADC_SAR_1_SAR_WRK0_PTR), LO16((uint32)datos));
CyDmaChSetInitialTd(DMA_1_Chan, DMA_1_TD[0]);
CyDmaChEnable(DMA_1_Chan, 1);

}

/* [] END OF FILE */