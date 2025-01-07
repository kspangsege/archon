class List:
    def __init__(self):
        self._tests = []

    def add(self, name, descr, func):
        test = _Test(name, descr, func)
        self._tests.append(test)

    def run(self):
        context = Context()
        for test in self._tests:
            print("%s: %s" % (test.name, test.descr))
            test.func(context)
        if not context._failure:
            print("Success")
        else:
            print("FAILURE")


class Context:
    def __init__(self):
        self._failure = False

    def check_equal(self, a, b):
        return self._check(a == b, "check_equal(%s, %s) failed", a, b)

    def check(self, cond):
        return self._check(cond, "check(%s) failed", cond)

    def _check(self, cond, message, *params):
        if cond:
            return True
        self._fail(message % params)
        return False

    def _fail(self, message):
        print("ERROR: %s" % message)
        self._failure = True


class _Test:
    def __init__(self, name, descr, func):
        self.name = name
        self.descr = descr
        self.func = func
