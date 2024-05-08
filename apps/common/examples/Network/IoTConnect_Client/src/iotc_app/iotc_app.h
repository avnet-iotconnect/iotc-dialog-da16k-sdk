#ifndef BASIC_SAMPLE_H
#define BASIC_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iotc_command_queue_item_s  {
    const char *command;
    const char *ack_id;  
} iotc_command_queue_item_t;

extern void stop_iotconnect(void);
extern void setup_iotconnect(void);
extern void start_iotconnect(void);
extern void reset_iotconnect(void);


void iotc_command_queue_item_destroy(iotc_command_queue_item_t *item);

/*  Gets the next available command from the command queue

    The pointer returned here *MUST* be dealt with by calling iotc_command_queue_item_destroy()!

    Returns NULL if there is no command available.*/
iotc_command_queue_item_t *iotc_command_queue_item_get();

#ifdef __cplusplus
}
#endif

#endif // BASIC_SAMPLE_H
