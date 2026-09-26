#include "dfa_prog.h"

const char*
dfa_error(int code) {
  switch(code) {
    case DFA_OK: return "no error";
    case DFA_ENOMEM: return "out of memory";
    case DFA_EPAREN: return "unbalanced ( )";
    case DFA_EBRACKET: return "unterminated [ ]";
    case DFA_EBRACE: return "invalid \\{m,n\\}";
    case DFA_ERANGE: return "invalid range in [ ]";
    case DFA_ECLASS: return "unknown [:class:]";
    case DFA_ESUBREG: return "invalid back-reference";
    case DFA_EBADRPT: return "repetition with nothing to repeat";
    case DFA_EESCAPE: return "trailing backslash";
    case DFA_ESIZE: return "pattern too large";
    default: return "unknown error";
  }
}
