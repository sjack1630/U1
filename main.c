#include <kilolib.h>
#include "runner.h"
#include "stationary.h"

typedef enum {
    RUNNER,
    STATIONARY
} role_t;

role_t current_role;
const uint16_t num_robots = 3;
int current_index;

void setup() {
    current_index = kilo_uid;
    if(current_role % num_robots == 0)
        current_role = RUNNER;
    else
        current_role = STATIONARY;
    
    runner_setup();
    stationary_setup();
}

void loop() {
    if(current_role == RUNNER)
        runner_loop();
    else
        stationary_loop(current_index);
}

void message_rx(message_t *m, distance_measurement_t *d) {
    //  TODO: Handle switch runner message
    //  must increment current index and set roles like in setup
    //

    if(current_role == RUNNER)
        runner_message_rx(m, d);
    else
        stationary_message_rx(m, d);
}

message_t *message_tx(){
    if(current_role == RUNNER)
        runner_message_tx();
    else
        stationary_message_tx();
}
void message_tx_success(){
    if(current_role == RUNNER)
        runner_message_tx_success();
    else
        stationary_message_tx_success();
}

int main() {
    kilo_init();
    kilo_message_rx = message_rx;
    kilo_message_tx = message_tx;
    kilo_message_tx_success = message_tx_success;
    kilo_start(setup, loop);

    return 0;
}
