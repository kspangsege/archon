from __future__ import annotations

import typing
import types
import inspect
import abc
import collections
import dataclasses
import traceback
import pathlib
import sys
import secrets
import random
import unittest
import bdb
import pdb

import archon.base as _b
import archon.log as _l
import archon.command_line_interface as _cli


def run_module_tests(module_name: str) -> None:
    def int_16(string: str) -> int:
        return int(string, 16)

    random_seed: int | None = None
    def set_random_seed(seed: int) -> None:
        nonlocal random_seed
        random_seed = seed

    help_              = _b.Wrap(False)
    debug_on_failure   = _b.Wrap(False)
    testcase_log_level = _b.Wrap(_l.LogLevel.OFF)

    spec = _cli.Spec()
    spec.opt(["--"], _cli.Stop())
    spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
    spec.opt(["-s", "--random-seed"], _cli.CallWithArg(int_16, set_random_seed))
    spec.opt(["-d", "--debug-on-failure"], _cli.Raise(debug_on_failure))
    spec.opt(["-l", "--testcase-log-level"], _cli.AssignWithArg(_l.parse_log_level, testcase_log_level))

    logger = _l.RootLogger()
    success, args = _cli.parse(sys.argv[1:], spec, logger)
    if not success:
        sys.exit(1)
    if help_.value:
        _cli.show_help(spec, logger)
        return
    if len(args) > 0:
        logger.error("Too many command-line arguments (try --help)")
        sys.exit(1)

    if random_seed is None:
        random_seed = secrets.randbits(384)  # 19968 for a fill seeding

    logger.info("Random seed: %s", hex(random_seed))
    tests = _get_module_tests(module_name)
    testcase_logger_1 = _l.PrefixLogger(logger, "Testcase: ")
    testcase_logger_2 = _l.LimitLogger(testcase_logger_1, testcase_log_level.value)
    run(tests, random_seed, debug_on_failure.value, logger, testcase_logger_2)


def generate_native_tests(module_name: str) -> unittest.TestSuite:
    logger = _l.RootLogger()
    tests = _get_module_tests(module_name)
    native_tests = unittest.TestSuite()
    for test in tests:
        native_tests.addTest(_make_native_test(test, logger))
    return native_tests


def run(tests: collections.abc.Iterable[Test], random_seed: int, debug_on_failure: bool, logger: _l.Logger,
        testcase_logger: _l.Logger) -> None:
    base_stack_depth = len(traceback.extract_stack())
    context = _RegularContext(base_stack_depth, random_seed, debug_on_failure, logger, testcase_logger)
    failure = False
    for test in tests:
        if test.description is None:
            logger.info("TEST: %s", test.qualified_name)
        else:
            logger.info("TEST: %s: %s", test.qualified_name, test.description)
        try:
            test.func(context)
        except CheckFailure:
            failure = True
        except (pdb.Restart, bdb.BdbQuit):
            raise
        except Exception as e:
            name = type(e).__name__
            message = str(e)
            signature = "%s: %s" % (name, message) if message else name
            tb = e.__traceback__.tb_next if e.__traceback__ else None
            stacktrace = "".join(traceback.format_tb(tb))
            string = "%s\n%s" % (signature, _b.chomp(stacktrace)) if stacktrace else signature
            logger.error("Unhandled exception: %s", string)
            failure = True
    if not failure:
        logger.info("Success")
    else:
        logger.info("FAILURE")


class Context(abc.ABC):
    @abc.abstractmethod
    def check(self, cond: bool) -> None:
        ...

    @abc.abstractmethod
    def check_not(self, cond: bool) -> None:
        ...

    @abc.abstractmethod
    def check_is_none(self, value: object) -> None:
        ...

    @abc.abstractmethod
    def check_is_not_none(self, value: object) -> None:
        ...

    @abc.abstractmethod
    def check_equal[T](self, a: T, b: T) -> None:
        ...

    @abc.abstractmethod
    def check_not_equal[T](self, a: T, b: T) -> None:
        ...

    @abc.abstractmethod
    def check_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        ...

    @abc.abstractmethod
    def check_not_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        ...

    @abc.abstractmethod
    def check_is_instance(self, value: object, type_info: TypeInfo) -> None:
        ...

    @abc.abstractmethod
    def check_not_is_instance(self, value: object, type_info: TypeInfo) -> None:
        ...

    @abc.abstractmethod
    def check_raises(self, exception_type_info: TypeInfo) -> typing.ContextManager[None]:
        ...

    @abc.abstractmethod
    def create_rng(self) -> random.Random:
        ...

    @property
    def logger(self) -> _l.Logger:
        return self._testcase_logger

    def __init__(self, testcase_logger: _l.Logger) -> None:
        self._testcase_logger = testcase_logger


type TypeInfo = type | types.UnionType | tuple[TypeInfo, ...]


@dataclasses.dataclass(slots=True, frozen=True)
class Test:
    name:           str
    qualified_name: str
    description:    str | None
    func:           collections.abc.Callable[[Context], None]


