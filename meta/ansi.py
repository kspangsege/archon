from __future__ import annotations
from typing import Any, TextIO

import enum
import sys
import os


def is_ansi_term(output_stream: TextIO) -> bool:
    if output_stream.isatty():
        return sys.platform != "win32" or "ANSICON" in os.environ
    return False


class AnsiTermColor(enum.Enum):
    BLACK   = 0
    RED     = 1
    GREEN   = 2
    YELLOW  = 3
    BLUE    = 4
    MAGENTA = 5
    CYAN    = 6
    WHITE   = 7


def ansi_term_set_color(color: AnsiTermColor) -> str:
    if color == AnsiTermColor.BLACK:
        return "\033[30m"
    if color == AnsiTermColor.RED:
        return "\033[31m"
    if color == AnsiTermColor.GREEN:
        return "\033[32m"
    if color == AnsiTermColor.YELLOW:
        return "\033[33m"
    if color == AnsiTermColor.BLUE:
        return "\033[34m"
    if color == AnsiTermColor.MAGENTA:
        return "\033[35m"
    if color == AnsiTermColor.CYAN:
        return "\033[36m"
    if color == AnsiTermColor.WHITE:
        return "\033[37m"
    assert False


def ansi_term_reset_color() -> str:
    return "\033[39m"
