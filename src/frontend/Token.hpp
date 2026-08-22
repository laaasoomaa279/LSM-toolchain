#ifndef LSM_TOKEN_HPP
#define LSM_TOKEN_HPP

#include <string_view>

enum class LsmTokenType {
    
    FullDev, Unsafe, Asm, Volatile, Ptr,
    SetReg, GetReg, CpuHalt, CpuCli, CpuSti,
    PortIn, PortOut, MmioRead, MmioWrite,

    
    RegRax, RegRbx, RegRcx, RegRdx, RegRsi, RegRdi,
    RegRbp, RegRsp, RegR8, RegR9, RegR10, RegR11,
    RegR12, RegR13, RegR14, RegR15,

    
    Inb, Outb, Inw, Outw, Inl, Outl,

    
    AtomicAdd, AtomicSub, AtomicCas, AtomicLoad, AtomicStore,

    
    DisableInterrupts, EnableInterrupts, Halt,

    
    True, False, Print, Input, Fct, Proc, Rec, Array, Return,
    If, Else, While, For, In, To, Break, Continue,
    Switch, Case, Default, Class, Extends, This, New,
    Import, Go, Chan, Defer, Interface, Enum, Type,
    And, Or, Not, Try, Catch, Throw, Let, Nil, Slice, Extern,

    
    Match, Ok, Err, Some, None,

    
    Hash, At,

    
    Ident, Int, Float, String,

    
    Plus, PlusEq, PlusPlus, Minus, MinusEq, MinusMinus,
    Star, StarEq, Slash, SlashEq, Percent,
    Eq, EqEq, NotEq, Greater, GreaterEq, Less, LessEq,
    Amp, AmpAmp, Pipe, PipePipe, Caret, Tilde, Shl, Shr,

    
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Dot, Colon, Semicolon, Question,
    FatArrow, ThinArrow, ChanSendRecv, Range, RangeInclusive, QuestionSafe,

    Eof
};

struct Token {
    LsmTokenType type;
    std::string_view value;
    size_t line = 1;
    size_t col = 1;
    size_t offset = 0;
};

inline bool isAssignOp(LsmTokenType t) {
    return t == LsmTokenType::Eq || t == LsmTokenType::PlusEq ||
           t == LsmTokenType::MinusEq || t == LsmTokenType::StarEq ||
           t == LsmTokenType::SlashEq;
}

#endif 