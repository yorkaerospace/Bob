#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "gps.h"


#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 0
#define UART_RX_PIN 1



static int32_t str_to_int(char *str, int len)
{
    int32_t result = 0;
    int i;
    int unit = 0;
    for (i=len - 1; i >= 0; i--)
    {
        if(str[i] != '.'){
            int digit = str[i] - '0';
            //printf("***%d", digit);
            result = result + (digit * pow(10, unit));
            unit++;

        }
    }

    return result;
}

static void send_message(unsigned char *message, int len)
{
    int i;

    for (i=0; i<len; i++)
    {
        printf("%c", message[i]);

        uart_putc(uart0, message[i]);
    }
}

void setup_gps(){
    // Set up UART
    uart_init(uart0, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
}

void reset_gps(){
    // Reset GPS
    unsigned char message[] = {'$','P','M','T','K','1','0','4','*','3','7','\r','\n'};

    int length = sizeof(message)/sizeof(message[0]);

    send_message(message, length);

}

void processNMEA( unsigned char *Line, GPS *gps, int length){

    if(Line[0] == '$' && Line[1] == 'G' && Line[2] == 'P' && Line[3] == 'G' && Line[4] == 'G' && Line[5] == 'A'){

        printf("GGA Message Received %s", Line);

        // using strtok to split the string
        char *token = strtok(Line, ",");
        int index = 0;
        while (token != NULL) {
            printf("%d: %s \n", index, token);
            token = strtok(NULL, ",");
            index++;

            if(index == 1){
                gps->Time = atoi(token);
                gps->Hours = gps->Time / 10000;
                gps->Minutes = (gps->Time - (gps->Hours * 10000)) / 100;
                gps->Seconds = (gps->Time - (gps->Hours * 10000) - (gps->Minutes * 100));
                gps->SecondsInDay = (gps->Hours * 3600) + (gps->Minutes * 60) + gps->Seconds;
                printf("Time: %d:%d:%d \n", gps->Hours, gps->Minutes, gps->Seconds);
            }

            if(index == 2){
                int length = strlen(token);
                //printf("Length: %d \n", length);
                //printf("Latitude: %s \n", token);
                int32_t result = str_to_int(token, length);
                gps->Latitude = result;
                //printf("Result: %d \n", result);
            }

            if(index == 2 && *token == '0'){
                gps->FixQuality = 0;
                printf("No Fix \n");
            }

            if(index == 3){
                if(token[0] == 'S'){
                    gps->Latitude = -gps->Latitude;
                }
            }

            if(index == 4){
                int length = strlen(token);
                int32_t result = str_to_int(token, length);
                gps->Longitude = result;
            }

            if(index == 5){
                if(token[0] == 'W'){
                    gps->Longitude = -gps->Longitude;
                }
            }

            if(index == 6){
                //printf("Fix: %s \n", token);
                gps->FixQuality = atoi(token);
            }

            if(index == 7){
                gps->Satellites = atoi(token);
            }

            if(index == 8){
                //printf("HDOP: %s \n", token);
                gps->HDOP = atof(token);
            }

            if(index == 9){
                gps->Altitude = atof(token);
            }

            if(index == 10){
                //printf("Altitude Units: %s \n", token);
            }

            if(index == 11){
                //printf("Geoid Separation: %s \n", token);
                gps->GeoidSeparation = atof(token);
            }

            if(index == 12){
                //printf("Geoid Separation Units: %s \n", token);
            }

            if (gps->FixQuality == 2)
            {

                if(index == 13){
                    //printf("Age of Differential GPS Data: %s \n", token);
                }

                if(index == 14){
                    //printf("Differential Reference Station ID: %s \n", token);
                }

                if(index == 15){
                    //printf("Checksum: %s \n", token);
                }
            }
            else{
                if(index == 13){
                    //printf("Checksum: %s \n", token);
                }
            }

        }
    }

}

void readGPSData(GPS *gps){

    static unsigned char Line[100];
    static int index=0;

    if (uart_is_readable(uart0)) {
        char c = uart_getc(uart0);
        // printf("%c", c);

        if(c == '$'){

            index = 0;
            Line[index] = c;
            index++;

        }
        else if(index > 90){

            index = 0;

        }
        else if(index > 0 && c != '\r'){

            Line[index] = c;
            index++;

            if(c == '\n'){

                Line[index] = '\0';
                //printf("%s", Line);
                processNMEA(Line , gps, index);
                //printf("Time: %d \n", gps.Time);
                index = 0;

            }

        }

    }

}
