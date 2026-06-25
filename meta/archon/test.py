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
import unittest

import archon.log as _l


def run_module_tests(module_name: str) -> None:
    logger = _l.RootLogger()
    tests = _get_module_tests(module_name)
    run(tests, logger)


def generate_native_tests(module_name: str) -> unittest.TestSuite:
    logger = _l.RootLogger()
    tests = _get_module_tests(module_name)
    native_tests = unittest.TestSuite()
    for test in tests:
        native_tests.addTest(_make_native_test(test, logger))
    return native_tests


def run(tests: collections.abc.Iterable, logger: _l.Logger) -> None:
    context = _RegularContext(logger)
    failure = False
    for test in tests:
        if test.description is None:
            print("TEST: %s" % test.qualified_name)
        else:
            print("TEST: %s: %s" % (test.qualified_name, test.description))
        try:
            test.func(context)
        except CheckFailure:
            failure = True
        except Exception as e:
            print("ERROR: Unhandled exception: %s" % type(e).__name__)
            traceback.print_tb(e.__traceback__, file=sys.stdout)
            failure = True
    if not failure:
        print("Success")
    else:
        print("FAILURE")


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
    def check_equal(self, a: object, b: object) -> None:
        ...

    @abc.abstractmethod
    def check_not_equal(self, a: object, b: object) -> None:
        ...

    @abc.abstractmethod
    def check_in(self, a: object, b: collections.abc.Container) -> None:
        ...

    @abc.abstractmethod
    def check_not_in(self, a: object, b: collections.abc.Container) -> None:
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

    @property
    def logger(self) -> _l.Logger:
        return self._logger

    def __init__(self, logger: _l.Logger) -> None:
        self._logger = logger


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
    def __init__(self, logger: _l.Logger) -> None:
        Context.__init__(self, logger)
        self._debug_on_failure = False

    @typing.override
    def check(self, cond: bool) -> None:
        self._check(cond, "check(%r) failed", cond)

    @typing.override
    def check_not(self, cond: bool) -> None:
        self._check(not cond, "check_not(%r) failed", cond)

    @typing.override
    def check_is_none(self, value: object) -> None:
        self._check(value is None, "check_is_none(%r) failed", value)

    @typing.override
    def check_is_not_none(self, value: object) -> None:
        self._check(value is not None, "check_is_not_none(%r) failed", value)

    @typing.override
    def check_equal(self, a: object, b: object) -> None:
        self._check(a == b, "check_equal(%r, %r) failed", a, b)

    @typing.override
    def check_not_equal(self, a: object, b: object) -> None:
        self._check(a != b, "check_not_equal(%r, %r) failed", a, b)

    @typing.override
    def check_in(self, a: object, b: collections.abc.Container) -> None:
        self._check(a in b, "check_in(%r, %r) failed", a, b)

    @typing.override
    def check_not_in(self, a: object, b: collections.abc.Container) -> None:
        self._check(a not in b, "check_not_in(%r, %r) failed", a, b)

    @typing.override
    def check_is_instance(self, value: object, type_info: TypeInfo) -> None:
        self._check(isinstance(value, type_info), "check_is_instance(%r, %s) failed: Type was %s", value,
                    _format_type_info(type_info), type(value).__name__)

    @typing.override
    def check_not_is_instance(self, value: object, type_info: TypeInfo) -> None:
        self._check(not isinstance(value, type_info), "check_not_is_instance(%r, %s) failed: Type was %s", value,
                    _format_type_info(type_info), type(value).__name__)

    @typing.override
    def check_raises(self, exception_type_info: TypeInfo) -> typing.ContextManager[None]:
        return _CheckRaises(self, exception_type_info)

    def _check(self, cond: bool, message: str, *params: typing.Any) -> None:
        if cond:
            return
        clip = 3
        self._fail(message % params, clip)

    def _fail(self, message: str, clip: int, exception_tb: types.TracebackType | None = None) -> typing.Never:
        print("ERROR: %s" % message)
        if self._debug_on_failure:
            breakpoint()
        entries = traceback.extract_stack()
        for entry in traceback.format_list(entries[:-clip]):
            sys.stdout.write(entry)
        if exception_tb:
            print("Unexpected exception:")
            traceback.print_tb(exception_tb, file = sys.stdout)
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
            submessage = "A different exception was raised (%s)" % exc_type.__name__
        message = "check_raises(%s) failed: %s" % (_format_type_info(self._exception_type_info), submessage)
        clip = 2
        self._context._fail(message, clip, tb)


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
    def method_func(self) -> None:
        context = _ContextBridge(self, logger)
        test.func(context)
    method_func.__doc__ = test.description
    attributes = {
        "__module__": test.func.__module__,
        method_name:  method_func,
    }
    test_case_class = type(class_name, base_classes, attributes)
    return test_case_class(methodName=method_name)


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
    def check_equal(self, a: object, b: object) -> None:
        self._test_case.assertEqual(a, b)

    @typing.override
    def check_not_equal(self, a: object, b: object) -> None:
        self._test_case.assertNotEqual(a, b)

    @typing.override
    def check_in(self, a: object, b: collections.abc.Container) -> None:
        self._test_case.assertIn(a, b)

    @typing.override
    def check_not_in(self, a: object, b: collections.abc.Container) -> None:
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
