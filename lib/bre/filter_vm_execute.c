#include "../bre.h"
#include "../stralloc.h"

int
filter_vm_execute(const struct vm_bytecode* prog, struct filter_context* ctx) {
  int ip = 0;
  while(ip < (int)prog->count) {
    struct vm_instruction* ins = &prog->code[ip];
    switch(ins->op) {
      case OP_HALT: return 0;
      case OP_MATCH:
        ctx->reg_bool = bre_match_exec(&ctx->bre, ins->str_ptr, ctx->pattern_space.s);
        break;
      case OP_NOT: ctx->reg_bool = !ctx->reg_bool; break;
      case OP_BRAF:
        if(!ctx->reg_bool) {
          ip += ins->arg_int;
          continue;
        }
        break;
      case OP_BRAT:
        if(ctx->reg_bool) {
          ip += ins->arg_int;
          continue;
        }
        break;
      case OP_ADDR_RANGE:
        if(!ctx->in_range && ctx->reg_bool)
          ctx->in_range = 1;
        ctx->reg_bool = ctx->in_range;
        if(ctx->in_range && ins->arg_int == 2 && ctx->reg_bool)
          ctx->in_range = 0;
        break;
      case OP_PRINT: break;
      case OP_SUBST:
        stralloc_zero(&ctx->output);
        bre_subst_expand(ins->str_ptr, &ctx->bre, &ctx->output);
        stralloc_copy(&ctx->pattern_space, &ctx->output);
        break;
      default: break;
    }
    ip++;
  }
  return 0;
}
