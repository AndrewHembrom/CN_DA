#ifndef DSPHELPER_H
#define DSPHELPER_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_dsp.h"
#include "esp_mac.h"

#define N_SAMPLES 4096
#define HOST_IP_ADDR "172.20.10.3"
#define PORT 11211
struct sockaddr_in dest_addr;
static const char *TAG_UDP = "UDP";
int sock = 0;

void initUDP() {
    int addr_family = 0;
    int ip_protocol = 0;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
    }

    ESP_LOGI(TAG_UDP, "Socket created");

}


void LPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {

    esp_err_t ret = ESP_OK;
    float coeffs_lpf[5];
    float w_lpf[5] = {0,0};

    float leftOutBuffer[N_SAMPLES/2];
    float rightOutBuffer[N_SAMPLES/2];

    // Calculate iir filter coefficients
    ret = dsps_biquad_gen_lpf_f32(coeffs_lpf, freq, qFactor);
    if (ret  != ESP_OK) {
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    // Process input signals
    //Left Channel
    ret = dsps_biquad_f32_ae32(leftInBuffer, leftOutBuffer, N_SAMPLES/2, coeffs_lpf, w_lpf);
    if (ret  != ESP_OK){
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    //Right Channel
    ret = dsps_biquad_f32_ae32(rightInBuffer, rightOutBuffer, N_SAMPLES/2, coeffs_lpf, w_lpf);
    if (ret  != ESP_OK){
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    memcpy(leftInBuffer, leftOutBuffer, (N_SAMPLES/2) * sizeof(float));
    memcpy(rightInBuffer, rightOutBuffer, (N_SAMPLES/2) * sizeof(float));
    
}

void HPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {

    esp_err_t ret = ESP_OK;
    float coeffs_hpf[5];
    float w_hpf[5] = {0,0};

    float leftOutBuffer[N_SAMPLES/2];
    float rightOutBuffer[N_SAMPLES/2];

    // Calculate iir filter coefficients
    ret = dsps_biquad_gen_hpf_f32(coeffs_hpf, freq, qFactor);
    if (ret  != ESP_OK) {
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }

    // Process input signals
    //Left Channel
    ret = dsps_biquad_f32_ae32(leftInBuffer, leftOutBuffer, N_SAMPLES/2, coeffs_hpf, w_hpf);
    if (ret  != ESP_OK){
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }
    //Right Channel
    ret = dsps_biquad_f32_ae32(rightInBuffer, rightOutBuffer, N_SAMPLES/2, coeffs_hpf, w_hpf);
    if (ret  != ESP_OK){
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }

    memcpy(leftInBuffer, leftOutBuffer, (N_SAMPLES/2) * sizeof(float));
    memcpy(rightInBuffer, rightOutBuffer, (N_SAMPLES/2) * sizeof(float));
    
}

void buffSplit(int16_t* buff, float* lBuffer, float* rBuffer) {

    int k = 0;
    for(int i = 0; i < N_SAMPLES; i += 2) {
        lBuffer[k] = (buff[i]);
        rBuffer[k++] = (buff[i + 1]);
    }

}


#endif

// #ifndef DSPHELPER_H
// #define DSPHELPER_H

// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include "esp_dsp.h"
// #include "esp_mac.h"

// #define N_SAMPLES 4096
// #define HOST_IP_ADDR "172.20.10.2"
// #define PORT 11211

// // UDP Configuration
// struct sockaddr_in dest_addr;
// static const char *TAG_UDP = "UDP";
// int sock = 0;

// // Initialize UDP Socket
// void initUDP() {
//     dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
//     dest_addr.sin_family = AF_INET;
//     dest_addr.sin_port = htons(PORT);

//     sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
//     if (sock < 0) {
//         ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
//         return;
//     }

//     ESP_LOGI(TAG_UDP, "Socket successfully created");
// }

// // Apply a Low-Pass Filter
// void applyFilter(float *inputBuffer, float *outputBuffer, float freq, float qFactor, esp_err_t (*filter_gen)(float*, float, float), const char *filterTag) {
//     float coeffs[5];
//     float state[5] = {0};
//     esp_err_t ret = ESP_OK;

//     // Generate filter coefficients
//     ret = filter_gen(coeffs, freq, qFactor);
//     if (ret != ESP_OK) {
//         ESP_LOGE(filterTag, "Coefficient generation error: %d", ret);
//         return;
//     }

//     // Process the input buffer
//     ret = dsps_biquad_f32_ae32(inputBuffer, outputBuffer, N_SAMPLES / 2, coeffs, state);
//     if (ret != ESP_OK) {
//         ESP_LOGE(filterTag, "Filter processing error: %d", ret);
//         return;
//     }

//     // Copy the processed output back to the input buffer
//     memcpy(inputBuffer, outputBuffer, (N_SAMPLES / 2) * sizeof(float));
// }

// // Low-Pass Filter Wrapper
// void LPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {
//     float leftOutBuffer[N_SAMPLES / 2];
//     float rightOutBuffer[N_SAMPLES / 2];

//     applyFilter(leftInBuffer, leftOutBuffer, freq, qFactor, dsps_biquad_gen_lpf_f32, "LPF");
//     applyFilter(rightInBuffer, rightOutBuffer, freq, qFactor, dsps_biquad_gen_lpf_f32, "LPF");
// }

// // High-Pass Filter Wrapper
// void HPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {
//     float leftOutBuffer[N_SAMPLES / 2];
//     float rightOutBuffer[N_SAMPLES / 2];

//     applyFilter(leftInBuffer, leftOutBuffer, freq, qFactor, dsps_biquad_gen_hpf_f32, "HPF");
//     applyFilter(rightInBuffer, rightOutBuffer, freq, qFactor, dsps_biquad_gen_hpf_f32, "HPF");
// }

// // Split Stereo Buffer into Left and Right Channels
// void buffSplit(int16_t *buff, float *lBuffer, float *rBuffer) {
//     for (int i = 0, k = 0; i < N_SAMPLES; i += 2, ++k) {
//         lBuffer[k] = (float)buff[i];
//         rBuffer[k] = (float)buff[i + 1];
//     }
// }

// #endif // DSPHELPER_H
