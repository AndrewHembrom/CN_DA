#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "WiFiHelper.h"
#include "dspHelper.h"

i2s_chan_handle_t rx_handle_1;
QueueHandle_t buffQueue_1;

i2s_chan_handle_t rx_handle_2;
QueueHandle_t buffQueue_2;

int Largestflag = 0;
float largest = 0;

#define _SAMPLE_RATE 8000

static void readTask_1 (void *args) {
    int32_t raw_buf[N_SAMPLES];
    int16_t output_buf[N_SAMPLES];
    size_t r_bytes = 0;
    int samples_read = 0;

    buffQueue_1 = xQueueCreate(2, sizeof(output_buf));

    while(1) {
        if(i2s_channel_read(rx_handle_1, raw_buf, sizeof(int32_t) * N_SAMPLES, &r_bytes, portMAX_DELAY) == ESP_OK) {
            samples_read = r_bytes/sizeof(int32_t);
            int sample_index = 0;
            for (int i = 0; i < samples_read; i++) {
                output_buf[sample_index++] = (raw_buf[i] & 0xFFFFFFF0) >> 11;
            }
            xQueueSend(buffQueue_1, (void*)output_buf, (TickType_t) 0);
        }
    }
}

static void printTask_1 (void *args) {
    int16_t rxBuffer[N_SAMPLES];
    float rBuff[N_SAMPLES/2];
    float lBuff[N_SAMPLES/2];
    
    while(1) {
        if(buffQueue_1 != 0) {
            if(xQueueReceive(buffQueue_1, &(rxBuffer), (TickType_t)10)){

                float* outputBuffer = (float*) calloc(N_SAMPLES, sizeof(float));

                buffSplit(rxBuffer, lBuff, rBuff);
                HPF(lBuff, rBuff, 50.0/_SAMPLE_RATE, 10);
                LPF(lBuff, rBuff, 400.0/_SAMPLE_RATE, 10);
                
                int lcount = 0;
                int rcount = 0;
                for(int i = 0; i < N_SAMPLES; i++) {
                    if(i < N_SAMPLES/2) {
                        outputBuffer[i] = lBuff[lcount++];
                    } else {
                        outputBuffer[i] = rBuff[rcount++];
                    }

                    if(outputBuffer[i] > largest && (Largestflag < 4096)) {
                        largest = outputBuffer[i];
                        Largestflag++;
                    }
                }

                for(int i = 0; i < N_SAMPLES; i++) {
                    outputBuffer[i] /= largest;
                }

                // ESP_LOGI(TAG_UDP, "UDP Packet Content:");
                // for (int i = 0; i < N_SAMPLES; i++) {
                //     ESP_LOGI(TAG_UDP, "[%d]: %f", i, outputBuffer[i]);
                // }

                // Transmit the UDP packet
                int err = sendto(sock, outputBuffer, (N_SAMPLES) * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                ESP_LOGI(TAG_UDP, "Packet Transmitted");
                if (err < 0) {
                    ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
                }

                // int err = sendto(sock, outputBuffer, (N_SAMPLES) * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                // if (err < 0) {
                //     ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
                // }

                free(outputBuffer);
            }
        }
    }
}

void app_main(void) {

    initNVS();

    WiFiConnectHelper();

    initUDP();

    // I2S Initialization
    i2s_chan_config_t rx_chan_cfg_1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    i2s_new_channel(&rx_chan_cfg_1, NULL, &rx_handle_1);

    i2s_std_config_t rx_std_cfg_1 = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_25,
            .ws = GPIO_NUM_26,
            .dout = I2S_GPIO_UNUSED,
            .din = GPIO_NUM_32,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    i2s_channel_init_std_mode(rx_handle_1, &rx_std_cfg_1);
    i2s_channel_enable(rx_handle_1);

    xTaskCreate(readTask_1, "readTask_1", 65536, NULL, 5, NULL);
    xTaskCreate(printTask_1, "printTask_1", 65664, NULL, 5, NULL);
}





// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "driver/i2s_std.h"
// #include "driver/gpio.h"
// #include "soc/gpio_struct.h"
// #include "driver/uart.h"
// #include "soc/uart_struct.h"
// #include <math.h>
// #include "esp_timer.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "lwip/err.h"
// #include "lwip/sockets.h"
// #include "lwip/sys.h"
// #include "lwip/netdb.h"
// #include "WiFiHelper.h"
// #include "dspHelper.h"

// i2s_chan_handle_t rx_handle_1;
// QueueHandle_t buffQueue_1;

// int Largestflag = 0;
// float largest = 0;

// #define _SAMPLE_RATE 8000
// #define TAG_UDP "UDP"

// // Task to read audio data from the microphone
// static void readTask_1(void *args) {
//     int32_t raw_buf[N_SAMPLES];
//     int16_t output_buf[N_SAMPLES];
//     size_t r_bytes = 0;
//     int samples_read = 0;

//     buffQueue_1 = xQueueCreate(2, sizeof(output_buf));

//     while (1) {
//         if (i2s_channel_read(rx_handle_1, raw_buf, sizeof(int32_t) * N_SAMPLES, &r_bytes, portMAX_DELAY) == ESP_OK) {
//             samples_read = r_bytes / sizeof(int32_t);
//             int sample_index = 0;
//             for (int i = 0; i < samples_read; i++) {
//                 output_buf[sample_index++] = (raw_buf[i] & 0xFFFFFFF0) >> 11; // Convert to 16-bit
//             }
//             xQueueSend(buffQueue_1, (void *)output_buf, (TickType_t)0);
//         }
//     }
// }

// // Task to process and transmit audio data
// static void printTask_1(void *args) {
//     int16_t rxBuffer[N_SAMPLES];

//     while (1) {
//         if (buffQueue_1 != 0) {
//             if (xQueueReceive(buffQueue_1, &(rxBuffer), (TickType_t)10)) {
//                 float *outputBuffer = (float *)calloc(N_SAMPLES, sizeof(float));

//                 // Apply DSP filters (High-Pass and Low-Pass)
//                 HPF(rxBuffer, NULL, 50.0 / _SAMPLE_RATE, 10);
//                 LPF(rxBuffer, NULL, 400.0 / _SAMPLE_RATE, 10);

//                 // Normalize and process data
//                 for (int i = 0; i < N_SAMPLES; i++) {
//                     if (rxBuffer[i] > largest && (Largestflag < 4096)) {
//                         largest = rxBuffer[i];
//                         Largestflag++;
//                     }
//                 }

//                 for (int i = 0; i < N_SAMPLES; i++) {
//                     outputBuffer[i] = rxBuffer[i] / largest;
//                 }

//                 // Send data over UDP
//                 int err = sendto(sock, outputBuffer, (N_SAMPLES) * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//                 if (err < 0) {
//                     ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
//                 }

//                 free(outputBuffer);
//             }
//         }
//     }
// }

// void app_main(void) {
//     initNVS();
//     WiFiConnectHelper();
//     initUDP();

//     // I2S Initialization for a single channel
//     i2s_chan_config_t rx_chan_cfg_1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
//     i2s_new_channel(&rx_chan_cfg_1, NULL, &rx_handle_1);

//     i2s_std_config_t rx_std_cfg_1 = {
//         .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_SAMPLE_RATE),
//         .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
//         .gpio_cfg = {
//             .mclk = I2S_GPIO_UNUSED,
//             .bclk = GPIO_NUM_25,
//             .ws = GPIO_NUM_26,
//             .dout = I2S_GPIO_UNUSED,
//             .din = GPIO_NUM_32,
//             .invert_flags = {
//                 .mclk_inv = false,
//                 .bclk_inv = false,
//                 .ws_inv = false,
//             },
//         },
//     };
//     i2s_channel_init_std_mode(rx_handle_1, &rx_std_cfg_1);
//     i2s_channel_enable(rx_handle_1);

//     xTaskCreate(readTask_1, "readTask_1", 65536, NULL, 5, NULL);
//     xTaskCreate(printTask_1, "printTask_1", 65664, NULL, 5, NULL);
// }







// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "driver/i2s_std.h"
// #include "driver/gpio.h"
// #include "esp_timer.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "lwip/sockets.h"
// #include "WiFiHelper.h"
// #include "dspHelper.h"

// // Constants
// #define SAMPLE_RATE 8000
// #define BUFFER_SIZE 1024
// #define QUEUE_SIZE 2
// #define TAG_UDP "UDP"

// // Handles and Queues
// static i2s_chan_handle_t rx_handle_1;
// static QueueHandle_t buffQueue_1;

// // Shared variables
// static int largest_flag = 0;
// static float largest_sample = 0;

// // Function to initialize I2S
// void initialize_i2s(i2s_chan_handle_t *rx_handle) {
//     i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
//     i2s_new_channel(&chan_cfg, NULL, rx_handle);

//     i2s_std_config_t std_cfg = {
//         .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
//         .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
//         .gpio_cfg = {
//             .mclk = I2S_GPIO_UNUSED,
//             .bclk = GPIO_NUM_25,
//             .ws = GPIO_NUM_26,
//             .dout = I2S_GPIO_UNUSED,
//             .din = GPIO_NUM_32,
//         },
//     };
//     i2s_channel_init_std_mode(*rx_handle, &std_cfg);
//     i2s_channel_enable(*rx_handle);
// }

// // Read Task
// static void read_task(void *args) {
//     int32_t raw_buffer[BUFFER_SIZE];
//     int16_t processed_buffer[BUFFER_SIZE];
//     size_t bytes_read = 0;

//     buffQueue_1 = xQueueCreate(QUEUE_SIZE, sizeof(processed_buffer));

//     while (1) {
//         if (i2s_channel_read(rx_handle_1, raw_buffer, sizeof(raw_buffer), &bytes_read, portMAX_DELAY) == ESP_OK) {
//             int samples_read = bytes_read / sizeof(int32_t);
//             for (int i = 0; i < samples_read; ++i) {
//                 processed_buffer[i] = (raw_buffer[i] & 0xFFFFFFF0) >> 11;
//             }
//             xQueueSend(buffQueue_1, (void *)processed_buffer, portMAX_DELAY);
//         }
//     }
// }

// // Process and Send Task
// static void process_and_send_task(void *args) {
//     int16_t rx_buffer[BUFFER_SIZE];
//     float left_channel[BUFFER_SIZE / 2];
//     float right_channel[BUFFER_SIZE / 2];

//     while (1) {
//         if (buffQueue_1 && xQueueReceive(buffQueue_1, &rx_buffer, portMAX_DELAY)) {
//             float *output_buffer = (float *)calloc(BUFFER_SIZE, sizeof(float));

//             // Signal processing
//             buffSplit(rx_buffer, left_channel, right_channel);
//             HPF(left_channel, right_channel, 50.0 / SAMPLE_RATE, 10);
//             LPF(left_channel, right_channel, 400.0 / SAMPLE_RATE, 10);

//             // Normalization and merging
//             for (int i = 0; i < BUFFER_SIZE; ++i) {
//                 float sample = (i < BUFFER_SIZE / 2) ? left_channel[i] : right_channel[i - BUFFER_SIZE / 2];
//                 if (sample > largest_sample && largest_flag < 4096) {
//                     largest_sample = sample;
//                     largest_flag++;
//                 }
//                 output_buffer[i] = sample / largest_sample;
//             }

//             // Send data over UDP
//             int err = sendto(sock, output_buffer, BUFFER_SIZE * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//             if (err < 0) {
//                 ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
//             }

//             free(output_buffer);
//         }
//     }
// }

// // Application Main
// void app_main(void) {
//     initNVS();
//     WiFiConnect();
//     initUDP();

//     initialize_i2s(&rx_handle_1);

//     xTaskCreate(read_task, "Read Task", 65536, NULL, 5, NULL);
//     xTaskCreate(process_and_send_task, "Process and Send Task", 65536, NULL, 5, NULL);
// }
