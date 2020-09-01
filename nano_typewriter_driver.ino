#define SERIAL_ENABLE
#include "keycodes.h"
#include "ring_buffer.h"
extern ring_buffer g_ring_buff;

#ifdef SERIAL_ENABLE
// ==== Begin Serial com vars ====
#define SERIAL_RATE 115200
#define SERIAL_INPUT_BUFFER_SIZE 256
#define SERIAL_OUTPUT_BUFFER_SIZE 64
char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
char serial_output_buffer[SERIAL_OUTPUT_BUFFER_SIZE];
// ==== End Serial com vars ====
#endif
#define DATA     2
#define SR_OE    A5
#define RCLK     3
#define SRCLK    4
#define PRINT_DELAY 5
uint8_t g_print_delay = PRINT_DELAY;
//uint8_t port_input_pins[8] = {
//    5, 6, 7, 8, 9, 10, 11, 12
//  0  1  2  3  4  5   6   7
//};


#ifdef SERIAL_ENABLE
// printf wrapper for serial output
void sprint(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(serial_output_buffer, SERIAL_OUTPUT_BUFFER_SIZE, fmt, args);
    va_end(args);
    Serial.print(serial_output_buffer);
}
// fetch serial input and store in get_serial_input
// returns length of recieved text on return press
// returns 0 if return hasn't been pressed
uint8_t get_serial_input(void) {
    static int gsi_index = 0;
    if(Serial.available()) {
        char cin = Serial.read();
        if(cin > 0) {
            switch(cin) {
                case 0x1B:
                    Serial.print("\n\r");
                    gsi_index = 0;
                    break;
                case 0x08:
                case 0x7F:
                    if(gsi_index == 0)
                        break;
                    gsi_index--;
                    Serial.print(cin);
                    serial_input_buffer[gsi_index] = 0;
                    break;
                case '\r':
                    //break;
                case '\n':
                    Serial.print("\n\r");
                    int rtn;
                    rtn = gsi_index;
                    gsi_index = 0;
                    return rtn;
                case '\t':
                default:
                    if(gsi_index < (SERIAL_INPUT_BUFFER_SIZE - 1)) {
                        serial_input_buffer[gsi_index] = cin;
                        Serial.print(cin);
                        gsi_index++;
                        serial_input_buffer[gsi_index] = 0;
                    }
                     //sprint("DEBUG: %s\nGSI: %d\nCHR: %02X\n", serial_input_buffer, gsi_index, cin);
                    return 0;
            }
        }
    } else {
        return 0;
    }
}
#endif

// decodes charin and produces the control code and address
// returns shift required true or false 
uint8_t decode_key(const char charin, uint8_t *code, uint8_t *addr) {
    uint8_t value = keycodes[charin];
    *addr = (value & 0x7);
    *code = (1 << ((value & 0xF0) >> 4));
    return (value & 0x08);
}

// shift byte into shift register, no output
void sr_shift_out(const uint8_t value) {
    for(uint8_t i; i < 8; i++) {
        uint8_t dout = ((value >> (7 - i)) & 0x1);
        PORTD = ((PORTD & 0xFB) | (dout << 2));
        PORTD = (PORTD | 0x10);
        PORTD = (PORTD & 0xEF);
    }
}

// latch shift register contents to output register
inline void sr_latch(void) {
    PORTD = PORTD | 0x08;
    PORTD = PORTD & 0xF7;    
}

// set SR pins to output
inline void sr_output_enable(void) {
    PORTC = (PORTC & 0xDF);
}

// set SR pins to high-impedance
inline void sr_output_disable(void) {
    PORTC = (PORTC | 0x20);
}

// Sends a decoded character code on a given address
void send_code(const uint8_t code, const uint8_t addr) {
    uint8_t outcode = ~code;        // pins are active low, so invert code
    sr_shift_out(0xFF);             // set all output pins high
    sr_latch();                     //  "
    sr_output_enable();             // enable high outputs (will block keyboard)
    
    // it seems like the code needs to be repeated for a few cycles to work.
    // The controller probably has software debouncing to filter out false-triggers
    for(uint8_t count = 0; count <= 2; count++) {
        sr_shift_out(outcode);          // prepare code for output, don't latch yet!
        while((PINC & 0x0F) != addr)    // wait for address to appear before latching
            __asm__("nop");
        sr_latch();                     // code has appeared, latch output value
        sr_shift_out(0xFF);             // prepare for reset 
        while((PINC & 0x0F) == addr)    // wait for scancode to change
            __asm__("nop");   
        sr_latch();                     // reset output
    }
    sr_output_disable();            // go back to high-impedance
}

