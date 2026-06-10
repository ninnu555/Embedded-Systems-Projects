#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"

#define IMG_SIZE 3*128*128
#define HEADER_SIZE 15

unsigned char header[HEADER_SIZE];
unsigned char img[IMG_SIZE];

void receivePpmImg () {                             
  // Variabili globali (HEADER_SIZE e IMG_SIZE) possono essere sottointese come input se vengono usate nelle funzioni
  // Se si usano variabili globali (header e img in questo caso) possono essere non returnati in output
  for (int i=0; i < IMG_SIZE + HEADER_SIZE; i++) {
    if (i < HEADER_SIZE)
      header[i] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);             // fetching header
    else
      img[i - HEADER_SIZE] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);  // fetching image
  }
}

void performImgNegative (unsigned char* img) { 
  // puntatori globali devono per forza essere indicati come input in una funzione
  for (int i = 0; i < IMG_SIZE; i++) {     
    img[i] = 255 - img[i];    // performing negative (overwriting img)
  }
}

void transmitPpmImg (unsigned char* header, unsigned char* img) {
  for (int i=0; i < IMG_SIZE + HEADER_SIZE; i++) {
    if (i < HEADER_SIZE)
      XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, header[i]);  // sending header first
    else
      XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, img[i - HEADER_SIZE]); // then, sending processed image
  }
}


int main()
{
    init_platform();
    XUartPs Uart_1_PS;
    u16 DeviceId_1= XPAR_PS7_UART_1_DEVICE_ID;
    int Status_1;
    XUartPs_Config *Config_1;
    Config_1 = XUartPs_LookupConfig(DeviceId_1);
    if (NULL == Config_1) {
      return XST_FAILURE;
    }
    Status_1 = XUartPs_CfgInitialize(&Uart_1_PS, Config_1, Config_1->BaseAddress); //inizializzo la UART
    if (Status_1 != XST_SUCCESS) {
      return XST_FAILURE;
    }
    u32 BaudRate = (u32)115200;
    Status_1 = XUartPs_SetBaudRate(&Uart_1_PS, BaudRate); //setto il BaudRate = 115200
    if (Status_1 != (s32)XST_SUCCESS) {
      return XST_FAILURE;
    }

    
    // uP will receive image by laptop by UART
    receivePpmImg(); 
    
    // negative operation will be performed
    performImgNegative(img); 
    
    // uP will transmit processed image to laptop via UART
    transmitPpmImg(header, img);


    cleanup_platform();
    return 0;
}



//int start, stop;
// Xil_DCacheDisable();
//
// Xil_ICacheDisable();
// Xil_L2CacheDisable();
// start=Xil_In32(XPAR_GLOBAL_TMR_BASEADDR);
// for (int i=0;i<3*32*32;i++)
// 	image[i]=255-image[i];
// stop=Xil_In32(XPAR_GLOBAL_TMR_BASEADDR);
// xil_printf("Duration=%d \r\n", stop-start);
