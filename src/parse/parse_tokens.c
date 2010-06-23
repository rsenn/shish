#include "tree.h"
#include "parse.h"

/* all the tokens that the parser understands: 
 * ----------------------------------------------------------------------- */
struct token parse_tokens[] =
{
  /* control operators */
  { 1, "EOF"    }, /* T_EOF     - end of file */
  { 0, "NL"     }, /* T_NL      - newline */
  { 0, ";"      }, /* T_SEMI    - semicolon */
  { 1, ";;"     }, /* T_ENDCASE - end of case */
  { 0, "&"      }, /* T_BACKGND - background operator */
  { 0, "&&"     }, /* T_AND     - boolean AND */
  { 0, "|"      }, /* T_PIPE    - pipe operator */
  { 0, "||"     }, /* T_OR      - boolean OR */
  { 0, "("      }, /* T_LP      - left parenthesis ( */
  { 1, ")"      }, /* T_RP      - right parenthesis ) */
  /* not really a control operator, but used
     as ending token when parsing backquoted
     command lists:
   */
  { 1, "`"      }, /* T_BQ      - backquote ` */
  
  /* special tokens */
  { 0, "name"   }, /* T_NAME    - name */
  { 0, "word"   }, /* T_WORD    - word */
  { 0, "assign" }, /* T_ASSIGN  - assignment */
  { 0, "redir"  }, /* T_REDIR   - redirection */

  /* keyword tokens (sorted) */
  { 0, "!"      }, /* T_NOT - boolean NOT */
  { 0, "case"   }, /* T_CASE  */
  { 0, "do"     }, /* T_DO    */
  { 1, "done"   }, /* T_DONE  */
  { 1, "elif"   }, /* T_ELIF  */
  { 1, "else"   }, /* T_ELSE  */
  { 1, "esac"   }, /* T_ESAC  */
  { 1, "fi"     }, /* T_FI    */
  { 0, "for"    }, /* T_FOR   */
  { 0, "if"     }, /* T_IF    */
  { 0, "in"     }, /* T_IN    */
  { 0, "then"   }, /* T_THEN  */
  { 0, "until"  }, /* T_UNTIL */
  { 0, "while"  }, /* T_WHILE */
  { 0, "{"      }, /* T_BEGIN */
  { 1, "}"      }, /* T_END   */
  /* keywords */
};

