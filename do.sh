# NOTE: THIS SCRIPT IS SUPPOSED TO RUN IN A POSIX SHELL

root_dir="$(dirname "$0")" || exit 1
if [ "$root_dir" != "." ]; then
    root_prefix="$root_dir/"
fi

if [ $# -gt 0 ]; then
    mode="$1"
    shift
fi

build_dir="${root_prefix}build"
if [ "$mode" = "clean" ]; then
    echo "Deleting directory '$build_dir'"
    rm -fr "$build_dir" || exit 1
    if [ $# -eq 0 ]; then
        exit 0
    fi
    mode="$1"
    shift
fi

case "$mode" in
    "build"|"asan"|"tsan")
        mode="$mode-release"
        ;;
esac

case "$mode" in
    "build-"*)
        mode="$(printf "%s\n" "$mode" | cut -d "-" -f "2-")" || exit 1
        ;;
esac

build_subdir_name="release"
build_type="Release"
asan="NO"
tsan="NO"

case "$mode" in
    "release")
        ;;
    "debug")
        build_subdir_name="debug"
        build_type="Debug"
        ;;
    "asan-release")
        build_subdir_name="asan-release"
        asan="YES"
        ;;
    "asan-debug")
        build_subdir_name="asan-debug"
        build_type="Debug"
        asan="YES"
        ;;
    "tsan-release")
        build_subdir_name="tsan-release"
        tsan="YES"
        ;;
    "tsan-debug")
        build_subdir_name="tsan-debug"
        build_type="Debug"
        tsan="YES"
        ;;
    *)
        cat >&2 <<EOF
ERROR: Bad or missing mode
Must be one of:
    clean
    release
    debug
    build
    build-release
    build-debug
    asan
    asan-release
    asan-debug
    tsan
    tsan-release
    tsan-debug
EOF
        exit 1
        ;;
esac

if [ $# -gt 0 ]; then
    cat >&2 <<EOF
ERROR: Too many command-line arguments

Synopsis: $0  [<mode>]
EOF
    exit 1
fi

if [ -z "$CMAKE_BUILD_PARALLEL_LEVEL" ]; then
    if [ "$OS" = "Darwin" ]; then
        n="$(sysctl -n hw.ncpu)" || exit 1
    elif [ -r "/proc/cpuinfo" ]; then
        n="$(cat /proc/cpuinfo | grep -E 'processor[[:space:]]*:' | wc -l)" || exit 1
    fi
    if [ "$n" ]; then
        export CMAKE_BUILD_PARALLEL_LEVEL="$NUM_PROCESSORS"
    fi
fi

build_subdir="$build_dir/$build_subdir_name"
cmake -S "$root_dir" -B "$build_subdir" -D CMAKE_BUILD_TYPE="$build_type" -D ARCHON_ASAN="$asan" -D ARCHON_TSAN="$tsan" || exit 1
cmake --build "$build_subdir" || exit 1
