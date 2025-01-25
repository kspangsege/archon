import sys
import traceback


def run_module_tests(module_name):
    tests = _get_module_tests(module_name)
    run(tests)


def generate_native_tests(module_name):
    tests = _get_module_tests(module_name)
    import unittest
    native_tests = unittest.TestSuite()
    for test in tests:
        native_tests.addTest(_make_native_test(test))
    return native_tests


def run(tests):
    context = _RegularContext()
    failure = False
    for test in tests:
        if test.descr is None:
            print("TEST: %s" % test.name)
        else:
            print("TEST: %s: %s" % (test.name, test.descr))
        try:
            test.func(context)
        except CheckFailure:
            failure = True
        except Exception as exc:
            print("ERROR: Unhandled exception: %s" % type(exc).__name__)
            traceback.print_tb(exc.__traceback__, file = sys.stdout)
            failure = True
    if not failure:
        print("Success")
    else:
        print("FAILURE")


class Context:
    def check(self, cond):
        raise NotImplementedError()

    def check_not(self, cond):
        raise NotImplementedError()

    def check_is_none(self, val):
        raise NotImplementedError()

    def check_is_not_none(self, val):
        raise NotImplementedError()

    def check_equal(self, a, b):
        raise NotImplementedError()

    def check_not_equal(self, a, b):
        raise NotImplementedError()

    def check_in(self, a, b):
        raise NotImplementedError()

    def check_not_in(self, a, b):
        raise NotImplementedError()

    def check_is_instance(self, val, type_):
        raise NotImplementedError()

    def check_not_is_instance(self, val, type_):
        raise NotImplementedError()

    def check_raises(self, exc):
        raise NotImplementedError()


class Test:
    def __init__(self, name, descr, func):
        self.name = name
        self.descr = descr
        self.func = func


class CheckFailure(Exception):
    pass



class _RegularContext(Context):
    def __init__(self):
        self._debug_on_failure = False

    def check(self, cond):
        self._check(cond, "check(%r) failed", cond)

    def check_not(self, cond):
        self._check(not cond, "check_not(%r) failed", cond)

    def check_is_none(self, val):
        self._check(val is None, "check_is_none(%r) failed", val)

    def check_is_not_none(self, val):
        self._check(val is not None, "check_is_not_none(%r) failed", val)

    def check_equal(self, a, b):
        self._check(a == b, "check_equal(%r, %r) failed", a, b)

    def check_not_equal(self, a, b):
        self._check(a != b, "check_not_equal(%r, %r) failed", a, b)

    def check_in(self, a, b):
        self._check(a in b, "check_in(%r, %r) failed", a, b)

    def check_not_in(self, a, b):
        self._check(a not in b, "check_not_in(%r, %r) failed", a, b)

    def check_is_instance(self, val, type_):
        self._check(isinstance(val, type_), "check_is_instance(%r, %s) failed: Type was %s", val, type_.__name__,
                    type(val).__name__)

    def check_not_is_instance(self, val, type_):
        self._check(not isinstance(val, type_), "check_not_is_instance(%r, %s) failed", val, type_.__name__)

    def check_raises(self, exc_type):
        return _CheckRaises(self, exc_type)

    def _check(self, cond, message, *params):
        if cond:
            return
        clip = 3
        self._fail(message % params, clip)

    def _fail(self, message, clip, exc_tb = None):
        print("ERROR: %s" % message)
        if self._debug_on_failure:
            breakpoint()
        entries = traceback.extract_stack()
        for entry in traceback.format_list(entries[:-clip]):
            sys.stdout.write(entry)
        if exc_tb:
            print("Unexpected exception:")
            traceback.print_tb(exc_tb, file = sys.stdout)
        raise CheckFailure()


class _CheckRaises:
    def __init__(self, context, exc_type):
        self._context = context
        self._exc_type = exc_type

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc, tb):
        if isinstance(exc, self._exc_type):
            return True
        submessage = "No exception was raised"
        if exc is not None:
            submessage = "A different exception was raised (%s)" % exc_type.__name__
        message = "check_raises(%s) failed: %s" % (self._exc_type.__name__, submessage)
        clip = 2
        self._context._fail(message, clip, tb)


def _get_module_tests(module_name):
    tests = []
    prefix = "test_"
    for name, func in sys.modules[module_name].__dict__.items():
        if not name.startswith(prefix):
            continue
        test_name = name[len(prefix):]
        descr = func.__doc__
        tests.append(Test(test_name, descr, func))
    return tests


def _make_native_test(test):
    import unittest
    class Test(unittest.TestCase):
        pass
    method_name = "test%s" % test.name
    def method_func(self):
        context = _ContextBridge(self)
        test.func(context)
    method_func.__doc__ = test.descr
    setattr(Test, method_name, method_func)
    return Test(methodName = method_name)


class _ContextBridge(Context):
    def __init__(self, test_case):
        self._test_case = test_case

    def check(self, cond):
        self._test_case.assertTrue(cond)

    def check_not(self, cond):
        self._test_case.assertFalse(cond)

    def check_is_none(self, val):
        self._test_case.assertIsNone(val)

    def check_is_not_none(self, val):
        self._test_case.assertIsNotNone(val)

    def check_equal(self, a, b):
        self._test_case.assertEqual(a, b)

    def check_not_equal(self, a, b):
        self._test_case.assertNotEqual(a, b)

    def check_in(self, a, b):
        self._test_case.assertIn(a, b)

    def check_not_in(self, a, b):
        self._test_case.assertNotIn(a, b)

    def check_is_instance(self, val, type_):
        self._test_case.assertIsInstance(val, type_)

    def check_not_is_instance(self, val, type_):
        self._test_case.assertNotIsInstance(val, type_)

    def check_raises(self, exc):
        return self._test_case.assertRaises(exc)
