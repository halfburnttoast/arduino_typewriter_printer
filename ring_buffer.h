#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFF_SIZE 0xFF

typedef struct {
    uint8_t buffer[RING_BUFF_SIZE];
    uint8_t read_idx;
    uint8_t write_idx;
    uint8_t empty;
} ring_buffer;

void rb_reset(void);
void rb_write(const unsigned char value);
uint8_t rb_read(void);

#endif
