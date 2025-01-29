import sys

class Logger:
    def log(self, message, *params):
        raise NotImplementedError

class NullLogger(Logger):
    def log(self, message, *params):
        pass

class FileLogger(Logger):
    def __init__(self, file = sys.stdout):
        self._file = file
    def log(self, message, *params):
        message_2 = message % params
        self._file.write("%s\n" % message_2)