class CheckFailure(Exception):
    pass


class BadTestFunctionSignature(Exception):
    pass








class _RegularContext(Context):
    def __init__(self, base_stack_depth: int, random_seed: int, debug_on_failure: bool, logger: _l.Logger,
                 testcase_logger: _l.Logger) -> None:
        Context.__init__(self, testcase_logger)
        self._base_stack_depth = base_stack_depth
        self._random_seed = random_seed
        self._debug_on_failure = debug_on_failure
        self._logger = logger

    @typing.override
    def check(self, cond: bool) -> None:
        self._check(cond, "check", "(%r) failed", cond)

    @typing.override
    def check_not(self, cond: bool) -> None:
        self._check(not cond, "check_not", "(%r) failed", cond)

    @typing.override
    def check_is_none(self, value: object) -> None:
        cond = value is None
        self._check(cond, "check_is_none", "(%r) failed", value)

    @typing.override
    def check_is_not_none(self, value: object) -> None:
        cond = value is not None
        self._check(cond, "check_is_not_none", "(%r) failed", value)

    @typing.override
    def check_equal[T](self, a: T, b: T) -> None:
        cond = a == b
        self._check(cond, "check_equal", "(%r, %r) failed", a, b)

    @typing.override
    def check_not_equal[T](self, a: T, b: T) -> None:
        cond = a != b
        self._check(cond, "check_not_equal", "(%r, %r) failed", a, b)

    @typing.override
    def check_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        cond = a in b
        self._check(cond, "check_in", "(%r, %r) failed", a, b)

    @typing.override
    def check_not_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        cond = a not in b
        self._check(cond, "check_not_in", "(%r, %r) failed", a, b)

    @typing.override
    def check_is_instance(self, value: object, type_info: TypeInfo) -> None:
        cond = isinstance(value, type_info)
        self._check(cond, "check_is_instance", "(%r, %s) failed: Type was %s", value, _format_type_info(type_info),
                    type(value).__name__)

    @typing.override
    def check_not_is_instance(self, value: object, type_info: TypeInfo) -> None:
        cond = not isinstance(value, type_info)
        self._check(cond, "check_not_is_instance", "(%r, %s) failed: Type was %s", value, _format_type_info(type_info),
                    type(value).__name__)

    @typing.override
    def check_raises(self, exception_type_info: TypeInfo) -> typing.ContextManager[None]:
        return _CheckRaises(self, exception_type_info)

    @typing.override
    def create_rng(self) -> random.Random:
        return random.Random(self._random_seed)

    def _check(self, cond: bool, check_name: str, message: str, *params: typing.Any) -> None:
        if cond:
            return
        message_2 = check_name + (message % params)
        clip = 3
        check_frame_name = check_name
        self._fail(message_2, clip, check_frame_name)

    def _fail(self, message: str, clip: int, check_frame_name: str,
              exception_tb: types.TracebackType | None = None) -> typing.Never:
        entries = traceback.extract_stack()
        if self._base_stack_depth > 0 and len(entries) >= self._base_stack_depth:
            entry = entries[self._base_stack_depth - 1]
            assert entry.name == "run" and entry.filename == __file__
        assert clip > 0
        if len(entries) > self._base_stack_depth + clip:
            entry = entries[-clip]
            assert entry.name == check_frame_name and entry.filename == __file__
        stacktrace = "".join(traceback.format_list(entries[self._base_stack_depth:-clip]))
        text = "%s\n%s" % (message, _b.chomp(stacktrace)) if stacktrace else message
        if exception_tb:
            stacktrace_2 = "".join(traceback.format_tb(exception_tb))
            if stacktrace_2:
                text = "%s\nUnexpected exception traceback:\n%s" % (text, _b.chomp(stacktrace_2))
        self._logger.error("%s", text)
        if self._debug_on_failure:
            breakpoint()
        raise CheckFailure


class _CheckRaises:
    def __init__(self, context: _RegularContext, exception_type_info: TypeInfo) -> None:
        self._context             = context
        self._exception_type_info = exception_type_info

    def __enter__(self) -> None:
        pass

    def __exit__(self, exc_type: type[BaseException] | None, exc_val: BaseException | None,
                 tb: types.TracebackType | None) -> bool | None:
        if isinstance(exc_val, self._exception_type_info):
            return True
        submessage = "No exception was raised"
        if exc_val is not None:
            assert exc_type
            name = exc_type.__name__
            message = str(exc_val)
            signature = "%s: %s" % (name, message) if message else name
            submessage = "A different exception was raised: %s" % signature
        message = "check_raises(%s) failed: %s" % (_format_type_info(self._exception_type_info), submessage)
        clip = 2
        check_frame_name = "__exit__"
        self._context._fail(message, clip, check_frame_name, tb)


