  #include <stdio.h>
  #include <stdlib.h>
  #include "platform.h"
  #include "xil_printf.h"
  #include "xuartps.h"

  #define MAX_HEADER_SIZE 17

  unsigned char header[MAX_HEADER_SIZE];
  
  void get_Img (int img_size, unsigned char* img) { 
  // receive image from laptop by UART
    for (int i=0; i < img_size; i++) {
      img[i] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
    }
  }

  int get_HeaderDim (unsigned char* header) {
  // read the header and extract its dimensions (MAX_HEADER_SIZE=17 ->maximum allowed dimensions up to 9999x9999)
    int i = 0;
    int NL_counter = 0;                     // initializing New Line character counter
    while (NL_counter < 3) {                // exit when the end of the header is encountered (third New Line character)
      if (i >= MAX_HEADER_SIZE){
      // header too big than allowed
        return -1;
      }
      header[i] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
      if (header[i] == '\n') {
      // New Line counter
        NL_counter++;
      }
      i++;  // i = true_header_size
    }
    return i; // returns the actual size of the header
  }

  int get_ImgDim (unsigned char* header) {
    // scanning and fetching image dimensions
    int i = 0;
    while (header[i] != '\n') i++;      // skim the first field (P6)

    i++;    // skip the newline character - point to the first figure of width

    int width = 0;
    while (header[i] >= '0' && header[i] <= '9') {   // stay here until a non-numerical character (a Space) is detected
      width = width * 10 + (header[i] - '0');       // performing width conversion from a set of characters to an int number
      i++;
    }

    i++;    // skip the space character - point to the first figure of height

    int height = 0;
    while (header[i] >= '0' && header[i] <= '9') {    // stay here until a non-numerical character (a New Line) is detected
      height = height * 10 + (header[i] - '0');     // performing width conversion from a set of characters to an int number
      i++;
    }
    return (3*width*height);
  }

  void transmit_PpmImg (int img_size, int header_size, unsigned char* header, unsigned char* img) {
  // transmit image and its header together
    for (int i=0; i < img_size + header_size; i++) {
      if (i < header_size)
        XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, header[i]);  // sending header first
      else
        XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, img[i - header_size]); // then, sending processed image
    }
  }

  void perform_HistStretchAlg (int img_size, unsigned char* img) {

    float IntLev_max = 0;
    for (int i = 0; i < img_size; i++) {
    // compute the maximum intensity level of the image
    if (IntLev_max < img[i])
      IntLev_max = img[i];
    }

    float IntLev_min = IntLev_max;
    for (int i = 0; i < img_size; i++) {
    // compute the minimum intensity level of the image
      if (IntLev_min > img[i])
        IntLev_min = img[i];
    }

    float scale = 255.0f / (IntLev_max - IntLev_min); 

    for (int i = 0; i < img_size; i++) {     // preserving the header
        img[i] = (img[i] - IntLev_min) * scale; // performing algorithm
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

    // get header size
    int header_size = get_HeaderDim(header);

    // get image size
    int img_size = get_ImgDim(header);

    // allocate sufficient space in the heap for the image we are about to receive (we now know the dimensions)
    unsigned char *img = malloc(img_size);

    // receive image from laptop via UART
    get_Img(img_size, img);

    // performing Histogram Stretching Algorithm
    perform_HistStretchAlg(img_size, img);

    // transmit both header and processed image
    transmit_PpmImg(img_size, header_size, header, img);
    free(img);

    cleanup_platform();
    return 0;
  }
