#include "stationary.h"

uint8_t message_sent = 0;
uint8_t cur_runner_id;
uint8_t cur_front_id;
uint8_t cur_second_id;
uint8_t switch_flag;
message_t msg;
uint8_t incoming_msg;

void switch_runner(uint8_t num_robots);

void stationary_setup(uint8_t num_robots){
    // initialize message
    msg.type = NORMAL;
    msg.data[0] = kilo_uid;
    msg.crc = message_crc(&msg);
    
    cur_runner_id = 1;
    cur_front_id = (cur_runner_id + num_robots - 2) % num_robots + 1;
    cur_second_id = (cur_front_id + num_robots - 2) % num_robots + 1;
    switch_flag = 0;
}

void stationary_loop(uint8_t current_index, uint8_t num_robots){
    // blink red when message is sent
    if (message_sent) {
        message_sent = 0;
        set_color(RGB(1,0,0));
        delay(20);
        set_color(RGB(0,0,0));
    }

    if (incoming_msg == 300 && switch_flag == 0){
        switch_runner(num_robots);
        switch_flag = 1;
    } else if (incoming_msg == 500){
        switch_flag = 0;
    }
}

void switch_runner(uint8_t num_robots){
    cur_runner_id = (cur_runner_id % num_robots) + 1;
    cur_front_id = (cur_runner_id + num_robots - 2) % num_robots + 1;
    cur_second_id = (cur_front_id + num_robots - 2) % num_robots + 1;
}

void stationary_message_rx(message_t *m, distance_measurement_t *d){
    incoming_msg = m->data[0];
}
message_t *stationary_message_tx(){
    return &msg;
}

void stationary_message_tx_success(){
    message_sent = 1;
}