def _format_type_info(type_info: TypeInfo) -> str:
    match type_info:
        case type():
            return type_info.__name__
        case types.UnionType():
            return _format_type_info(type_info.__args__)
        case tuple():
            return "(%s)" % " | ".join(_format_type_info(t) for t in type_info)
    typing.assert_never(type_info)


def _get_module_tests(module_name: str) -> list[Test]:
    tests = []
    prefix = "test_"
    module = sys.modules[module_name]
    for name, obj in module.__dict__.items():
        if not name.startswith(prefix) or not inspect.isfunction(obj) or obj.__module__ != module_name:
            continue
        func = obj
        sig = inspect.signature(func)
        params = list(sig.parameters.values())
        if len(params) != 1:
            raise BadTestFunctionSignature("Wrong number of arguments") from None
        param_name = params[0].name
        try:
            hints = typing.get_type_hints(func, globalns=module.__dict__)
        except NameError as e:
            raise BadTestFunctionSignature("Failed to determine type hint for `%s` parameter of `%s`: %s" %
                                           (param_name, name, e)) from None
        if hints.get(param_name) is not Context:
            raise BadTestFunctionSignature("Wrong type hint for `%s` parameter of `%s` (must be "
                                           "`archon.test.Context`)" % (param_name, name)) from None
        if hints.get("return") is not type(None):
            raise BadTestFunctionSignature("Wrong return type hint for `%s` (must be `None`)" % name) from None
        assert module.__file__ is not None
        proper_module_name = pathlib.Path(module.__file__).stem if module_name == "__main__" else module_name
        test_name = name[len(prefix):]
        qualified_name = "%s.%s" % (proper_module_name, test_name)
        description = func.__doc__
        tests.append(Test(test_name, qualified_name, description, func))
    return tests


def _make_native_test(test: Test, logger: _l.Logger) -> unittest.TestCase:
    class_name = "Tests"
    base_classes = (unittest.TestCase,)
    method_name = "test_%s" % test.name
    def method_func(self: unittest.TestCase) -> None:
        context = _ContextBridge(self, logger)
        test.func(context)
    method_func.__doc__ = test.description
    attributes = {
        "__module__": test.func.__module__,
        method_name:  method_func,
    }
    test_case_class = type(class_name, base_classes, attributes)
    return typing.cast(unittest.TestCase, test_case_class(methodName=method_name))


class _ContextBridge(Context):
    def __init__(self, test_case: unittest.TestCase, logger: _l.Logger) -> None:
        Context.__init__(self, logger)
        self._test_case = test_case

    @typing.override
    def check(self, cond: bool) -> None:
        self._test_case.assertTrue(cond)

    @typing.override
    def check_not(self, cond: bool) -> None:
        self._test_case.assertFalse(cond)

    @typing.override
    def check_is_none(self, value: object) -> None:
        self._test_case.assertIsNone(value)

    @typing.override
    def check_is_not_none(self, value: object) -> None:
        self._test_case.assertIsNotNone(value)

    @typing.override
    def check_equal[T](self, a: T, b: T) -> None:
        self._test_case.assertEqual(a, b)

    @typing.override
    def check_not_equal[T](self, a: T, b: T) -> None:
        self._test_case.assertNotEqual(a, b)

    @typing.override
    def check_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        self._test_case.assertIn(a, b)

    @typing.override
    def check_not_in[T](self, a: T, b: collections.abc.Container[T]) -> None:
        self._test_case.assertNotIn(a, b)

    @typing.override
    def check_is_instance(self, value: object, type_info: TypeInfo) -> None:
        self._test_case.assertIsInstance(value, type_info)

    @typing.override
    def check_not_is_instance(self, value: object, type_info: TypeInfo) -> None:
        self._test_case.assertNotIsInstance(value, type_info)

    @typing.override
    def check_raises(self, exception_type_info: TypeInfo) -> typing.ContextManager[None]:
        compat_type_info = _type_info_compat_cast(exception_type_info)
        native_cm: typing.ContextManager[typing.Any] = self._test_case.assertRaises(compat_type_info)
        return _NativeContextManagerAdaptor(native_cm)

    @typing.override
    def create_rng(self) -> random.Random:
        return random.Random()


def _type_info_compat_cast(type_info: TypeInfo) -> tuple[type, ...]:
    match type_info:
        case type():
            return (type_info,)
        case types.UnionType():
            return tuple(u for t in type_info.__args__ for u in _type_info_compat_cast(t))
        case tuple():
            return tuple(u for t in type_info for u in _type_info_compat_cast(t))
    typing.assert_never(type_info)


class _NativeContextManagerAdaptor:
    def __init__(self, native_cm: typing.ContextManager[typing.Any]) -> None:
        self._native_cm = native_cm

    def __enter__(self) -> None:
        self._native_cm.__enter__()
        return None

    def __exit__(self, exc_type: type[BaseException] | None, exc_val: BaseException | None,
                 tb: types.TracebackType | None) -> bool | None:
        return self._native_cm.__exit__(exc_type, exc_val, tb)
