#ifndef EXPR_H
#define EXPR_H

#include "parse.h"

/* tokens */
enum op_id {
  XI_EOF = 0,
  XI_ADD,              /* +   */
  XI_SUB,              /* -   */
  XI_MUL,              /* *   */
  XI_DIV,              /* /   */
  XI_MOD,              /* %   */
  XI_LSHIFT,           /* <<  */
  XI_RSHIFT,           /* >>  */
  XI_LT,               /* <   */
  XI_GT,               /* >   */
  XI_LE,               /* <=  */
  XI_GE,               /* >=  */
  XI_EQUAL,            /* ==  */
  XI_NEQUAL,           /* !=  */
  XI_BAND,             /* &   */
  XI_BXOR,             /* ^   */
  XI_BOR,              /* |   */
  XI_LAND,             /* &&  */
  XI_LOR,              /* ||  */
  XI_QUEST,            /* ?   */
  XI_COLON,            /* :   */
  XI_ASSIGN,           /* =   */
  XI_ADD_ASSIGN,       /* +=  */
  XI_SUB_ASSIGN,       /* -=  */
  XI_MUL_ASSIGN,       /* *=  */
  XI_DIV_ASSIGN,       /* /=  */
  XI_MOD_ASSIGN,       /* %=  */
  XI_LSHIFT_ASSIGN,    /* <<= */
  XI_RSHIFT_ASSIGN,    /* >>= */
  XI_BAND_ASSIGN,      /* &=  */
  XI_BXOR_ASSIGN,      /* ^=  */
  XI_BOR_ASSIGN,       /* |=  */
  XI_INC,              /* ++  */
  XI_DEC,              /* --  */
  XI_UADD,             /* +   */
  XI_USUB,             /* -   */
  XI_BNOT,             /* ~   */
  XI_LNOT,             /* !   */
  XI_LP,               /* (   */
  XI_RP,               /* )   */
  XI_COMMA,            /* ,   */
  XI_END,              /* op-delimiter */
  XI_NUM,              /* numerical value */
  XI_VAR,              /* variable name */
  XI_ERROR,            /* No token found */
};

enum expr_id {
  X_EOF           = (1ll << XI_EOF),
  X_ADD           = (1ll << XI_ADD),
  X_SUB           = (1ll << XI_SUB),
  X_MUL           = (1ll << XI_MUL),
  X_DIV           = (1ll << XI_DIV),
  X_MOD           = (1ll << XI_MOD),
  X_LSHIFT        = (1ll << XI_LSHIFT),
  X_RSHIFT        = (1ll << XI_RSHIFT),
  X_LT            = (1ll << XI_LT),
  X_GT            = (1ll << XI_GT),
  X_LE            = (1ll << XI_LE),
  X_GE            = (1ll << XI_GE),
  X_EQUAL         = (1ll << XI_EQUAL),
  X_NEQUAL        = (1ll << XI_NEQUAL),
  X_BAND          = (1ll << XI_BAND),
  X_BXOR          = (1ll << XI_BXOR),
  X_BOR           = (1ll << XI_BOR),
  X_LAND          = (1ll << XI_LAND),
  X_LOR           = (1ll << XI_LOR),
  X_QUEST         = (1ll << XI_QUEST),
  X_COLON         = (1ll << XI_COLON),
  X_ASSIGN        = (1ll << XI_ASSIGN),
  X_ADD_ASSIGN    = (1ll << XI_ADD_ASSIGN),
  X_SUB_ASSIGN    = (1ll << XI_SUB_ASSIGN),
  X_MUL_ASSIGN    = (1ll << XI_MUL_ASSIGN),
  X_DIV_ASSIGN    = (1ll << XI_DIV_ASSIGN),
  X_MOD_ASSIGN    = (1ll << XI_MOD_ASSIGN),
  X_LSHIFT_ASSIGN = (1ll << XI_LSHIFT_ASSIGN),
  X_RSHIFT_ASSIGN = (1ll << XI_RSHIFT_ASSIGN),
  X_BAND_ASSIGN   = (1ll << XI_BAND_ASSIGN),
  X_BXOR_ASSIGN   = (1ll << XI_BXOR_ASSIGN),
  X_BOR_ASSIGN    = (1ll << XI_BOR_ASSIGN),
  X_INC           = (1ll << XI_INC),
  X_DEC           = (1ll << XI_DEC),
  X_UADD          = (1ll << XI_UADD),
  X_USUB          = (1ll << XI_USUB),
  X_BNOT          = (1ll << XI_BNOT),
  X_LNOT          = (1ll << XI_LNOT),
  X_LP            = (1ll << XI_LP),
  X_RP            = (1ll << XI_RP),
  X_COMMA         = (1ll << XI_COMMA),
  X_END           = (1ll << XI_END),
  X_NUM           = (1ll << XI_NUM),
  X_VAR           = (1ll << XI_VAR),
  X_ERROR         = (1ll << XI_ERROR),
};

const char *expr_tokname(uint64 tok);
uint64 expr_gettok(struct parser *p, int tempflags);

int expr_expect(struct parser *p, int tempflags, uint64 toks, union node *nfree);

struct nargarith* expr_newnode(struct parser *p, enum op_id op);


#endif /* EXPR_H */
