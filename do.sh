# NOTE: This script is supposed to be able to run in any POSIX compliant shell.

root_dir="$(dirname "$0")" || exit 1
if [ "$root_dir" != "." ]; then
    root_prefix="$root_dir/"
fi
build_dir="${root_prefix}build"

clean=""
build_all=""
no_parallel=""
stop_on_error=""
warnings_are_errors=""
options=""
check_option()
{
    case "$1" in
        "-c"|"--clean")
            clean="1"
            ;;
        "-a"|"--build-all")
            build_all="1"
            ;;
        "-n"|"--no-parallel")
            no_parallel="1"
            ;;
        "-s"|"--stop-on-error")
            stop_on_error="1"
            ;;
        "-w"|"--warnings-are-errors")
            warnings_are_errors="1"
            ;;
        *)
            cat >&2 <<EOF
ERROR: Unrecognized option (\`$1\`)!
Try \`sh  $0  help\`.
EOF
            exit 1
            ;;
    esac
    if [ -n "$options" ]; then
        options="$options "
    fi
    options="$options$1"
}
while [ $# -gt 0 ]; do
    case "$1" in
        "--")
            shift
            break
            ;;
        "--"*)
            check_option "$1"
            shift
            ;;
        "-"?*)
            str="${1#?}"
            while [ -n "$str" ]; do
                rest="${str#?}"
                char="${str%"$rest"}"
                check_option "-$char"
                str="$rest"
            done
            shift
            ;;
        *)
            break
            ;;
    esac
done

action=""
if [ $# -gt 0 ]; then
    action="$1"
    shift
fi

case "$action" in
    "build"|"asan"|"tsan"|"ubsan"|"check"|"check-asan"|"check-tsan"|"check-ubsan"|"check-time"|"check-vg"|"check-gdb"|"check-lldb"|"run"|"run-asan"|"run-tsan"|"run-ubsan"|"run-vg"|"run-time"|"run-gdb"|"run-lldb")
        action="$action-release"
        ;;
esac

case "$action" in
    "build-release"|"build-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "2-")" || exit 1
        ;;
esac

build_demo_progs="NO"
build_test_suite="NO"
case "$action" in
    "run-"*)
        build_demo_progs="YES"
        ;;
    "check-"*)
        build_test_suite="YES"
        ;;
esac
if [ -n "$build_all" ]; then
    build_demo_progs="YES"
    build_test_suite="YES"
fi

need_run_path=""
case "$action" in
    "run-"*)
        action="check-$(printf "%s\n" "$action" | cut -d "-" -f "2-")" || exit 1
        need_run_path="1"
        ;;
esac

run=""
install="NO"
install_sudo="NO"
install_prefix=""
allow_args="NO"
case "$action" in
    "install")
        install="YES"
        if [ "$1" = "--sudo" ]; then
            install_sudo="YES"
            shift
        fi
        if [ $# -gt 0 ]; then
            install_prefix="$1"
            shift
        fi
        ;;
    "check-time-release"|"check-time-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="time"
        allow_args="YES"
        ;;
    "check-vg-release"|"check-vg-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="valgrind"
        allow_args="YES"
        ;;
    "check-gdb-release"|"check-gdb-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="gdb"
        allow_args="YES"
        ;;
    "check-lldb-release"|"check-lldb-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="lldb"
        allow_args="YES"
        ;;
    "check-"*)
        action="$(printf "%s\n" "$action" | cut -d "-" -f "2-")" || exit 1
        run="plain"
        allow_args="YES"
        ;;
    "thorough-check")
        allow_args="YES"
        ;;
esac

build_subdir_name="release"
build_type="Release"
asan="NO"
tsan="NO"
ubsan="NO"
case "$action" in
    "help")
        ;;
    "clean")
        ;;
    "release")
        ;;
    "debug")
        build_subdir_name="debug"
        build_type="Debug"
        ;;
    "install")
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
    "ubsan-release")
        build_subdir_name="ubsan-release"
        ubsan="YES"
        ;;
    "ubsan-debug")
        build_subdir_name="ubsan-debug"
        build_type="Debug"
        ubsan="YES"
        ;;
    "thorough-check")
        ;;
    "")
        cat >&2 <<EOF
ERROR: Unspecified action!
Try \`sh  $0  help\`.
EOF
        exit 1
        ;;
    *)
        cat >&2 <<EOF
ERROR: Unrecognized action (\`$action\`)!
Try \`sh  $0  help\`.
EOF
        exit 1
        ;;
esac

run_path=""
if [ -n "$need_run_path" ]; then
    if [ $# -lt 1 ]; then
        cat >&2 <<EOF
ERROR: Need path to run!
Try \`sh  $0  help\`.
EOF
        exit 1
    fi
    run_path="$1"
    shift
fi

if [ "$allow_args" != "YES" ] && [ $# -gt 0 ]; then
    cat >&2 <<EOF
ERROR: Too many command-line arguments!
Try \`sh  $0  help\`.
EOF
    exit 1
fi

help()
{
    cat >&2 <<EOF
Synopsis:  sh  $0  help
      or:  sh  $0  clean
      or:  sh  $0  release
      or:  sh  $0  debug
      or:  sh  $0  build
      or:  sh  $0  build-release
      or:  sh  $0  build-debug
      or:  sh  $0  install  [--sudo]  [<install prefix>]
      or:  sh  $0  asan
      or:  sh  $0  asan-release
      or:  sh  $0  asan-debug
      or:  sh  $0  tsan
      or:  sh  $0  tsan-release
      or:  sh  $0  tsan-debug
      or:  sh  $0  ubsan
      or:  sh  $0  ubsan-release
      or:  sh  $0  ubsan-debug
      or:  sh  $0  check                [<arg>...]
      or:  sh  $0  check-release        [<arg>...]
      or:  sh  $0  check-debug          [<arg>...]
      or:  sh  $0  check-asan           [<arg>...]
      or:  sh  $0  check-asan-release   [<arg>...]
      or:  sh  $0  check-asan-debug     [<arg>...]
      or:  sh  $0  check-tsan           [<arg>...]
      or:  sh  $0  check-tsan-release   [<arg>...]
      or:  sh  $0  check-tsan-debug     [<arg>...]
      or:  sh  $0  check-ubsan          [<arg>...]
      or:  sh  $0  check-ubsan-release  [<arg>...]
      or:  sh  $0  check-ubsan-debug    [<arg>...]
      or:  sh  $0  check-time           [<arg>...]
      or:  sh  $0  check-time-release   [<arg>...]
      or:  sh  $0  check-time-debug     [<arg>...]
      or:  sh  $0  check-vg             [<arg>...]
      or:  sh  $0  check-vg-release     [<arg>...]
      or:  sh  $0  check-vg-debug       [<arg>...]
      or:  sh  $0  check-gdb            [<arg>...]
      or:  sh  $0  check-gdb-release    [<arg>...]
      or:  sh  $0  check-gdb-debug      [<arg>...]
      or:  sh  $0  check-lldb           [<arg>...]
      or:  sh  $0  check-lldb-release   [<arg>...]
      or:  sh  $0  check-lldb-debug     [<arg>...]
      or:  sh  $0  thorough-check       [<arg>...]
      or:  sh  $0  run                <path>  [<arg>...]
      or:  sh  $0  run-release        <path>  [<arg>...]
      or:  sh  $0  run-debug          <path>  [<arg>...]
      or:  sh  $0  run-asan           <path>  [<arg>...]
      or:  sh  $0  run-asan-release   <path>  [<arg>...]
      or:  sh  $0  run-asan-debug     <path>  [<arg>...]
      or:  sh  $0  run-tsan           <path>  [<arg>...]
      or:  sh  $0  run-tsan-release   <path>  [<arg>...]
      or:  sh  $0  run-tsan-debug     <path>  [<arg>...]
      or:  sh  $0  run-ubsan          <path>  [<arg>...]
      or:  sh  $0  run-ubsan-release  <path>  [<arg>...]
      or:  sh  $0  run-ubsan-debug    <path>  [<arg>...]
      or:  sh  $0  run-time           <path>  [<arg>...]
      or:  sh  $0  run-time-release   <path>  [<arg>...]
      or:  sh  $0  run-time-debug     <path>  [<arg>...]
      or:  sh  $0  run-vg             <path>  [<arg>...]
      or:  sh  $0  run-vg-release     <path>  [<arg>...]
      or:  sh  $0  run-vg-debug       <path>  [<arg>...]
      or:  sh  $0  run-gdb            <path>  [<arg>...]
      or:  sh  $0  run-gdb-release    <path>  [<arg>...]
      or:  sh  $0  run-gdb-debug      <path>  [<arg>...]
      or:  sh  $0  run-lldb           <path>  [<arg>...]
      or:  sh  $0  run-lldb-release   <path>  [<arg>...]
      or:  sh  $0  run-lldb-debug     <path>  [<arg>...]

Options:
    -c, --clean
    -n, --no-parallel
    -s, --stop-on-error
    -w, --warnings-are-errors

To be recognized, these options need to be placed immediately after $0

EOF
}

clean()
{
    echo "Deleting directory '$build_dir'"
    rm -fr "$build_dir" || exit 1
}

banner() {
    awk -v str="$1" -v w="${2:-92}" 'BEGIN {
        up = toupper(str)
        mid = " " up " "
        if (w < length(mid)) w = length(mid)
        rem = w - length(mid)
        lpad = int(rem / 2)
        rpad = rem - lpad
        left = right = ""
        for (i = 1; i <= lpad; i++) left = left "*"
        for (i = 1; i <= rpad; i++) right = right "*"
        border = ""
        for (i = 1; i <= w; i++) border = border "*"
        print border
        print left mid right
        print border
    }'
}

case "$action" in
    "help")
        help
        exit 0
        ;;
    "clean")
        clean
        exit 0
        ;;
    "thorough-check")
        banner "CHECK"
        sh "$0" $options -ca check || exit 1
        banner "CHECK DEBUG"
        sh "$0" $options -ca check-debug || exit 1
        banner "CHECK ASAN DEBUG"
        sh "$0" $options -ca check-asan-debug || exit 1
        banner "CHECK WITH DISABLED PLATFORM OPTIMIZATIONS"
        CXXFLAGS="-DARCHON_DISABLE_PLATFORM_OPTIMIZATIONS" sh "$0" $options -ca check-debug || exit 1
        for x in PNG JPEG Freetype X11 OpenGL SDL3 GLEW; do
            banner "CHECK WITHOUT $x"
            CMAKE_ARGS="-DCMAKE_DISABLE_FIND_PACKAGE_$x=ON" sh "$0" $options -ca check-debug || exit 1
        done
        banner "SUCCESS"
        exit 0
        ;;
esac

if [ -n "$clean" ]; then
    clean
fi

if [ -z "${CMAKE_TOOLCHAIN_FILE:-}" ]; then
    if [ "$OS" = "Windows_NT" ]; then
        if path_1="$(command -v vcpkg 2>/dev/null)"; then
            path_2="$(dirname "$path_1")/scripts/buildsystems/vcpkg.cmake"
            if [ -e "$path_2" ]; then
                export CMAKE_TOOLCHAIN_FILE="$path_2"
            fi
        fi
    fi
fi

parallel_option="-j"
if [ -n "$no_parallel" ]; then
    parallel_option=""
fi

add_cxxflag()
{
    flag="$1"
    if [ -n "$CXXFLAGS" ]; then
        CXXFLAGS="$CXXFLAGS $flag"
    else
        CXXFLAGS="$flag"
    fi
    export CXXFLAGS
}
if [ -n "$stop_on_error" ]; then
    add_cxxflag "-Wfatal-errors"
fi
if [ -n "$warnings_are_errors" ]; then
    add_cxxflag "-Werror"
fi

build_subdir="$build_dir/do/$build_subdir_name"
cmake -S "$root_dir" -B "$build_subdir" -D CMAKE_BUILD_TYPE="$build_type" -D ARCHON_BUILD_DEMO_PROGS="$build_demo_progs" -D ARCHON_BUILD_TEST_SUITE="$build_test_suite" -D ARCHON_ASAN="$asan" -D ARCHON_TSAN="$tsan" -D ARCHON_UBSAN="$ubsan" $CMAKE_ARGS || exit 1
cmake --build "$build_subdir" --config "$build_type" $parallel_option || exit 1

visual_studio_generator=""
if [ -n "$run" ]; then
    if [ -z "$run_path" ]; then
        run_path="src/test"
    fi

    if [ -e "$build_subdir/Archon.sln" ] || [ -e "$build_subdir/Archon.slnx" ]; then
        visual_studio_generator="1"
    fi

    if [ -n "$visual_studio_generator" ]; then
        dirname="$(dirname "$run_path")" || exit 1
        basename="$(basename "$run_path")" || exit 1
        run_path_2="$build_subdir/$dirname/$build_type/$basename.exe"
    else
        run_path_2="$build_subdir/$run_path"
    fi
fi

case "$run" in
    "plain")
        "$run_path_2" "$@" || exit 1
        ;;
    "time")
        time "$run_path_2" "$@" || exit 1
        ;;
    "valgrind")
        valgrind --quiet --track-origins=yes --leak-check=yes --leak-resolution=low --num-callers=24 "$run_path_2" "$@" || exit 1
        ;;
    "gdb")
        gdb --args "$run_path_2" "$@" || exit 1
        ;;
    "lldb")
        lldb "$run_path_2" -- "$@" || exit 1
        ;;
esac

if [ "$install" = "YES" ]; then
    cmake="cmake"
    if [ "$install_sudo" = "YES" ]; then
        cmake="sudo $cmake"
    fi
    if [ -n "$install_prefix" ]; then
        $cmake --install "$build_subdir" --prefix "$install_prefix" || exit 1
    else
        $cmake --install "$build_subdir" || exit 1
    fi
fi
