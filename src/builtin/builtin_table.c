#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../builtin.h"
#include "../history.h"

#ifdef HAVE_BUILTIN_CONFIG_H
#include "src/builtin_config.h"
#endif

#ifndef BUILTIN_ALIAS
#define BUILTIN_ALIAS 1
#endif
#ifndef BUILTIN_BREAK
#define BUILTIN_BREAK 1
#endif
#ifndef BUILTIN_CAT
#define BUILTIN_CAT 0
#endif
#ifndef BUILTIN_CD
#define BUILTIN_CD 1
#endif
#ifndef BUILTIN_CHMOD
#define BUILTIN_CHMOD 0
#endif
#ifndef BUILTIN_CP
#define BUILTIN_CP 0
#endif
#ifndef BUILTIN_DATE
#define BUILTIN_DATE 0
#endif
#ifndef BUILTIN_ENV
#define BUILTIN_ENV 0
#endif
#ifndef BUILTIN_ID
#define BUILTIN_ID 0
#endif
#ifndef BUILTIN_DIRS
#define BUILTIN_DIRS 0
#endif
#ifndef BUILTIN_POPD
#define BUILTIN_POPD 0
#endif
#ifndef BUILTIN_PUSHD
#define BUILTIN_PUSHD 0
#endif
#ifndef BUILTIN_MV
#define BUILTIN_MV 0
#endif
#ifndef BUILTIN_SORT
#define BUILTIN_SORT 0
#endif
#ifndef BUILTIN_SPLIT
#define BUILTIN_SPLIT 0
#endif
#ifndef BUILTIN_TAIL
#define BUILTIN_TAIL 0
#endif
#ifndef BUILTIN_CONTINUE
#define BUILTIN_CONTINUE 1
#endif
#ifndef BUILTIN_DUMP
#define BUILTIN_DUMP 1
#endif
#ifndef BUILTIN_ECHO
#define BUILTIN_ECHO 1
#endif
#ifndef BUILTIN_EVAL
#define BUILTIN_EVAL 1
#endif
#ifndef BUILTIN_EXEC
#define BUILTIN_EXEC 1
#endif
#ifndef BUILTIN_EXIT
#define BUILTIN_EXIT 1
#endif
#ifndef BUILTIN_EXPORT
#define BUILTIN_EXPORT 1
#endif
#ifndef BUILTIN_EXPR
#define BUILTIN_EXPR 1
#endif
#ifndef BUILTIN_FALSE
#define BUILTIN_FALSE 1
#endif
#ifndef BUILTIN_FDTABLE
#define BUILTIN_FDTABLE 1
#endif
#ifndef BUILTIN_HASH
#define BUILTIN_HASH 1
#endif
#ifndef BUILTIN_GETOPTS
#define BUILTIN_GETOPTS 1
#endif
#ifndef BUILTIN_HELP
#define BUILTIN_HELP 0
#endif
#ifndef BUILTIN_HISTORY
#define BUILTIN_HISTORY 1
#endif
#ifndef BUILTIN_LOCAL
#define BUILTIN_LOCAL 1
#endif
#ifndef BUILTIN_LINK
#define BUILTIN_LINK 0
#endif
#ifndef BUILTIN_UNLINK
#define BUILTIN_UNLINK 0
#endif
#ifndef BUILTIN_LN
#define BUILTIN_LN 0
#endif
#ifndef BUILTIN_LS
#define BUILTIN_LS 0
#endif
#ifndef BUILTIN_MKDIR
#define BUILTIN_MKDIR 0
#endif
#ifndef BUILTIN_BASENAME
#define BUILTIN_BASENAME 1
#endif
#ifndef BUILTIN_COMMAND
#define BUILTIN_COMMAND 1
#endif
#ifndef BUILTIN_DIGEST
#define BUILTIN_DIGEST 0
#endif
#ifndef BUILTIN_DIRNAME
#define BUILTIN_DIRNAME 1
#endif
#ifndef BUILTIN_HOSTNAME
#define BUILTIN_HOSTNAME 0
#endif
#ifndef BUILTIN_JOBS
#define BUILTIN_JOBS 1
#endif
#ifndef BUILTIN_KILL
#define BUILTIN_KILL 1
#endif
#ifndef BUILTIN_MKTEMP
#define BUILTIN_MKTEMP 0
#endif
#ifndef BUILTIN_PRINTF
#define BUILTIN_PRINTF 1
#endif
#ifndef BUILTIN_HEAD
#define BUILTIN_HEAD 0
#endif
#ifndef BUILTIN_UNIQ
#define BUILTIN_UNIQ 0
#endif
#ifndef BUILTIN_CUT
#define BUILTIN_CUT 0
#endif
#ifndef BUILTIN_NL
#define BUILTIN_NL 0
#endif
#ifndef BUILTIN_TR
#define BUILTIN_TR 0
#endif
#ifndef BUILTIN_PASTE
#define BUILTIN_PASTE 0
#endif
#ifndef BUILTIN_TEE
#define BUILTIN_TEE 0
#endif
#ifndef BUILTIN_TEST
#define BUILTIN_TEST 1
#endif
#ifndef BUILTIN_TIMES
#define BUILTIN_TIMES 1
#endif
#ifndef BUILTIN_TIMEOUT
#define BUILTIN_TIMEOUT 0
#endif
#ifndef BUILTIN_TOUCH
#define BUILTIN_TOUCH 0
#endif
#ifndef BUILTIN_TRAP
#define BUILTIN_TRAP 1
#endif
#ifndef BUILTIN_PWD
#define BUILTIN_PWD 1
#endif
#ifndef BUILTIN_SET
#define BUILTIN_SET 1
#endif
#ifndef BUILTIN_READ
#define BUILTIN_READ 1
#endif
#ifndef BUILTIN_READLINK
#define BUILTIN_READLINK 0
#endif
#ifndef BUILTIN_READONLY
#define BUILTIN_READONLY 1
#endif
#ifndef BUILTIN_REALPATH
#define BUILTIN_REALPATH 0
#endif
#ifndef BUILTIN_RETURN
#define BUILTIN_RETURN 1
#endif
#ifndef BUILTIN_RM
#define BUILTIN_RM 0
#endif
#ifndef BUILTIN_RMDIR
#define BUILTIN_RMDIR 0
#endif
#ifndef BUILTIN_SED
#define BUILTIN_SED 0
#endif
#ifndef BUILTIN_SHIFT
#define BUILTIN_SHIFT 1
#endif
#ifndef BUILTIN_SLEEP
#define BUILTIN_SLEEP 0
#endif
#ifndef BUILTIN_SOURCE
#define BUILTIN_SOURCE 1
#endif
#ifndef BUILTIN_TRUE
#define BUILTIN_TRUE 1
#endif
#ifndef BUILTIN_TYPE
#define BUILTIN_TYPE 1
#endif
#ifndef BUILTIN_ULIMIT
#define BUILTIN_ULIMIT 1
#endif
#ifndef BUILTIN_UMASK
#define BUILTIN_UMASK 1
#endif
#ifndef BUILTIN_UNSET
#define BUILTIN_UNSET 1
#endif
#ifndef BUILTIN_UNAME
#define BUILTIN_UNAME 0
#endif
#ifndef BUILTIN_WAIT
#define BUILTIN_WAIT 1
#endif
#ifndef BUILTIN_WC
#define BUILTIN_WC 0
#endif
#ifndef BUILTIN_WHICH
#define BUILTIN_WHICH 0
#endif

