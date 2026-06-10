#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"
// #include "weights.h"
#include "test_images.h"
#include <xtime_l.h>
#include <time.h>

#include <math.h>

#define n_bias0 64
#define n_weights0 50176
#define n_bias1 10
#define n_weights1 640

#define IMG_SIZE 784 // 1568/2

typedef signed char DATA;

short int gemm0_bias[n_bias0];
short int gemm0_weights[n_weights0];
short int gemm1_bias[n_bias1];
short int gemm1_weights[n_weights1];

DATA opt_gemm0_bias[n_bias0];
DATA opt_gemm0_weights[n_weights0];
DATA opt_gemm1_bias[n_bias1];
DATA opt_gemm1_weights[n_weights1];

DATA output_gemm0[64];
DATA  input_gemm1[64];
DATA output_gemm1[10];

unsigned int get_gemm0_bias_time[2];
unsigned int get_gemm0_weights_time[2];
unsigned int get_gemm1_bias_time[2];
unsigned int get_gemm1_weights_time[2];
unsigned int sending_time[2];
unsigned int loop_iter_time[2];
unsigned int get_img_time[2];
unsigned int first_FC_forward_time[2];
unsigned int second_FC_forward_time[2];
unsigned int first_relu_forward_time[2];
unsigned int processing_time[2];  

short int img[IMG_SIZE];
DATA  opt_img[IMG_SIZE];

int offset = 2; // needed for accuracy

#define FIXED2FLOAT(a, qf) (((float) (a)) / (1<<qf))
#define FLOAT2FIXED(a, qf) ((short int) round((a) * (1<<qf)))

#define _MAX_ 127
#define _MIN_ -128

void FC_forward(DATA* input, DATA* output, int in_s, int out_s, DATA* weights, DATA* bias, int qf) ;
static inline long long int saturate(long long int mac);
static inline void relu_forward(DATA* input, DATA* output, int size);
int resultsProcessing(DATA* results, int size);

void readFromUart (int size, short int* data) {
  // receiving image/weights/biases from computer by UART
    for (int i=0; i < size; i++) {
      unsigned char data1, data2; // data1 and data2 are 1byte sized
      short int data16;
      data1 = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
      data2 = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
      data16 = (data2<<8) + data1;

      data[i] = data16;
    }
  }

void sendOptimizedImageUart (int size, short int* data) {
  // sending back the optimized image to the computer
  for (int i=0; i < size; i++) {
    XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, (DATA) ((data[i]+8)>>4));  // Q8.8 to Q6.2 conversion
  }
}

void readImageFromUart_opt (int size, DATA* data) {
  // recceiving optimized image from computer by UART
    for (int i=0; i < size; i++) {
      data[i] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
    }
  }

void sendOptimizedWeightsBiasUart (int size, short int* data) {
  for (int i=0; i < size; i++) {
    XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, (DATA) ((data[i]+8)>>4)); // Q8.8 to Q6.2 conversion
  }
}

void readWeightsBiasFromUart_opt (int size, DATA* data) {
    for (int i=0; i < size; i++) {
      data[i] = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
    }
  }

int count_max_sat = 0; // how many times saturation to max happened
int count_min_sat = 0; // how many times saturation to min happened
int total_calls = 0;   // total number of MAC operations

