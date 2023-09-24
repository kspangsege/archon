# NOTE: This script is supposed to be able to run in any POSIX compliant shell.

root_dir="$(dirname "$0")" || exit 1
if [ "$root_dir" != "." ]; then
    root_prefix="$root_dir/"
fi
build_dir="${root_prefix}build"

no_parallel=""
fatal_errors=""
warnings_are_errors=""
while [ $# -gt 0 ]; do
    case "$1" in
        "-"*)
            case "$1" in
                "-n"|"--no-parallel")
                    no_parallel="1"
                    ;;
                "-s"|"--stop-on-error")
                    stop_on_error="1"
                    ;;
                "-e"|"--warnings-are-errors")
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
            shift
            ;;
        *)
            break
            ;;
    esac
done

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

case "$action" in
    "run-"*)
        action="check-$(printf "%s\n" "$action" | cut -d "-" -f "2-")" || exit 1
        need_run_path="1"
        ;;
esac

install="NO"
install_sudo="NO"
install_prefix=""
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
        ;;
    "check-vg-release"|"check-vg-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="valgrind"
        ;;
    "check-gdb-release"|"check-gdb-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="gdb"
        ;;
    "check-lldb-release"|"check-lldb-debug")
        action="$(printf "%s\n" "$action" | cut -d "-" -f "3-")" || exit 1
        run="lldb"
        ;;
    "check-"*)
        action="$(printf "%s\n" "$action" | cut -d "-" -f "2-")" || exit 1
        run="plain"
        ;;
esac

build_subdir_name="release"
build_type="Release"
asan="NO"
tsan="NO"
ubsan="NO"

case "$action" in
    "help")
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
      or:  sh  $0  check                [<test harness arg>...]
      or:  sh  $0  check-release        [<test harness arg>...]
      or:  sh  $0  check-debug          [<test harness arg>...]
      or:  sh  $0  check-asan           [<test harness arg>...]
      or:  sh  $0  check-asan-release   [<test harness arg>...]
      or:  sh  $0  check-asan-debug     [<test harness arg>...]
      or:  sh  $0  check-tsan           [<test harness arg>...]
      or:  sh  $0  check-tsan-release   [<test harness arg>...]
      or:  sh  $0  check-tsan-debug     [<test harness arg>...]
      or:  sh  $0  check-ubsan          [<test harness arg>...]
      or:  sh  $0  check-ubsan-release  [<test harness arg>...]
      or:  sh  $0  check-ubsan-debug    [<test harness arg>...]
      or:  sh  $0  check-time           [<test harness arg>...]
      or:  sh  $0  check-time-release   [<test harness arg>...]
      or:  sh  $0  check-time-debug     [<test harness arg>...]
      or:  sh  $0  check-vg             [<test harness arg>...]
      or:  sh  $0  check-vg-release     [<test harness arg>...]
      or:  sh  $0  check-vg-debug       [<test harness arg>...]
      or:  sh  $0  check-gdb            [<test harness arg>...]
      or:  sh  $0  check-gdb-release    [<test harness arg>...]
      or:  sh  $0  check-gdb-debug      [<test harness arg>...]
      or:  sh  $0  check-lldb           [<test harness arg>...]
      or:  sh  $0  check-lldb-release   [<test harness arg>...]
      or:  sh  $0  check-lldb-debug     [<test harness arg>...]
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
EOF
        exit 0
        ;;
    "clean")
        echo "Deleting directory '$build_dir'"
        rm -fr "$build_dir" || exit 1
        exit 0
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

if [ "$need_run_path" ]; then
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

if [ -z "$run" -a $# -gt 0 ]; then
    cat >&2 <<EOF
ERROR: Too many command-line arguments!
Try \`sh  $0  help\`.
EOF
    exit 1
fi

if [ -z "$CMAKE_TOOLCHAIN_FILE" ]; then
    if [ "$OS" = "Windows_NT" ]; then
        # Asuming that Vcpkg is installed, and that it is installed under c:/src/vcpkg
        export CMAKE_TOOLCHAIN_FILE="c:/src/vcpkg/scripts/buildsystems/vcpkg.cmake"
    fi
fi

parallel_option="-j"
if [ "$no_parallel" ]; then
    parallel_option=""
fi

add_cxxflag()
{
    local flag
    flag="$1"
    if [ "$CXXFLAGS" ]; then
        CXXFLAGS="$CXXFLAGS $flag"
    else
        CXXFLAGS="$flag"
    fi
    export CXXFLAGS
}
if [ "$stop_on_error" ]; then
    add_cxxflag "-Wfatal-errors"
fi
if [ "$warnings_are_errors" ]; then
    add_cxxflag "-Werror"
fi

build_subdir="$build_dir/$build_subdir_name"
cmake -S "$root_dir" -B "$build_subdir" -D CMAKE_BUILD_TYPE="$build_type" -D ARCHON_ASAN="$asan" -D ARCHON_TSAN="$tsan" -D ARCHON_UBSAN="$ubsan" || exit 1
cmake --build "$build_subdir" --config "$build_type" $parallel_option || exit 1

if [ "$run" ]; then
    if [ -z "$run_path" ]; then
        run_path="src/test"
    fi

    if [ "$OS" = "Windows_NT" ]; then
        # Assuming Visual Studio generator here
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
    if [ "$install_prefix" ]; then
        $cmake --install "$build_subdir" --prefix "$install_prefix" || exit 1
    else
        $cmake --install "$build_subdir" || exit 1
    fi
fi
