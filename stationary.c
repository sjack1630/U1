#include "stationary.h"

// Take current_runner from main
extern uint8_t current_runner;
extern uint8_t num_robots;

uint8_t message_sent;
message_t msg;

void stationary_setup(){
    message_sent = 0;
    msg.type = NORMAL;
    msg.data[0] = kilo_uid;
    msg.data[1] = current_runner;
    msg.crc = message_crc(&msg);
}

void stationary_loop(){
    // blink red when message is sent
    if (message_sent) {
        message_sent = 0;
        set_color(RGB(1,0,0));
        delay(20);
        set_color(RGB(0,0,0));
    }

    // Continuously update current_runner in message
    if (num_robots != 2){
        msg.data[1] = current_runner;
        msg.crc = message_crc(&msg);
    }


}

void stationary_message_rx(message_t *m, distance_measurement_t *d){
    // rx in main takes care of receiving current_runner updates
}

message_t *stationary_message_tx(){
    return &msg;
}

void stationary_message_tx_success(){
    message_sent = 1;
}