#####################################################################
# Automatically generated by qtpromaker
#####################################################################

TEMPLATE = app
TARGET = shish
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += _GNU_SOURCE 
DEFINES += _FILE_OFFSET_BITS=64 _LARGEFILE64_SOURCE _LARGEFILE_SOURCE
DEFINES += PACKAGE_NAME="\\\"shish\\\"" PACKAGE_VERSION="\\\"0.8\\\""

#CONFIG += debug_and_release

CONFIG(debug, debug|release) {
  mac: TARGET = $$join(TARGET,,,_debug)
  win32: TARGET = $$join(TARGET,,d)
  linux: TARGET = $$join(TARGET,,d)

  DEFINES += DEBUG=1
}

#CONFIG(release, debug|release):message(Release build!) #will print
CONFIG(debug, debug|release):message(Debug build!) #no print

SOURCES *= \
  lib/buffer/buffer_close.c \
  lib/buffer/buffer_default.c \
  lib/buffer/buffer_dump.c \
  lib/buffer/buffer_feed.c \
  lib/buffer/buffer_flush.c \
  lib/buffer/buffer_free.c \
  lib/buffer/buffer_fromsa.c \
  lib/buffer/buffer_fromstr.c \
  lib/buffer/buffer_get.c \
  lib/buffer/buffer_getc.c \
  lib/buffer/buffer_get_until.c \
  lib/buffer/buffer_init.c \
  lib/buffer/buffer_mmapprivate.c \
  lib/buffer/buffer_mmapread.c \
  lib/buffer/buffer_mmapread_fd.c \
  lib/buffer/buffer_prefetch.c \
  lib/buffer/buffer_put.c \
  lib/buffer/buffer_putc.c \
  lib/buffer/buffer_putflush.c \
  lib/buffer/buffer_putm_internal.c \
  lib/buffer/buffer_putnlflush.c \
  lib/buffer/buffer_putnspace.c \
  lib/buffer/buffer_puts.c \
  lib/buffer/buffer_putsa.c \
  lib/buffer/buffer_putsflush.c \
  lib/buffer/buffer_putspace.c \
  lib/buffer/buffer_putulong.c \
  lib/buffer/buffer_skip_until.c \
  lib/buffer/buffer_stubborn.c \
  lib/buffer/buffer_stubborn2.c \
  lib/buffer/buffer_tosa.c \
  lib/buffer/buffer_truncfile.c \
  lib/byte/byte_chr.c \
  lib/byte/byte_copy.c \
  lib/byte/byte_copyr.c \
  lib/byte/byte_diff.c \
  lib/byte/byte_fill.c \
  lib/byte/byte_rchr.c \
  lib/byte/byte_zero.c \
  lib/fmt/fmt_long.c \
  lib/fmt/fmt_minus.c \
  lib/fmt/fmt_ulong.c \
  lib/fmt/fmt_ulong0.c \
  lib/fmt/fmt_xlong.c \
  lib/fmt/fmt_xlonglong.c \
  lib/mmap/mmap_private.c \
  lib/mmap/mmap_read.c \
  lib/mmap/mmap_read_fd.c \
  lib/mmap/mmap_unmap.c \
  lib/open/open_read.c \
  lib/open/open_temp.c \
  lib/open/open_trunc.c \
  lib/scan/scan_8long.c \
  lib/scan/scan_int.c \
  lib/scan/scan_long.c \
  lib/scan/scan_longlong.c \
  lib/scan/scan_uint.c \
  lib/scan/scan_ulong.c \
  lib/scan/scan_ulonglong.c \
  lib/shell/shell_alloc.c \
  lib/shell/shell_basename.c \
  lib/shell/shell_canonicalize.c \
  lib/shell/shell_dirname.c \
  lib/shell/shell_error.c \
  lib/shell/shell_errorn.c \
  lib/shell/shell_fnmatch.c \
  lib/shell/shell_getcwd.c \
  lib/shell/shell_gethome.c \
  lib/shell/shell_gethostname.c \
  lib/shell/shell_getopt.c \
  lib/shell/shell_init.c \
  lib/shell/shell_readlink.c \
  lib/shell/shell_realloc.c \
  lib/shell/shell_realpath.c \
  lib/shell/shell_strdup.c \
  lib/stralloc/stralloc_cat.c \
  lib/stralloc/stralloc_catb.c \
  lib/stralloc/stralloc_catc.c \
  lib/stralloc/stralloc_catlong0.c \
  lib/stralloc/stralloc_cats.c \
  lib/stralloc/stralloc_catulong0.c \
  lib/stralloc/stralloc_copy.c \
  lib/stralloc/stralloc_copyb.c \
  lib/stralloc/stralloc_copys.c \
  lib/stralloc/stralloc_diffs.c \
  lib/stralloc/stralloc_free.c \
  lib/stralloc/stralloc_init.c \
  lib/stralloc/stralloc_insertb.c \
  lib/stralloc/stralloc_move.c \
  lib/stralloc/stralloc_nul.c \
  lib/stralloc/stralloc_ready.c \
  lib/stralloc/stralloc_readyplus.c \
  lib/stralloc/stralloc_remove.c \
  lib/stralloc/stralloc_trunc.c \
  lib/stralloc/stralloc_write.c \
  lib/stralloc/stralloc_zero.c \
  lib/str/str_chr.c \
  lib/str/str_copy.c \
  lib/str/str_copyn.c \
  lib/str/str_diff.c \
  lib/str/str_len.c \
  lib/str/str_rchr.c \
  lib/uint32/uint32_prng.c \
  lib/uint32/uint32_random.c \
  lib/uint32/uint32_seed.c \
  src/builtin/builtin_basename.c \
  src/builtin/builtin_break.c \
  src/builtin/builtin_cd.c \
  src/builtin/builtin_dirname.c \
  src/builtin/builtin_dump.c \
  src/builtin/builtin_echo.c \
  src/builtin/builtin_error.c \
  src/builtin/builtin_eval.c \
  src/builtin/builtin_exec.c \
  src/builtin/builtin_exit.c \
  src/builtin/builtin_export.c \
  src/builtin/builtin_false.c \
  src/builtin/builtin_fdtable.c \
  src/builtin/builtin_hash.c \
  src/builtin/builtin_history.c \
  src/builtin/builtin_hostname.c \
  src/builtin/builtin_pwd.c \
  src/builtin/builtin_search.c \
  src/builtin/builtin_set.c \
  src/builtin/builtin_shift.c \
  src/builtin/builtin_source.c \
  src/builtin/builtin_table.c \
  src/builtin/builtin_test.c \
  src/builtin/builtin_true.c \
  src/builtin/builtin_unset.c \
  src/debug/debug_alloc.c \
  src/debug/debug_begin.c \
  src/debug/debug_char.c \
  src/debug/debug_end.c \
  src/debug/debug_error.c \
  src/debug/debug_free.c \
  src/debug/debug_list.c \
  src/debug/debug_memory.c \
  src/debug/debug_node.c \
  src/debug/debug_ptr.c \
  src/debug/debug_realloc.c \
  src/debug/debug_redir.c \
  src/debug/debug_space.c \
  src/debug/debug_str.c \
  src/debug/debug_stralloc.c \
  src/debug/debug_sublist.c \
  src/debug/debug_subnode.c \
  src/debug/debug_subst.c \
  src/debug/debug_ulong.c \
  src/debug/debug_unquoted.c \
  src/eval/eval_and_or.c \
  src/eval/eval_case.c \
  src/eval/eval_cmdlist.c \
  src/eval/eval_command.c \
  src/eval/eval_for.c \
  src/eval/eval_if.c \
  src/eval/eval_jump.c \
  src/eval/eval_loop.c \
  src/eval/eval_pipeline.c \
  src/eval/eval_pop.c \
  src/eval/eval_push.c \
  src/eval/eval_simple_command.c \
  src/eval/eval_subshell.c \
  src/eval/eval_tree.c \
  src/exec/exec_check.c \
  src/exec/exec_command.c \
  src/exec/exec_create.c \
  src/exec/exec_error.c \
  src/exec/exec_hash.c \
  src/exec/exec_path.c \
  src/exec/exec_program.c \
  src/exec/exec_search.c \
  src/expand/expand_arg.c \
  src/expand/expand_args.c \
  src/expand/expand_argv.c \
  src/expand/expand_arith.c \
  src/expand/expand_cat.c \
  src/expand/expand_catsa.c \
  src/expand/expand_command.c \
  src/expand/expand_copysa.c \
  src/expand/expand_escape.c \
  src/expand/expand_glob.c \
  src/expand/expand_param.c \
  src/expand/expand_unescape.c \
  src/expand/expand_vars.c \
  src/fdstack/fdstack_data.c \
  src/fdstack/fdstack_dump.c \
  src/fdstack/fdstack_flatten.c \
  src/fdstack/fdstack_fork.c \
  src/fdstack/fdstack_link.c \
  src/fdstack/fdstack_npipes.c \
  src/fdstack/fdstack_pipe.c \
  src/fdstack/fdstack_pop.c \
  src/fdstack/fdstack_push.c \
  src/fdstack/fdstack_root.c \
  src/fdstack/fdstack_search.c \
  src/fdstack/fdstack_unlink.c \
  src/fdstack/fdstack_unref.c \
  src/fdstack/fdstack_update.c \
  src/fdtable/fdtable_check.c \
  src/fdtable/fdtable_close.c \
  src/fdtable/fdtable_dump.c \
  src/fdtable/fdtable_dup.c \
  src/fdtable/fdtable_exec.c \
  src/fdtable/fdtable_gap.c \
  src/fdtable/fdtable_here.c \
  src/fdtable/fdtable_lazy.c \
  src/fdtable/fdtable_link.c \
  src/fdtable/fdtable_newfd.c \
  src/fdtable/fdtable_open.c \
  src/fdtable/fdtable_resolve.c \
  src/fdtable/fdtable_table.c \
  src/fdtable/fdtable_track.c \
  src/fdtable/fdtable_unexpected.c \
  src/fdtable/fdtable_unlink.c \
  src/fdtable/fdtable_up.c \
  src/fdtable/fdtable_wish.c \
  src/fd/fd_allocbuf.c \
  src/fd/fd_close.c \
  src/fd/fd_dump.c \
  src/fd/fd_dumplist.c \
  src/fd/fd_dup.c \
  src/fd/fd_error.c \
  src/fd/fd_exec.c \
  src/fd/fd_free.c \
  src/fd/fd_getname.c \
  src/fd/fd_here.c \
  src/fd/fd_init.c \
  src/fd/fd_list.c \
  src/fd/fd_mmap.c \
  src/fd/fd_needbuf.c \
  src/fd/fd_new.c \
  src/fd/fd_null.c \
  src/fd/fd_open.c \
  src/fd/fd_pipe.c \
  src/fd/fd_pop.c \
  src/fd/fd_print.c \
  src/fd/fd_push.c \
  src/fd/fd_reinit.c \
  src/fd/fd_setbuf.c \
  src/fd/fd_setfd.c \
  src/fd/fd_stat.c \
  src/fd/fd_string.c \
  src/fd/fd_subst.c \
  src/fd/fd_tempfile.c \
  src/history/history_advance.c \
  src/history/history_clear.c \
  src/history/history_cmdlen.c \
  src/history/history_free.c \
  src/history/history_load.c \
  src/history/history_next.c \
  src/history/history_prev.c \
  src/history/history_print.c \
  src/history/history_resize.c \
  src/history/history_save.c \
  src/history/history_set.c \
  src/job/job_fork.c \
  src/job/job_init.c \
  src/job/job_new.c \
  src/job/job_status.c \
  src/job/job_wait.c \
  src/parse/parse_and_or.c \
  src/parse/parse_arith.c \
  src/parse/parse_arith_binary.c \
  src/parse/parse_arith_expr.c \
  src/parse/parse_arith_paren.c \
  src/parse/parse_arith_unary.c \
  src/parse/parse_arith_value.c \
  src/parse/parse_bquoted.c \
  src/parse/parse_case.c \
  src/parse/parse_chartable.c \
  src/parse/parse_command.c \
  src/parse/parse_compound_list.c \
  src/parse/parse_dquoted.c \
  src/parse/parse_error.c \
  src/parse/parse_expect.c \
  src/parse/parse_for.c \
  src/parse/parse_function.c \
  src/parse/parse_getarg.c \
  src/parse/parse_gettok.c \
  src/parse/parse_grouping.c \
  src/parse/parse_here.c \
  src/parse/parse_if.c \
  src/parse/parse_init.c \
  src/parse/parse_keyword.c \
  src/parse/parse_list.c \
  src/parse/parse_loop.c \
  src/parse/parse_newnode.c \
  src/parse/parse_param.c \
  src/parse/parse_pipeline.c \
  src/parse/parse_simpletok.c \
  src/parse/parse_simple_command.c \
  src/parse/parse_skipspace.c \
  src/parse/parse_squoted.c \
  src/parse/parse_string.c \
  src/parse/parse_subst.c \
  src/parse/parse_tokens.c \
  src/parse/parse_tokname.c \
  src/parse/parse_unquoted.c \
  src/parse/parse_validname.c \
  src/parse/parse_word.c \
  src/prompt/prompt_escape.c \
  src/prompt/prompt_expand.c \
  src/prompt/prompt_parse.c \
  src/prompt/prompt_show.c \
  src/redir/redir_addhere.c \
  src/redir/redir_dup.c \
  src/redir/redir_eval.c \
  src/redir/redir_here.c \
  src/redir/redir_open.c \
  src/redir/redir_parse.c \
  src/redir/redir_source.c \
  src/sh/sh_error.c \
  src/sh/sh_errorn.c \
  src/sh/sh_exit.c \
  src/sh/sh_forked.c \
  src/sh/sh_getcwd.c \
  src/sh/sh_gethome.c \
  src/sh/sh_init.c \
  src/sh/sh_loop.c \
  src/sh/sh_main.c \
  src/sh/sh_msg.c \
  src/sh/sh_pop.c \
  src/sh/sh_popargs.c \
  src/sh/sh_push.c \
  src/sh/sh_pushargs.c \
  src/sh/sh_root.c \
  src/sh/sh_setargs.c \
  src/sh/sh_subshell.c \
  src/sh/sh_usage.c \
  src/sig/sig_block.c \
  src/sig/sig_unblock.c \
  src/source/source_flush.c \
  src/source/source_get.c \
  src/source/source_msg.c \
  src/source/source_newline.c \
  src/source/source_next.c \
  src/source/source_peek.c \
  src/source/source_peekn.c \
  src/source/source_pop.c \
  src/source/source_push.c \
  src/source/source_skip.c \
  src/term/term_ansi.c \
  src/term/term_attr.c \
  src/term/term_backspace.c \
  src/term/term_delete.c \
  src/term/term_end.c \
  src/term/term_escape.c \
  src/term/term_getline.c \
  src/term/term_home.c \
  src/term/term_init.c \
  src/term/term_insertc.c \
  src/term/term_left.c \
  src/term/term_newline.c \
  src/term/term_overwritec.c \
  src/term/term_read.c \
  src/term/term_restore.c \
  src/term/term_right.c \
  src/term/term_setline.c \
  src/term/term_winsize.c \
  src/tree/tree_count.c \
  src/tree/tree_free.c \
  src/tree/tree_newlink.c \
  src/tree/tree_newnode.c \
  src/tree/tree_print.c \
  src/tree/tree_printlist.c \
  src/vartab/vartab_add.c \
  src/vartab/vartab_cleanup.c \
  src/vartab/vartab_dump.c \
  src/vartab/vartab_hash.c \
  src/vartab/vartab_pop.c \
  src/vartab/vartab_print.c \
  src/vartab/vartab_push.c \
  src/vartab/vartab_root.c \
  src/vartab/vartab_search.c \
  src/var/var_bsearch.c \
  src/var/var_chflg.c \
  src/var/var_cleanup.c \
  src/var/var_copys.c \
  src/var/var_count.c \
  src/var/var_create.c \
  src/var/var_dump.c \
  src/var/var_export.c \
  src/var/var_get.c \
  src/var/var_hsearch.c \
  src/var/var_import.c \
  src/var/var_init.c \
  src/var/var_len.c \
  src/var/var_lexhash.c \
  src/var/var_print.c \
  src/var/var_rndhash.c \
  src/var/var_search.c \
  src/var/var_set.c \
  src/var/var_setsa.c \
  src/var/var_setvint.c \
  src/var/var_setvsa.c \
  src/var/var_unset.c \
  src/var/var_valid.c \
  src/var/var_value.c \
  src/var/var_vdefault.c \
  src/var/var_vlen.c

