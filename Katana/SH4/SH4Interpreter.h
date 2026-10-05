#pragma once

typedef void (SH4Interpreter::*Handler)(uint16_t);

struct InstructionInfo
{
    const char* Pattern;   //exactly as the manual writes it
    Handler Function;
    const char* Mnemonic;
};

class SH4Interpreter
{
public:
    
};
