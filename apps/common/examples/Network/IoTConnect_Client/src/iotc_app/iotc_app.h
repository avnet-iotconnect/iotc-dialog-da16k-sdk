#ifndef BASIC_SAMPLE_H
#define BASIC_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iotc_command_queue_item_s  {
    const char *command;
    const char *ack_id;  
} iotc_command_queue_item;

extern void stop_iotconnect(void);
extern void setup_iotconnect(void);
extern void start_iotconnect(void);
extern void reset_iotconnect(void);


void iotc_command_queue_item_destroy(iotc_command_queue_item item);

/*  Gets the next available command & parameter from the command queue.
    Returns false if there was an error or no command was available.*/
bool iotc_command_queue_item_get(iotc_command_queue_item *dst_item);

#ifdef __cplusplus
}
#endif

#endif // BASIC_SAMPLE_H
