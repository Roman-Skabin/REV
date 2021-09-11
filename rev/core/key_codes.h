//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#if REV_PLATFORM_WIN64
    #undef DELETE
#endif

namespace REV
{
    enum class KEY
    {
        FIRST       = 0x08,
    
        ESC         = 0x1B,
        TAB         = 0x09,
        BACK        = 0x08,
        CLEAR       = 0x0C,
        ENTER       = 0x0D,
        SPACE       = 0x20,
    
        LCTRL       = 0xA2,
        LSHIFT      = 0xA0,
        LALT        = 0xA4,
    
        RCTRL       = 0xA3,
        RSHIFT      = 0xA1,
        RALT        = 0xA5,
        
        CTRL        = 0x11,
        ALT         = 0x12,
        SHIFT       = 0x10,
    
        PRINT       = 0x2A,
        SNAPSHOT    = 0x2C,
    
        CAPSLOCK    = 0x14,
        NUMLOCK     = 0x90,
        SCROLL      = 0x91,
        
        HOME        = 0x24,
        END         = 0x23,
        INSERT      = 0x2D,
        DELETE      = 0x2E,

        LEFT        = 0x25,
        UP          = 0x26,
        RIGHT       = 0x27,
        DOWN        = 0x28,
    
        LWIN        = 0x5B,
        RWIN        = 0x5C,
    
        _0          = 0x30,
        _1          = 0x31,
        _2          = 0x32,
        _3          = 0x33,
        _4          = 0x34,
        _5          = 0x35,
        _6          = 0x36,
        _7          = 0x37,
        _8          = 0x38,
        _9          = 0x39,
    
        A           = 0x41,
        B           = 0x42,
        C           = 0x43,
        D           = 0x44,
        E           = 0x45,
        F           = 0x46,
        G           = 0x47,
        H           = 0x48,
        I           = 0x49,
        J           = 0x4A,
        K           = 0x4B,
        L           = 0x4C,
        M           = 0x4D,
        N           = 0x4E,
        O           = 0x4F,
        P           = 0x50,
        Q           = 0x51,
        R           = 0x52,
        S           = 0x53,
        T           = 0x54,
        U           = 0x55,
        V           = 0x56,
        W           = 0x57,
        X           = 0x58,
        Y           = 0x59,
        Z           = 0x5A,
    
        NUM_0       = 0x60,
        NUM_1       = 0x61,
        NUM_2       = 0x62,
        NUM_3       = 0x63,
        NUM_4       = 0x64,
        NUM_5       = 0x65,
        NUM_6       = 0x66,
        NUM_7       = 0x67,
        NUM_8       = 0x68,
        NUM_9       = 0x69,
        NUM_ADD     = 0x6B,
        NUM_SUB     = 0x6D,
        NUM_MUL     = 0x6A,
        NUM_DIV     = 0x6F,
        NUM_DECIMAL = 0x6E,
        NUM_SLASH   = 0x6C,
    
        F1          = 0x70,
        F2          = 0x71,
        F3          = 0x72,
        F4          = 0x73,
        F5          = 0x74,
        F6          = 0x75,
        F7          = 0x76,
        F8          = 0x77,
        F9          = 0x78,
        F10         = 0x79,
        F11         = 0x7A,
        F12         = 0x7B,
        F13         = 0x7C,
        F14         = 0x7D,
        F15         = 0x7E,
        F16         = 0x7F,
        F17         = 0x80,
        F18         = 0x81,
        F19         = 0x82,
        F20         = 0x83,
        F21         = 0x84,
        F22         = 0x85,
        F23         = 0x86,
        F24         = 0x87,
    
        PLUS        = 0xBB,
        MINUS       = 0xBD,
    
        DOT         = 0xBE,
        COMMA       = 0xBC,
    
        OEM_1       = 0xBA, // [;:] for US
        OEM_2       = 0xBF, // [/?] for US
        OEM_3       = 0xC0, // [`~] for US
        OEM_4       = 0xDB, // [[{] for US
        OEM_5       = 0xDC, // [\|] for US
        OEM_6       = 0xDD, // []}] for US
        OEM_7       = 0xDE, // ['"] for US
        OEM_8       = 0xDF,
        OEM_102     = 0xE2, // [<>] or [\|] on RT 102-key kbd.

        LAST        = 0xFF,
        MAX
    };
}
