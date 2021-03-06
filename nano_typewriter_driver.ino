#define SERIAL_ENABLE
#define PORT_ENABLE
#include "keycodes.h"
#include "ring_buffer.h"
extern ring_buffer g_ring_buff;         // global ring buffer for printing

#ifdef SERIAL_ENABLE
#define SERIAL_RATE 115200
#define SERIAL_INPUT_BUFFER_SIZE 256
#define SERIAL_OUTPUT_BUFFER_SIZE 64
char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
char serial_output_buffer[SERIAL_OUTPUT_BUFFER_SIZE];
#endif

// Control pins
#define DATA     A4
#define SR_OE    A5
#define RCLK     3
#define SRCLK    4

// Internal print-state variables
#define CHARS_PER_LINE  64              // maximum characters per line of paper
uint8_t g_character_index = 0;          // used to track carriage position




/*
 * Functions ----------------------------------------------
 */

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
#endif   // ifdef SERIAL_ENABLE

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
        PORTC = ((PORTC & 0xEF) | (dout << 4));
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
            _NOP();
        sr_latch();                     // code has appeared, latch output value
        sr_shift_out(0xFF);             // prepare for reset 
        while((PINC & 0x0F) == addr)    // wait for scancode to change
            _NOP();   
        sr_latch();                     // reset output
    }
    sr_output_disable();            // go back to high-impedance
}

// CALL THIS function to print characters
void print_c(const char cin) {
    uint8_t c, a, s;
    static uint8_t last_char;           // track last character printed for linefeed bug
    if(cin == 0)
        return;

    /*
     * Linefeed bug: On this typewriter, LF and CR do the same operation. Either will 
     * do both a CR and LF. LF and CR are often, but not always, sent together. 
     * This filters out one of those operations to prevent accidental linefeeds, 
     * but still allows multiple linefeeds to be sent intentionally. A better fix would 
     * be to map either the LF or CR character codes in keycodes.h to a null operation. 
     * But, I don't know of any yet. 
     */
    switch(cin) {
        case 0x0A:
            if(last_char == 0x0D)
                return;
        case 0x0D:
            if(last_char == 0x0A)
                return;
    }
    s = decode_key(cin, &c, &a);
    if(s != 0) { send_code(0x80, 7); }  // send shift key if necessary
    send_code(c, a);
    switch(cin) {                       // if one of these characters was sent
        case 0x0A:                      // delay the ISR so the hardware can finish.
        case 0x0D:
        case 0x07:
        case 0x09:
            delay(2000);                // long delay for large carriage movement
            g_character_index = 0;
            break;
        default:
            g_character_index++;
    }
    last_char = cin;
}

// process ONE character from the ring buffer per call
void handle_buffer(void) {
    // auto insert a newline if the carriage has reached the end of paper
    // and next character isn't a newline. 
    if(g_character_index < CHARS_PER_LINE) {
        print_c(rb_read());
    } else {
        switch(rb_peek()) {         // if the next character is a newline, run it
            case 0x0A:
            case 0x0D:
            case 0x07:
            case 0x09:
                print_c(rb_read()); // otherwise insert a newline 
                break;
            default:
                print_c('\n');
        }
    }
}

void setup() {
#ifdef SERIAL_ENABLE
    Serial.begin(SERIAL_RATE);
    //for(uint8_t i = 0; i < 40; i++)
    //    sprint("\n");
    Serial.flush();
    sprint("Serial ready.\n\r");
#endif
    pinMode(LED_BUILTIN, OUTPUT);
    //pinMode(12, INPUT_PULLUP);  // latching pin to prevent stray character pressses
    
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

#ifdef PORT_ENABLE
    // setup interrupt for INT0 (port 2 bonded to port 12 - latching pin)
    EICRA |= 0x03;              // trigger on rising edge of INT0
    EIMSK |= 0x01;              // mask enable
#endif

    // reset carriage position
    rb_write('\n');
}

#ifdef PORT_ENABLE
ISR(INT0_vect) {
    uint8_t bytein = ((PIND & 0xE0) >> 5);
    bytein |= ((PINB & 0x1F) << 3);
    rb_write((bytein & 0x7F));
}
#endif

//uint8_t triggered = 0;
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
#endif

    if(rb_is_empty() == 0) {
        PORTB = (PORTB | 0x20);
        handle_buffer();
        PORTB = (PORTB & 0xDF);     // toggle status LED
        delay(100);
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
