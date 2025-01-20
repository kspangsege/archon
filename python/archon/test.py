import sys


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
    for test in tests:
        if test.descr is None:
            print("%s" % test.name)
        else:
            print("%s: %s" % (test.name, test.descr))
        test.func(context)
    if not context._failure:
        print("Success")
    else:
        print("FAILURE")


class Context:
    def check_equal(self, a, b):
        raise RuntimeError("Abstract function")

    def check(self, cond):
        raise RuntimeError("Abstract function")

    def check_not(self, cond):
        raise RuntimeError("Abstract function")


class Test:
    def __init__(self, name, descr, func):
        self.name = name
        self.descr = descr
        self.func = func


class _RegularContext(Context):
    def __init__(self):
        self._debug_on_failure = True
        self._failure = False

    def check_equal(self, a, b):
        return self._check(a == b, "check_equal(%s, %s) failed", a, b)

    def check(self, cond):
        return self._check(cond, "check(%s) failed", cond)

    def check_not(self, cond):
        return self._check(not cond, "check_not(%s) failed", cond)

    def _check(self, cond, message, *params):
        if cond:
            return True
        self._fail(message % params)
        if self._debug_on_failure:
            breakpoint()
        return False

    def _fail(self, message):
        print("ERROR: %s" % message)
        self._failure = True


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

    def check_equal(self, a, b):
        return self._test_case.assertEqual(a, b)

    def check(self, cond):
        return self._test_case.assertTrue(cond)

    def check_not(self, cond):
        return self._test_case.assertFalse(cond)
