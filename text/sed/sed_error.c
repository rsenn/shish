#include "sed_internal.h"

const char*
sed_error(int code) {
  switch(code) {
    case SED_OK: return "no error";
    case SED_ENOMEM: return "out of memory";
    case SED_EADDR: return "invalid address";
    case SED_ECMD: return "unknown command, or wrong number of addresses for it";
    case SED_ELABEL: return "undefined label";
    case SED_EBRACE: return "unbalanced { }";
    case SED_EDELIM: return "invalid s/// or y/// delimiters";
    case SED_EUNTERM: return "unterminated command";
    case SED_EREGEX: return "invalid regular expression";
    case SED_ESIZE: return "script too large";
    default: return "unknown error";
  }
}