int main(){
  init_platform();

  //UART setup
  XUartPs Uart_1_PS;
  u16 DeviceId_1= XPAR_PS7_UART_1_DEVICE_ID;
  int Status_1;
  XUartPs_Config *Config_1;
  Config_1 = XUartPs_LookupConfig(DeviceId_1);
  if (NULL == Config_1) {
    return XST_FAILURE;
  }
  /*the default configuration is stored in Config and it can be used to initialize the controller */
  Status_1 = XUartPs_CfgInitialize(&Uart_1_PS, Config_1, Config_1->BaseAddress);
  if (Status_1 != XST_SUCCESS) {
    return XST_FAILURE;
  }
  // Set the BAUD rate
  u32 BaudRate = (u32)115200;
  Status_1 = XUartPs_SetBaudRate(&Uart_1_PS, BaudRate);
  if (Status_1 != (s32)XST_SUCCESS) {
    return XST_FAILURE;
  }
  //END UART SETUP
  xil_printf ("Started\n");

    ///////////////////////////////////////////////////////
    // This code allows to perform optimization of weights and biases on FPGA and send them back to the computer via RealTerm
    // Comment if you already have the optimized weights and bias from the computer
    //////////////////////////////////////////////////////
    // print("Open Capture AND Insert weights and biases\n\r");
    // readFromUart (n_bias0, gemm0_bias);
    // sendOptimizedWeightsBiasUart (n_bias0, gemm0_bias);
    // // print("Received gemm0_bias and sent back optimized\n\r");
    // // print("OPEN CAPTURE AND Insert gemm0_weights\n\r");
    // readFromUart (n_weights0, gemm0_weights);
    // sendOptimizedWeightsBiasUart (n_weights0, gemm0_weights);
    // // print("Received gemm0_weights and sent back optimized\n\r");
    // // print("OPEN CAPTURE AND Insert gemm1_bias\n\r");
    // readFromUart (n_bias1, gemm1_bias);
    // sendOptimizedWeightsBiasUart (n_bias1, gemm1_bias);
    // // print("Received gemm1_bias and sent back optimized\n\r");
    // // print("OPEN CAPTURE AND Insert gemm1_weights\n\r");
    // readFromUart (n_weights1, gemm1_weights);
    // sendOptimizedWeightsBiasUart (n_weights1, gemm1_weights);
    // // print("Received gemm1_weights and sent back optimized\n\r");
    ////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    // This code allows to upload the optimized weights and biases via RealTerm to FPGA
    ///////////////////////////////////////////////////////////
    // print("Insert optimized gemm0_bias\n\r");
    get_gemm0_bias_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      readWeightsBiasFromUart_opt (n_bias0, opt_gemm0_bias);
    get_gemm0_bias_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    // print("Received optimized gemm0_bias\n\r");
    // print("Insert optimized gemm0_weights\n\r");
    get_gemm0_weights_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      readWeightsBiasFromUart_opt (n_weights0, opt_gemm0_weights);
    get_gemm0_weights_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    // print("Received optimized gemm0_weights\n\r");
    // print("Insert optimized gemm1_bias\n\r");
    get_gemm1_bias_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      readWeightsBiasFromUart_opt (n_bias1, opt_gemm1_bias);
    get_gemm1_bias_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    // print("Received optimized gemm1_bias\n\r");
    // print("Insert optimized gemm1_weights\n\r");
    get_gemm1_weights_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      readWeightsBiasFromUart_opt (n_weights1, opt_gemm1_weights);
    get_gemm1_weights_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    // print("Received optimized gemm1_weights\n\r");
    ///////////////////////////////////////////////////////////
  
  while (1) {
    /////////////////////////////////////////////////////
    // // This code allows to receive the image from UART, optimize it on FPGA and send it back to the computer
    // // Comment if you want already have the optimized image from the computer
    /////////////////////////////////////////////////////
    // // print("Open CAPTURE\r\n");
    // // print("Waiting for the image to convert...\n\r");
    // // Receive img on FPGA
    // readFromUart (IMG_SIZE, img);
    // // Send back optimized image to Computer
    // sendOptimizedImageUart (IMG_SIZE, img);
    ////////////////////////////////////////////////////

    ///////////////////////////////////////////////////
    // Performing DNN and timing each part
    ///////////////////////////////////////////////////
    while (!XUartPs_IsReceiveData(XPAR_PS7_UART_1_BASEADDR)); // busy wait - start annotating time only when data is sent
    loop_iter_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    
    // Send image
    get_img_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET); 
      readImageFromUart_opt (IMG_SIZE, opt_img);
    get_img_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

    
    first_FC_forward_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      // we'll use the 8 bit wide optimized image for the DNN (opt_img)
	    FC_forward(opt_img, output_gemm0, 784, 64, opt_gemm0_weights, opt_gemm0_bias, 4);
    first_FC_forward_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

    first_relu_forward_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
	    relu_forward(output_gemm0, input_gemm1, 64);
    first_relu_forward_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

    second_FC_forward_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
	    FC_forward(input_gemm1, output_gemm1, 64, 10, opt_gemm1_weights, opt_gemm1_bias, 4);
    second_FC_forward_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

    processing_time[0] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
	    resultsProcessing(output_gemm1, 10);
    processing_time[1] = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

      loop_iter_time[1] =  Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
      xil_printf("=== TIMING REPORT ===\n\r");
      xil_printf("get_gemm0_bias_time: %u\r\n", get_gemm0_bias_time[1]-get_gemm0_bias_time[0]);
      xil_printf("get_gemm0_weights_time: %u\r\n", get_gemm0_weights_time[1]-get_gemm0_weights_time[0]);
      xil_printf("get_gemm1_bias_time: %u\r\n", get_gemm1_bias_time[1]-get_gemm1_bias_time[0]);
      xil_printf("get_gemm1_weights_time: %u\r\n", get_gemm1_weights_time[1]-get_gemm1_weights_time[0]);
      xil_printf("duration loop iteration: %u\r\n", loop_iter_time[1]-loop_iter_time[0]);
      xil_printf("duration flattening: %u\r\n", get_img_time[1]-get_img_time[0]);
      xil_printf("duration first FC forward: %u\r\n", first_FC_forward_time[1]-first_FC_forward_time[0]);
      xil_printf("duration first relu forward: %u\r\n", first_relu_forward_time[1]-first_relu_forward_time[0]);
      xil_printf("duration second FC forward: %u\r\n", second_FC_forward_time[1]-second_FC_forward_time[0]);
      xil_printf("duration results processing: %u\r\n", processing_time[1]-processing_time[0]);
      /////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////
      // Saturation stats
      xil_printf("--- SATURATION STATS ---\n\r");
      xil_printf("Total operations: %d\n\r", total_calls);
      xil_printf("Saturated MAX (+127): %d\n\r", count_max_sat);
      xil_printf("Saturated MIN (-128): %d\n\r", count_min_sat);

      // Loss rate
      int total_sat = count_max_sat + count_min_sat;
      xil_printf("Saturation Rate: %d%%\n\r", (total_sat * 100) / total_calls);

      // Reset counters for next image
      count_max_sat = 0;
      count_min_sat = 0;
      total_calls = 0;
      /////////////////////////////////////////////////////////

  }

    cleanup_platform();
    return 0;
}