// CALL THIS function to print characters
// if called, it will set the interrupt delay to compenstate for the 
// carriage head movement depending on the character sent.
void print_c(const char cin) {
    uint8_t c, a, s;
    if(cin == 0)
        return;
    s = decode_key(cin, &c, &a);
    if(s != 0) { send_code(0x80, 7); }  // send shift key if necessary
    send_code(c, a);
    switch(cin) {                       // if one of these characters was sent
        case 0x0A:                      // delay the ISR so the hardware can finish.
        case 0x0D:
        case 0x07:
        case 0x09:
            g_print_delay = 0x50;       // long delay for large carriage movement
            break;
        default:
            g_print_delay = PRINT_DELAY;
    }
}

// process ONE character from the ring buffer per call, return if empty
// This must reset the ISR delay before returning if buffer is empty
void handle_buffer(void) {
    if(g_ring_buff.empty) {
        g_print_delay = PRINT_DELAY;
        return;
    }
    print_c(rb_read());
}

void setup() {
#ifdef SERIAL_ENABLE
    Serial.begin(SERIAL_RATE);
    //for(uint8_t i = 0; i < 40; i++)
    //    sprint("\n");
    sprint("Serial ready.\n\r");
#endif
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(12, INPUT_PULLUP);
    
    // setup shift register output
    pinMode(DATA, OUTPUT);
    pinMode(SR_OE, OUTPUT);
    pinMode(RCLK, OUTPUT);
    pinMode(SRCLK, OUTPUT);
    sr_output_disable();
    sr_shift_out(0xFF);

    // input address from controller
    pinMode(A0, INPUT);         // defined just to help me keep track
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
    pinMode(A3, INPUT);

    // reset ring buffer
    rb_reset();

    // setup interrupts for TIMER1
    cli();
    TCCR1A = 0;                 // clear timer1 control reg A
    TCCR1B = 0;                 // clear timer1 control reg B
    TCNT1 = 0;                  // clear timer1 count
    TIMSK1 |= (1 << TOIE1);
    TCCR1B |= 0x02;
    sei();
}

/* Timer1 is running but needs to be tuned a bit, thus the delay. This delay
 *  needs to be changeable by the print routines to compensate for LF/CR/TAB.
 *  Calling this ISR automatically resets the interrupt flag
 */
ISR(TIMER1_OVF_vect) {
    g_print_delay--;
    if(g_print_delay == 0) {
        PORTB = (PORTB | 0x20);
        handle_buffer();
        PORTB = (PORTB & 0xDF);     // toggle status LED
    }

}

uint8_t triggered = 0;
void loop() {
#ifdef SERIAL_ENABLE
    if(get_serial_input()) {
        for(uint8_t i = 0; i <= 0xFF; i++) {
            uint8_t temp = serial_input_buffer[i];
            if(temp == 0) {
                rb_write('\n');
                break;
            }
            rb_write(temp);
        }  
    }
    /*
    if(Serial.available()) {
        uint8_t bytes_read = Serial.readBytesUntil('\r', serial_input_buffer, SERIAL_INPUT_BUFFER_SIZE - 1);
        serial_input_buffer[bytes_read] = 0;
        for(uint8_t i = 0; i <= 0xFF; i++) {
            uint8_t temp = serial_input_buffer[i];
            if(temp == 0) {
                rb_write('\n');
                break;
            }
            rb_write(temp);
        }    
    }
    */
#endif

    // Use bit 7 from the input byte (port 12) to latch the ascii value code 
    if(triggered == 0 && (PINB & 0x10)) {
        triggered = 1;
        uint8_t bytein = ((PIND & 0xE0) >> 5);
        sprint("D: %02X, B: %02X\n", PIND, PINB);
        sprint("%02X -> ", bytein);
        bytein |= ((PINB & 0x1F) << 3);
        sprint("%02X\n", bytein);
        rb_write((bytein & 0x7F));
    } 
    else if(triggered == 1 && !(PINB & 0x10)) {
        triggered = 0;
    }
}



/*   
// DEBUG STUFF --------------------------------------
    //delay(1000);
    //send_code(1, 5);    // tab
    //send_code(2, 6);    // whiteout
    //send_code(0x04, 6); // backspace
    //send_code(0x20, 6);   // whiteout word
    send_code(0x40, 6);   // CR + LF
    delay(3000);
    //send_code(0x40, 7);     // shift_lock lock
    //send_code(0x80, 7);
    //send_code(1, 6);            // cmd  
    //send_code(0x40, 7);         // caps lock
    //delay(10);
    for(uint8_t i = 0x41; i <= 0x7A; i++) {
        print_c(i);
        delay(100);
    }
    send_code(1, 5);
// -------------------------------------------------
*/
