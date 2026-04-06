#include "runner.h"

// declare constants
uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
uint8_t DESIRED_DISTANCE = 60; // 60 mm

extern uint8_t num_robots;
extern uint8_t current_runner;

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

// variables from original orbit code 
motion_t cur_motion = STOP;
orbit_state_t orbit_state = ORBIT_NORMAL;
uint8_t cur_distance = 0;
uint8_t new_message = 0;
distance_measurement_t dist;
message_t msg;

// variables for leap frog
uint8_t rx_kilo_id; // id of bot that sent message
uint8_t front_kilo_id; // id of the bot in the front
uint8_t second_kilo_id; // id of bot behind front one; bot used to know when to stop runner at the front
uint8_t prev_second_dist = 0; // the previous distance from the runner to the second bot
uint8_t dist_val; // distance from the message received
uint8_t stop_flag; // flag to stop runner
uint8_t switch_sent_flag; // flag to initialize switch == set compute new current_runner and set in message
uint8_t cur_target_kilo_id; // id of bot runner is currently orbiting
uint8_t current_runner_local; // should be same as current_runner until switch. It doesn't work if I don't separate it into current_runner and current_runner_local. I think it's because I'm getting current_runner from main, but I'm also not really sure still


// function to set new motion
// UNCHANGED
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

// UNCHANGED
void orbit_normal() {
    if (cur_distance < TOOCLOSE_DISTANCE) {
        orbit_state = ORBIT_TOOCLOSE;
    } else {
        if (cur_distance < DESIRED_DISTANCE)
            set_motion(LEFT);
        else
            set_motion(RIGHT);
    }
}

// UNCHANGED
void orbit_tooclose() {
    if (cur_distance >= DESIRED_DISTANCE )
        orbit_state = ORBIT_NORMAL;
    else
        set_motion(FORWARD);
}

void runner_setup(){
    // Runner is this bot
    current_runner_local = kilo_uid;

    // Reset flag values
    stop_flag = 0;
    switch_sent_flag = 0;

    // Modulo math. On the first pass with three robots it should go: 1, 2, 1. With init runner being 0. 
    cur_target_kilo_id = (current_runner + 1) % num_robots;
    front_kilo_id = (current_runner - 1 + num_robots) % num_robots;
    second_kilo_id = (front_kilo_id - 1 + num_robots) % num_robots;

    // Set both message parts
    msg.type = NORMAL;
    msg.data[0] = kilo_uid;
    msg.data[1] = current_runner_local;
    msg.crc = message_crc(&msg);
}

void runner_loop(){
    // Update distance estimate with every message
    if (new_message) {
        new_message = 0;
        dist_val = estimate_distance(&dist);

        // If incoming dist value is less than my previous distance and that distance isn't from the bot runner is currently orbiting, switch the target bot and blink BLUE
        if (dist_val < cur_distance && rx_kilo_id != cur_target_kilo_id){
            cur_distance = dist_val;
            cur_target_kilo_id = rx_kilo_id;
            set_color(RGB(0,0,1));
            delay(20);
            set_color(RGB(0,0,0));
        }

        // Update cur_distance if message is from target bot
        if (rx_kilo_id == cur_target_kilo_id){
            cur_distance = dist_val;
        }

        // Logic for stopping in front
        // If runner is orbiting front bot and receives message from second to front bot:
        // 1) Lower desired distance. Maybe not necessary but it's been working well. The tighter orbit keeps runner in range of second to front bot
        // 2) Check if distance from second to front bot is increasing. If it is, update prev_second_dist to this new dist_val
        // If distance is not increasing and the distance from second to front bot to runner is greater than 80 mm, stop runner, switch stop_flag, blink blue
        // dist_val > 80 is necessary because bot movement is sporadic, so dist from runner to second to front will increase and decrease as it moves to the front. dist_val > 80 makes sure it's actually in the front when distance starts to decrease
        if (cur_target_kilo_id == front_kilo_id && rx_kilo_id == second_kilo_id){
            DESIRED_DISTANCE = 50;
            if (dist_val > prev_second_dist){
                prev_second_dist = dist_val;
            } else if (dist_val < prev_second_dist && dist_val > 80){
                set_motion(STOP);
                stop_flag = 1;
                set_color(RGB(0,0,1));
                delay(20);
                set_color(RGB(0,0,0));
            }

        // This is just debugging. Blink yellow if distance is decreasing but dist_val is not greater than 80. I used this to validate need for dist_val > 80. In my text in the group chat I lied and said yellow was switch. What I meant was blue (above) is switch.
        if (cur_distance < prev_second_dist){
                set_color(RGB(1,1,0));
                delay(20);
                set_color(RGB(0,0,0));
            }
        }

    } else if (cur_distance == 0) // skip state machine if no distance measurement available
        return;

    // Changing of current_runner logic
    // When stop_flag is triggered:
    // 1) Keep motion as STOP. Note stop_flag never gets set back to 0. It will when runner_setup() is run in main when this current bot becomes runner again
    // 2) If switch_sent_flag hasn't been triggered (switch command hasn't been propagated to swarm), then compute next runner id and set message [1] value to this. Trigger switch_sent_flag so that runner does not keep computing/updating runner id
    // Had to separate stop_flag and switch_sent_flag to separate: 1) STOP motion of current runner 2) computing next runner. STOP motion had to persist but computing next runner only happens once. 
    if (stop_flag == 1){
        set_motion(STOP);
        if (switch_sent_flag == 0){
            current_runner_local = (current_runner_local + 1) % num_robots;
            msg.data[1] = current_runner_local;
            msg.crc = message_crc(&msg);
            switch_sent_flag = 1;
        }
        return;
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

}

void runner_message_rx(message_t *m, distance_measurement_t *d){
    rx_kilo_id = m->data[0];
    new_message = 1;
    dist = *d;
}
message_t *runner_message_tx(){
    return &msg;
}

void runner_message_tx_success(){

}