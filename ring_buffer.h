#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFF_SIZE 0xFF

// This struct can now be modified by the input ISR
typedef struct {
    volatile uint8_t buffer[RING_BUFF_SIZE];
    volatile uint8_t read_idx;
    volatile uint8_t write_idx;
    volatile uint8_t empty;
} ring_buffer;

void rb_reset(void);
void rb_write(const unsigned char value);
uint8_t rb_read(void);
uint8_t rb_peek(void);
uint8_t rb_is_empty(void);

#endif
