#include "ring_buffer.h"

// there's probably only ever going to be one of these, so make it global
ring_buffer g_ring_buff; 

// reset the global ring buffer state, must be called before using!
void rb_reset(void) {
    g_ring_buff.read_idx  = 0;
    g_ring_buff.write_idx = 0;
    g_ring_buff.empty = 1;
}

uint8_t rb_is_empty(void) {
    return g_ring_buff.empty;
}

// write a byte to the buffer and increment the write offset pointer
void rb_write(const unsigned char value) {
    g_ring_buff.buffer[g_ring_buff.write_idx] = value;
    g_ring_buff.write_idx++;
    g_ring_buff.buffer[g_ring_buff.write_idx] = 0;
    g_ring_buff.empty = 0;
}

// read a byte from the buffer, increment the read offset pointer
// if both pointers are equal, we've reached the end of the buffer items
uint8_t rb_read(void) {
    uint8_t temp = g_ring_buff.buffer[g_ring_buff.read_idx];
    g_ring_buff.read_idx++;
    if(g_ring_buff.read_idx == g_ring_buff.write_idx)
        rb_reset();
    return temp;
}

uint8_t rb_peek(void) {
    return g_ring_buff.buffer[g_ring_buff.read_idx];
}
