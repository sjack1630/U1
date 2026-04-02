#include <kilolib.h>

void runner_message_rx(message_t *m, distance_measurement_t *d);
message_t *runner_message_tx();
void runner_message_tx_success();

void runner_loop(uint8_t num_robots);
void runner_setup(uint8_t num_robots);