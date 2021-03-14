IFS="
"
exec_cmd() (
	CMD=$(
		IFS=" "
		echo "$*"
	)
	[ "$DEBUG" = true ] && echo "$CMD" 1>&2
	command "$@"
)

find_cmake_files() (
	set -f
	set -- ${@:-.}
	if [ "$TYPE" ]; then
		case "$TYPE" in
		'^'* | '!'*) set -- "$@" -not -type "${TYPE#?}" ;;
		*) set -- "$@" -type "$TYPE" ;;
		esac
	fi
	set -- "$@" "(" -iname CMakeLists.txt \
		-or -iname "*.cmake" \
		")"
	set -- "$@" -and "(" \
		-not -wholename "*build/*" \
		-or -wholename "*build/cmake/*" \
		")"
	if [ "$NOT" ]; then
		for PATTERN in $NOT; do
			set -- "$@" -and -not -iwholename "$PATTERN"
		done
	fi
	[ "$DEBUG" = true ] && set -x
	exec find "$@"
)

append() {
	eval "shift; $1=\${$1:+\${$1}\$IFS}\$*"
}

find_cmake() (
	DO_FMT=false
	NO_3RDPARTY=true
	REVERSE=
	TYPE='^d'
	while [ $# -gt 0 ]; do
		case $1 in
		-f | --fmt) DO_FMT=true && shift ;;
		-x | --debug) DEBUG=true && shift ;;
		-3 | --3rdparty) NO_3RDPARTY=false && shift ;;
		-t | --time) BY_TIME=true && shift ;;
		-r | --reverse) REVERSE=r && shift ;;
		-s | --sort) SORT=true && shift ;;
		-H | --no-header*) NO_HEADERS=true && shift ;;
		*) break ;;
		esac
	done
	CMD='echo "$FILE"'
	LIST_CMD='find_cmake_files'
	: ${CMAKE_FORMAT:=".cmake-format"}
	if [ -f "$CMAKE_FORMAT" ]; then
		CMAKE_FORMAT_CONFIG=$(realpath --relative-to "$PWD" "$CMAKE_FORMAT")
	fi
	if [ "$SORT" = true ]; then
		LIST_CMD=$LIST_CMD' | sort -f -V ${REVERSE:+-r}'
	elif [ "$BY_TIME" = true ]; then
		LIST_CMD='ls -td${REVERSE} -- $('$LIST_CMD')'
	fi
	if [ "$DO_FMT" = true ]; then
		NO_HEADERS=true
		NO_3RDPARTY=true
		TYPE=f
		CMD='exec_cmd cmake-format ${CMAKE_FORMAT_CONFIG:+-c} ${CMAKE_FORMAT_CONFIG:+"$CMAKE_FORMAT_CONFIG"} -i "$FILE" || return $?'
	fi
	if [ "$NO_3RDPARTY" = true ]; then
		append NOT "*3rdparty/*"
	fi
	if [ "$NO_HEADERS" = true ]; then
		append NOT "*.h.*"
	fi
	CMD='for FILE in $('$LIST_CMD'); do FILE=${FILE#./}; '$CMD'; done'

	eval "$CMD"
)
find_cmake "$@"
