#include "runner.h"

// declare constants
static const uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
static const uint8_t DESIRED_DISTANCE = 60; // 60 mm
static const uint8_t DISTANCE_RESET_TICKS = 32; // 1 s

// declare motion variable type
typedef enum {
    STOP,
    FORWARD,
    LEFT,
    RIGHT
} motion_t;

// declare state variable type
typedef enum {
    ORBIT_TOOCLOSE,
    ORBIT_NORMAL,
} orbit_state_t;

// declare variables
motion_t cur_motion = STOP;
orbit_state_t orbit_state = ORBIT_NORMAL;
uint8_t new_message = 0;
message_t msg_plain;
message_t msg_switch;

uint8_t cur_runner_id;
uint8_t cur_front_id;
uint8_t cur_second_id;
uint8_t rx_kilo_id;
uint8_t cur_target_kilo_id = 0;
uint8_t switch_flag;

uint8_t cur_lowest_dist = UINT8_MAX;
uint8_t cur_second_dist = UINT8_MAX;
uint8_t prev_second_dist = UINT8_MAX;

// For reset logic
uint8_t last_update;

distance_measurement_t dist;

void orbit_normal();
void orbit_tooclose();
void set_motion(motion_t new_motion);
void switch_runner(uint8_t num_robots);

// no setup code required
void runner_setup(uint8_t num_robots){
    cur_runner_id = 1;
    cur_front_id = (cur_runner_id + num_robots - 2) % num_robots + 1;
    cur_second_id = (cur_front_id + num_robots - 2) % num_robots + 1;
    last_update = kilo_ticks;
    switch_flag = 0;

    msg_plain.type = NORMAL;
    msg_plain.data[0] = 500;
    msg_plain.crc = message_crc(&msg_plain);
    msg_switch.type = NORMAL;
    msg_switch.data[0] = 300;
    msg_switch.crc = message_crc(&msg_switch);
}

void runner_loop(uint8_t num_robots){
    // Update distance estimate with every message
    if (new_message) {
        new_message = 0;
    } else if (cur_lowest_dist == 0) // skip state machine if no distance measurement available
        return;

    // Orbit state machine
    switch(orbit_state) {
        case ORBIT_NORMAL:
            orbit_normal();
            break;
        case ORBIT_TOOCLOSE:
            orbit_tooclose();
            break;
    }

    // If orbiting the front bot and distance from second bot starts to increase, stop runner
    if (cur_target_kilo_id == cur_front_id){
        if (cur_second_dist > prev_second_dist && switch_flag == 0){
            set_motion(STOP);
            switch_runner(num_robots);
            switch_flag = 1;
        }
    }

    // Every second reset the lowest dist and target kilo id to make sure runner doesn't get stuck in a "pit"
    // Without a reset, there could be no new minimum between two stationary bots
    if (kilo_ticks > last_update + DISTANCE_RESET_TICKS) {
        last_update = kilo_ticks;
        cur_lowest_dist = UINT8_MAX;
        cur_target_kilo_id = 0;
    }
}

// function to set new motion
void set_motion(motion_t new_motion) {
    if (cur_motion != new_motion) {
        cur_motion = new_motion;
        switch(cur_motion) {
            case STOP:
                set_motors(0,0);
                break;
            case FORWARD:
                spinup_motors();
                set_motors(kilo_straight_left, kilo_straight_right);
                break;
            case LEFT:
                spinup_motors();
                set_motors(kilo_turn_left, 0); 
                break;
            case RIGHT:
                spinup_motors();
                set_motors(0, kilo_turn_right); 
                break;
        }
    }
}

void orbit_normal() {
    if (cur_lowest_dist < TOOCLOSE_DISTANCE) {
        orbit_state = ORBIT_TOOCLOSE;
    } else {
        if (cur_lowest_dist < DESIRED_DISTANCE)
            set_motion(LEFT);
        else
            set_motion(RIGHT);
    }
}

void orbit_tooclose() {
    if (cur_lowest_dist >= DESIRED_DISTANCE)
        orbit_state = ORBIT_NORMAL;
    else
        set_motion(FORWARD);
}

void switch_runner(uint8_t num_robots){
    cur_runner_id = (cur_runner_id % num_robots) + 1;
    cur_front_id = (cur_runner_id + num_robots - 2) % num_robots + 1;
    cur_second_id = (cur_front_id + num_robots - 2) % num_robots + 1;
}

void runner_message_rx(message_t *m, distance_measurement_t *d){
    rx_kilo_id = m->data[0];
    new_message = 1;
    dist = *d;
    uint8_t dist_val = estimate_distance(&dist);

    // Determine closest robot = target orbiting robot
    if (dist_val < cur_lowest_dist){
        cur_lowest_dist = dist_val;
        cur_target_kilo_id = rx_kilo_id;
    }

    // Set second current distance if received; to be used when target robot is front robot
    if (rx_kilo_id == cur_second_id){
        cur_second_dist = dist_val;
    }
}
message_t *runner_message_tx(){
    if (switch_flag)
        return &msg_switch;
    else
        return &msg_plain;
}

void runner_message_tx_success(){

}