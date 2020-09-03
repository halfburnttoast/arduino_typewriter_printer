#ifndef KEYCODES_H
#define KEYCODES_H
// encoded ascii values 
//      last nibble represents the control code
//      4th bit indicates whether shift is pressed or not 
//      first three bits indicate the scan address
//        IE:   CCCC.SAAA
const uint8_t keycodes[0x80] = {
    0x00,       //NUL
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x05,
    
    0x26,       // BS
    0x05,
    0x66,       // LF
    0x00,
    0x00,
    0x66,       // CR
    0x00,
    0x00,
//--------------------
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,

    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
//--------------------
    0x76,       // space
    0x58,       // !
    0x7D,       // "
    0x48,       // # 
    0x49,       // $
    0x28,       // %
    0x29,       // ^
    0x38,       // &

    0x00,       // )
    0x08,       // (
    0x39,       // *
    0x09,
    0x70,       // ,
    0x00,
    0x71,       // .
    0x00,
//--------------------    
    0x01,       // 0
    0x50,       // 1
    0x51,       // 2
    0x40,       // 3
    0x41,       // 4
    0x20,       // 5
    0x21,       // 6
    0x30,       // 7

    0x31,       // 8
    0x00,       // 9
    0x7B,       // :
    0x73,       // ;
    0x00,
    0x11,       // =
    0x00,
    0x00,
//--------------------
    0x00,
    0x6D,       // A
    0x0C,       // B
    0x0B,       // C
    0x3D,       // D
    0x5A,       // E
    0x5B,       // F
    0x5D,       // G

    0x4B,       // H
    0x2C,       // I
    0x4D,       // J
    0x2B,       // K
    0x2D,       // L
    0x1C,       // M
    0x1A,       // N
    0x3A,       // O
    0x3C,       // P
//--------------------
    0x6A,       // Q
    0x5C,       // R
    0x3B,       // S
    0x4A,       // T
    0x2A,       // U
    0x0A,       // V
    0x6C,       // W
    0x1B,       // X

    0x4C,       // Y
    0x6B,       // Z
    0x00,
    0x29,       // ^
    0x18,       // _
    0x00,    
    0x00,
    0x00,
    0x65,       // a
    0x04,       // b
//--------------------
    0x03,       // c
    0x35,       // d
    0x52,       // e
    0x53,       // f
    0x55,       // g
    0x43,       // h
    0x24,       // i
    0x45,       // j

    0x23,       // k
    0x25,       // l
    0x14,       // m
    0x12,       // n
    0x32,       // o
    0x34,       // p
    0x62,       // q
    0x54,       // r
//--------------------
    0x33,       // s
    0x42,       // t
    0x22,       // u
    0x02,       // v
    0x64,       // w
    0x13,       // x
    0x44,       // y
    0x63,       // z
};

#endif