HEADERS *= \
  lib/buffer.h \
  lib/byte.h \
  lib/fmt.h \
  lib/mmap.h \
  lib/open.h \
  lib/safemult.h \
  lib/scan.h \
  lib/shell.h \
  lib/str.h \
  lib/stralloc.h \
  lib/uint16.h \
  lib/uint32.h \
  lib/uint64.h \
  src/builtin.h \
  src/debug.h \
  src/eval.h \
  src/exec.h \
  src/expand.h \
  src/fd.h \
  src/fdstack.h \
  src/fdtable.h \
  src/history.h \
  src/job.h \
  src/parse.h \
  src/prompt.h \
  src/redir.h \
  src/sh.h \
  src/sig.h \
  src/source.h \
  src/term.h \
  src/tree.h \
  src/var.h \
  src/vartab.h

PATHS *= \
    . \
    ./lib \
    ./build \
    ./src \
    ./autom4te.cache \
    ./obj \
    ./scripts \
    ./m4 \
    ./doc \
    ./lib/open \
    ./lib/uint32 \
    ./lib/str \
    ./lib/stralloc \
    ./lib/buffer \
    ./lib/fmt \
    ./lib/mmap \
    ./lib/byte \
    ./lib/shell \
    ./lib/scan \
    ./src/eval \
    ./src/exec \
    ./src/vartab \
    ./src/builtin \
    ./src/term \
    ./src/redir \
    ./src/history \
    ./src/debug \
    ./src/sig \
    ./src/fd \
    ./src/parse \
    ./src/prompt \
    ./src/job \
    ./src/expand \
    ./src/fdstack \
    ./src/fdtable \
    ./src/tree \
    ./src/var \
    ./src/source \
    ./src/sh \
    ./doc/man \
    ./doc/posix

DEPENDPATH *= $$PATHS

INCLUDEPATH *= $$PATHS

DISTFILES += \
    src/parse/arith-grammar.txt
