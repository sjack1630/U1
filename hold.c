#include "runner.h"

// declare constants
uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
uint8_t DESIRED_DISTANCE = 60; // 60 mm

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
uint8_t cur_distance = 0;
uint8_t new_message = 0;
uint8_t rx_kilo_id;
uint8_t front_kilo_id;
uint8_t second_kilo_id;
uint8_t prev_second_dist = 0;
uint8_t dist_val;
uint8_t stop_flag = 0;
uint8_t cur_target_kilo_id;
uint8_t switch_flag_runner = 0;
distance_measurement_t dist;
message_t msg;

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
    if (stop_flag == 1){
        set_motion(STOP);
    } else if (cur_distance < TOOCLOSE_DISTANCE) {
        orbit_state = ORBIT_TOOCLOSE;
    } else {
        if (cur_distance < DESIRED_DISTANCE)
            set_motion(LEFT);
        else
            set_motion(RIGHT);
    }
}

void orbit_tooclose() {
    if (stop_flag == 1){
        set_motion(STOP);
    } else if (cur_distance >= DESIRED_DISTANCE )
        orbit_state = ORBIT_NORMAL;
    else
        set_motion(FORWARD);
}

// no setup code required
void runner_setup(uint8_t num_robots, uint8_t current_runner){
    cur_target_kilo_id = (current_runner + 1) % num_robots;
    front_kilo_id = (current_runner - 1 + num_robots) % num_robots;
    second_kilo_id = (front_kilo_id - 1 + num_robots) % num_robots;
    msg.type = NORMAL;
    msg.data[0] = kilo_uid;
    msg.crc = message_crc(&msg);
}

uint8_t runner_loop(uint8_t return_switch_flag){
    // Update distance estimate with every message
    if (new_message) {
        new_message = 0;
        dist_val = estimate_distance(&dist);

        // Determine closest robot = target orbiting robot
        if (dist_val < cur_distance && rx_kilo_id != cur_target_kilo_id && rx_kilo_id > cur_target_kilo_id){
            cur_distance = dist_val;
            cur_target_kilo_id = rx_kilo_id;
            set_color(RGB(0,1,0));
        }
        if (rx_kilo_id == cur_target_kilo_id){
            cur_distance = dist_val;
        }

        if (cur_target_kilo_id == front_kilo_id && rx_kilo_id == second_kilo_id){
            DESIRED_DISTANCE = 50;
            if (dist_val > prev_second_dist){
                prev_second_dist = dist_val;
            } else if (dist_val < prev_second_dist && dist_val > 85){
                set_motion(STOP);
                stop_flag = 1;
                set_color(RGB(0,0,1));
                delay(20);
                set_color(RGB(0,0,0));
            }

            if (cur_distance < prev_second_dist){
                set_color(RGB(1,1,0));
                delay(20);
                set_color(RGB(0,0,0));
            }
        }

    } else if (cur_distance == 0) // skip state machine if no distance measurement available
        return switch_flag_runner;

    if (stop_flag == 1){
        msg.data[1] = 1;
        msg.crc = message_crc(&msg);
        switch_flag_runner = 1;
    }
    if (return_switch_flag == 1){
        stop_flag = 0;
        switch_flag_runner = 0;
    }

    // Orbit state machine
    switch(orbit_state) {
        case ORBIT_NORMAL:
            orbit_normal();
            break;
        case ORBIT_TOOCLOSE:
            orbit_tooclose();
            break;
    }
    return switch_flag_runner;

}

void runner_message_rx(message_t *m, distance_measurement_t *d){
    rx_kilo_id = m->data[0];
    new_message = 1;
    dist = *d;
    set_color(RGB(0, 1, 1));
}
message_t *runner_message_tx(){
    return &msg;
}

void runner_message_tx_success(){

}