void FC_forward(DATA* input, DATA* output, int in_s, int out_s, DATA* weights, DATA* bias, int qf) {
	// NOTE return W * x
	int hkern = 0;
	int wkern = 0;
	long long int mac = 0;
	DATA current = 0;

	for (hkern = 0; hkern < out_s; hkern++) {   // scrolling output neurons
		mac = ((long long int)bias[hkern]) << qf; // initilizing mac with bias and adjusting bias precision
		for (wkern = 0; wkern < in_s; wkern++) {  // scrolling input neurons
			current = input[wkern];
			mac += current * weights[hkern*in_s + wkern];
		}
		output[hkern] = (DATA)saturate((mac >> (qf)));  // output with original precision
	}
}

static inline long long int saturate(long long int mac)
{
  total_calls++;

	if(mac > _MAX_) {
		// printf("[WARNING] Saturation.mac: %lld -> %llx _MAX_: %d  _MIN_: %d  res: %d\n",  mac, mac, _MAX_, _MIN_, _MAX_);
		count_max_sat++;
    return _MAX_;
	}

	if(mac < _MIN_){
		// printf( "[WARNING] Saturation. mac: %lld -> %llx _MAX_: %d  _MIN_: %d  res: %d\n",  mac, mac, _MAX_, _MIN_, _MIN_);
		count_min_sat++;
		return _MIN_;
	}

	//printf("mac: %lld -> %llx _MAX_: %lld  _MIN_: %lld  res: %lld\n", mac, mac, _MAX_, _MIN_, mac);
    return mac;

}

static inline void relu_forward(DATA* input, DATA* output, int size) {
	int i = 0;
	for(i = 0; i < size; i++) {
		DATA v = input[i];
		v = v > 0 ? v : 0;
		output[i] = v;
	}
}

#define SIZEWA 10
int resultsProcessing(DATA* results, int size){
//What do you want to do with the results of the CNN? Here is the place where you should put the classifier or the detection (see YOLO detection for example)
//The simplest classifier is a maximum search for the results which returns the index value of the maximum

 char *labels[10]={"digit 0", "digit 1", "digit 2", "digit 3", "digit 4", "digit 5", "digit 6", "digit 7", "digit 8", "digit 9"};

  int size_wa = SIZEWA;
  float  r[SIZEWA];
  int  c[SIZEWA];
  float results_float[SIZEWA];
  float sum=0.0;
  DATA max=0;
  int max_i;
  for (int i =0;i<size_wa;i++){
      results_float[i] = FIXED2FLOAT(results[i], 4);
    int n;
    if (results[i]>0)
      n=results[i];
    else
      n=-results[i];
    if (n>max){
      max=n;
      max_i=i;
    }
  }
  for (int i =0;i<size_wa;i++)
    sum+=exp(results_float[i]);

  for (int i =0;i<size_wa;i++){
    r[i]=exp(results_float[i]) / sum;
    c[i]=i;
  }
  for (int i =0;i<size_wa;i++){
    for (int j =i;j<size_wa;j++){
      if (r[j]>r[i]){
        float t= r[j];
        r[j]=r[i];
        r[i]=t;
        int tc= c[j];
        c[j]=c[i];
        c[i]=tc;
      }
    }
  }
  int top0=0;
  float topval=results_float[0];
  for (int i =1;i<size_wa;i++){
    if (results_float[i]>topval){
      top0=i;
      topval=results_float[i];
    }
  }
  //xil_printf("\n\n");
  for (int i =0;i<5;i++){
//	  xil_printf("            TOP %d: [%d] %s   \n",i, c[i], labels[c[i]]);
  }
  xil_printf("max= %x \n\r",top0);
  return top0;
}