/* builtin lookup table
 * ----------------------------------------------------------------------- */
struct builtin_cmd builtin_table[] = {
#if BUILTIN_SOURCE
    {".", &builtin_source, B_SPECIAL, "file [arguments]", help_source},
#endif
#if BUILTIN_TRUE
    {":", &builtin_true, B_SPECIAL, "", help_true},
#endif
#if BUILTIN_ALIAS
    {"alias", &builtin_alias, B_DEFAULT, "[-p] [name[=value] ...]", help_alias},
#endif
#if BUILTIN_AWK
    {"awk",
     &builtin_awk,
     B_DEFAULT,
     "[-F sep] [-v assign]... [-f progfile | 'program'] [file...]",
     help_awk},
#endif
#if BUILTIN_BASENAME
    {"basename", &builtin_basename, B_DEFAULT, "path [suffix]", help_basename},
#endif
#if BUILTIN_ID
    {"id", &builtin_id, B_DEFAULT, "[-Ggu] [-nr] [user]", help_id},
#endif
#if BUILTIN_JOBS
    {"bg", &builtin_bg, B_DEFAULT, "[job...]", help_bg},
#endif
#if BUILTIN_BREAK
    {"break", &builtin_break, B_SPECIAL, "[n]", help_break},
#endif
#if BUILTIN_CAT
    {"cat", &builtin_cat, B_DEFAULT, "[-nb] [FILE]...", help_cat, &cat_filter},
#endif
#if BUILTIN_CD
    {"cd", &builtin_cd, B_DEFAULT, "[-L|-P] [directory]", help_cd},
#endif
#if BUILTIN_CHMOD
    {"chmod", &builtin_chmod, B_DEFAULT, "[-v] [FILE]...", help_chmod},
#endif
#if BUILTIN_COMMAND
    {"command", &builtin_command, B_DEFAULT, "[-pVv] command [arg ...]", help_command},
#endif
#if BUILTIN_CONTINUE
    {"continue", &builtin_break, B_SPECIAL, "[n]", help_break},
#endif
#if BUILTIN_CP
    {"cp", &builtin_cpmv, B_DEFAULT, "[-RrHLPfipnva] source... target", help_cp},
#endif
#if BUILTIN_DATE
    {"date", &builtin_date, B_DEFAULT, "[-u] [+format]", help_date},
#endif
#if BUILTIN_DIRNAME
    {"dirname", &builtin_dirname, B_DEFAULT, "path", help_dirname},
#endif
#if BUILTIN_DUMP
    {"dump",
     &builtin_dump,
     B_DEFAULT,
     "[-Fvl"
#ifdef DEBUG_OUTPUT
     "tsjf"
#endif
     "] [-u FD]",
     help_dump},
#endif
#if BUILTIN_ECHO
    {"echo", &builtin_echo, B_DEFAULT, "[-ne] [arg ...]", help_echo},
#endif
#if BUILTIN_ENV
    {"env", &builtin_env, B_DEFAULT, "[-i] [-u name]... [name=value]... [utility [argument...]]", help_env},
#endif
#if BUILTIN_EVAL
    {"eval", &builtin_eval, B_SPECIAL, "[args]", help_eval},
#endif
#if BUILTIN_EXEC
    {"exec", &builtin_exec, B_EXEC, "[-cl] [-a name] [cmd [args]]", help_exec},
#endif
#if BUILTIN_EXIT
    {"exit", &builtin_exit, B_SPECIAL, "[exitcode]", help_exit},
#endif
#if BUILTIN_EXPORT
    {"export", &builtin_export, B_SPECIAL, "[-np] [name=[value]]", help_export},
#endif
#if BUILTIN_EXPR
    {"expr", &builtin_expr, B_DEFAULT, "[expression]", help_expr},
#endif
#if BUILTIN_FALSE
    {"false", &builtin_false, B_DEFAULT, "", help_false},
#endif
#if BUILTIN_FDTABLE
    {"fdtable", &builtin_fdtable, B_DEFAULT, "[-u FD]", help_fdtable},
#endif
#if BUILTIN_FIND
    {"find", &builtin_find, B_DEFAULT, "[path...] [expression]", help_find},
#endif
#if BUILTIN_JOBS
    {"fg", &builtin_fg, B_DEFAULT, "[job...]", help_fg},
#endif
#if BUILTIN_GETOPTS
    {"getopts", &builtin_getopts, B_DEFAULT, "optstring name [arg ... ]", help_getopts},
#endif
#if BUILTIN_GREP
    {"grep", &builtin_grep, B_DEFAULT, "[options] <pattern-list>", help_grep, &grep_filter},
#endif
#if BUILTIN_HASH
    {"hash",
     &builtin_hash,
     B_DEFAULT,
     "[-lr] [-p pathname name] [-d name ...] [name ...]",
     help_hash},
#endif
#if BUILTIN_HELP
    {"help", &builtin_help, B_DEFAULT, "[command]", help_help},
#endif
#if BUILTIN_HISTORY
    {"history", &builtin_history, B_DEFAULT, "[-c]", help_history},
#endif
#if BUILTIN_HOSTNAME
    {"hostname", &builtin_hostname, B_DEFAULT, "[name]", help_hostname},
#endif
#if BUILTIN_JOBS
    {"jobs", &builtin_jobs, B_DEFAULT, "[-lnprs] [job...]", help_jobs},
#endif
#if BUILTIN_KILL
    {"kill", &builtin_kill, B_DEFAULT, "[-signal|-number] pid|%job ...", help_kill},
#endif
#if BUILTIN_UNLINK
    {"unlink", &builtin_unlink, B_DEFAULT, "file", help_unlink},
#endif
#if BUILTIN_LINK
    {"link", &builtin_link, B_DEFAULT, "file1 file2", help_link},
#endif
#if BUILTIN_LN
    {"ln", &builtin_ln, B_DEFAULT, "[-sfv]", help_ln},
#endif
#if BUILTIN_LOCAL
    {"local", &builtin_local, B_DEFAULT, "[option] name[=value] ...", help_local},
#endif
#if BUILTIN_LS
    {"ls", &builtin_ls, B_DEFAULT, "[-adl1] [file...]", help_ls},
#endif
#if BUILTIN_DIGEST
    {"md5sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
#endif
#if BUILTIN_DIRS
    {"dirs", &builtin_dirs, B_DEFAULT, "[-clpv] [+N | -N]", help_dirs},
#endif
#if BUILTIN_POPD
    {"popd", &builtin_popd, B_DEFAULT, "[-n] [+N | -N]", help_popd},
#endif
#if BUILTIN_PUSHD
    {"pushd", &builtin_pushd, B_DEFAULT, "[-n] [dir | +N | -N]", help_pushd},
#endif
#if BUILTIN_MV
    {"mv", &builtin_cpmv, B_DEFAULT, "[-finv] source... target", help_mv},
#endif
#if BUILTIN_MKDIR
    {"mkdir", &builtin_mkdir, B_DEFAULT, "[-p]", help_mkdir},
#endif
#if BUILTIN_MKTEMP
    {"mktemp", &builtin_mktemp, B_DEFAULT, "[-dt] [-p DIR] [TEMPLATE]", help_mktemp},
#endif
#if BUILTIN_PRINTF
    {"printf", &builtin_printf, B_DEFAULT, "format [args ...]", help_printf},
#endif
#if BUILTIN_PWD
    {"pwd", &builtin_pwd, B_DEFAULT, "[-L|-P]", help_pwd},
#endif
#if BUILTIN_SET
    {"set", &builtin_set, B_SPECIAL, "[arguments]", help_set},
#endif
#if BUILTIN_DIGEST
    {"sha1sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha224sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha256sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha384sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha512-224sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha512-256sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
    {"sha512sum", &builtin_digest, B_DEFAULT, "[file...]", help_digest},
#endif
#if BUILTIN_SHIFT
    {"shift", &builtin_shift, B_SPECIAL, "[n]", help_shift},
#endif
#if BUILTIN_SLEEP
    {"sleep", &builtin_sleep, B_DEFAULT, "seconds", help_sleep},
#endif
#if BUILTIN_READ
    {"read",
     &builtin_read,
     B_DEFAULT,
     "[-rs] [-d DELIM] [-n|-N NCHARS] [-p PROMPT] [-t TIMEOUT] [-u FD] [name "
     "...]",
     help_read},
#endif
#if BUILTIN_READLINK
    {"readlink", &builtin_readlink, B_DEFAULT, "file...", help_readlink},
#endif
#if BUILTIN_READONLY
    {"readonly", &builtin_readonly, B_SPECIAL, "[-p] [name[=value] ...]", help_readonly},
#endif
#if BUILTIN_REALPATH
    {"realpath", &builtin_realpath, B_DEFAULT, "[-s] file...", help_realpath},
#endif
#if BUILTIN_RETURN
    {"return", &builtin_return, B_SPECIAL, "[n]", help_return},
#endif
#if BUILTIN_RM
    {"rm", &builtin_rm, B_DEFAULT, "[-vrf] [file]...", help_rm},
#endif
#if BUILTIN_RMDIR
    {"rmdir", &builtin_rmdir, B_DEFAULT, "[-p] [directory]...", help_rmdir},
#endif
#if BUILTIN_SED
    {"sed",
     &builtin_sed,
     B_DEFAULT,
     "[-n] [-E|-r] {script | -e script | -f file}... [file]...",
     help_sed,
     &sed_filter},
#endif
#if BUILTIN_SOURCE
    {"source", &builtin_source, B_SPECIAL, "file [arguments]", help_source},
#endif
#if BUILTIN_SORT
    {"sort", &builtin_sort, B_DEFAULT, "[-cmu] [-o output] [-bdfinr] [-t char] [-k keydef]... [file...]", help_sort, &sort_filter},
#endif
#if BUILTIN_SPLIT
    {"split", &builtin_split, B_DEFAULT, "[-l line_count | -b n[k|m]] [-a suffix_length] [file [name]]", help_split},
#endif
#if BUILTIN_TAIL
    {"tail", &builtin_tail, B_DEFAULT, "[-f] [-c number | -n number] [file...]", help_tail, &tail_filter},
#endif
#if BUILTIN_HEAD
    {"head", &builtin_head, B_DEFAULT, "[-n number | -c number] [-qv] [file...]", help_head, &head_filter},
#endif
#if BUILTIN_UNIQ
    {"uniq", &builtin_uniq, B_DEFAULT, "[-c | -d | -u] [-f fields] [-s chars] [input [output]]", help_uniq, &uniq_filter},
#endif
#if BUILTIN_CUT
    {"cut", &builtin_cut, B_DEFAULT, "-b list | -c list | -f list [-d delim] [-s] [file...]", help_cut, &cut_filter},
#endif
#if BUILTIN_NL
    {"nl", &builtin_nl, B_DEFAULT, "[-p] [-b type] [-d delim] [-f type] [-h type] [-i incr] [-l num] [-n format] [-s sep] [-v start] [-w width] [file]", help_nl, &nl_filter},
#endif
#if BUILTIN_TR
    {"tr", &builtin_tr, B_DEFAULT, "[-c|-C] [-s] string1 string2 | [-c|-C] -d [-s] string1 | [-c|-C] -s string1", help_tr, &tr_filter},
#endif
#if BUILTIN_PASTE
    {"paste", &builtin_paste, B_DEFAULT, "[-s] [-d list] file...", help_paste, &paste_filter},
#endif
#if BUILTIN_TEE
    {"tee", &builtin_tee, B_DEFAULT, "[-ai] [file...]", help_tee},
#endif
#if BUILTIN_TEST
    {"test", &builtin_test, B_DEFAULT, "[expr]", help_test},
#endif
#if BUILTIN_TIMES
    {"times", &builtin_times, B_SPECIAL, "", help_times},
#endif
#if BUILTIN_TIMEOUT
    {"timeout",
     &builtin_timeout,
     B_DEFAULT,
     "[-v] [-k DURATION] [-s SIGNAL] DURATION COMMAND [ARG]...",
     help_timeout},
#endif
#if BUILTIN_TOUCH
    {"touch",
     &builtin_touch,
     B_DEFAULT,
     "[-amf] [-d DATE|-t STAMP|-r FILE] [--time=WORD] file...",
     help_touch},
#endif
#if BUILTIN_TRUE
    {"true", &builtin_true, B_DEFAULT, "", help_true},
#endif
#if BUILTIN_TYPE
    {"type", &builtin_type, B_DEFAULT, "[-afptP] name [name ...]", help_type},
#endif
#if BUILTIN_ULIMIT
    {"ulimit", &builtin_ulimit, B_DEFAULT, "[-HSa] [-cdfnstuvlm] [limit]", help_ulimit},
#endif
#if BUILTIN_UMASK
    {"umask", &builtin_umask, B_DEFAULT, "[-p] [-S] [mode]", help_umask},
#endif
#if BUILTIN_UNSET
    {"unset", &builtin_unset, B_SPECIAL, "[name ...]", help_unset},
#endif
#if BUILTIN_TEST
    {"[", &builtin_test, B_DEFAULT, "expr ]", help_test},
#endif
#if BUILTIN_TRAP
    {"trap", &builtin_trap, B_SPECIAL, "[-lp] [[arg] signal_spec ...]", help_trap},
#endif
#if BUILTIN_TYPE
    {"type", &builtin_type, B_DEFAULT, "name ...", help_type},
#endif
#if BUILTIN_ALIAS
    {"unalias", &builtin_unalias, B_DEFAULT, "[-a] [name ...]", help_unalias},
#endif
#if BUILTIN_UNAME
    {"uname", &builtin_uname, B_DEFAULT, "[-amnrspvio]", help_uname},
#endif
#if BUILTIN_WAIT
    {"wait", &builtin_wait, B_DEFAULT, "[pid...]", help_wait},
#endif
#if BUILTIN_WC
    {"wc", &builtin_wc, B_DEFAULT, "[-cmlLw] [file...]", help_wc},
#endif
#if BUILTIN_WHICH
    {"which", &builtin_which, B_DEFAULT, "[-a] filename ...", help_which},
#endif
#if BUILTIN_XARGS
    {"xargs", &builtin_xargs, B_DEFAULT, "[-0opr] [-a FILE] [-d DELIM] [-l/-L MAX-LINES] [-n MAX-ARGS] [-P MAX-PROCS] <command> [...args]", help_xargs},
#endif
#if BUILTIN_COMPRESS
    {"gzip", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"bzip2", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"lbzip2", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"lz", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"lz4", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"lzma", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"lzop", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"xz", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
    {"zstd", &builtin_compress, B_DEFAULT, "[-cdfhk] [-1..-9] [file...]", help_compress, &compress_filter},
#endif
#if BUILTIN_UNCOMPRESS
    {"zcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"bzcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"xzcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"zstdcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"lbzcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"lz4cat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"lzcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"lzopcat", &builtin_uncompress, B_DEFAULT, "[file...]", help_uncompress, &uncompress_filter},
    {"gunzip", &builtin_uncompress, B_DEFAULT, "[-cdfhk] [file...]", help_uncompress, &compress_filter},
    {"unxz", &builtin_uncompress, B_DEFAULT, "[-cdfhk] [file...]", help_uncompress, &compress_filter},
    {"unzstd", &builtin_uncompress, B_DEFAULT, "[-cdfhk] [file...]", help_uncompress, &compress_filter},
#endif
    {NULL, NULL, 0, NULL, NULL},
};
