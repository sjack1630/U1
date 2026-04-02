#include "stationary.h"

uint8_t message_sent = 0;
message_t msg;

void stationary_setup(){
    // initialize message
    msg.type = NORMAL;
    msg.crc = message_crc(&msg);
    msg.data[0] = kilo_uid;
}

void stationary_loop(uint16_t current_index){
    // blink red when message is sent
    if (message_sent) {
        message_sent = 0;
        set_color(RGB(1,0,0));
        delay(20);
        set_color(RGB(0,0,0));
    }
}

void stationary_message_rx(message_t *m, distance_measurement_t *d){

}
message_t *stationary_message_tx(){
    return &msg;
}

void stationary_message_tx_success(){
    message_sent = 1;
}