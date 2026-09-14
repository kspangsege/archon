from __future__ import annotations
from typing import Any
from collections.abc import Iterable

import re

import base
import log


def parse(args: list[str], spec: Spec, result: Result, logger: log.Logger) -> bool:
    return _parse(args, spec, result, logger)


def show_help(spec: Spec, logger: log.Logger) -> None:
    return _show_help(spec, logger)


class Spec:
    def __init__(self) -> None:
        self._options: list[Spec._Option] = []

    def opt(self, forms: Iterable[str], value: Any = True, default_value: Any = None) -> None:
        option = self._SimpleOption(forms, value, default_value)
        self._options.append(option)

    def opt_with_arg(self, forms: Iterable[str], value_mapper, default_value: Any = None) -> None:
        option = self._OptionWithArgument(forms, value_mapper, default_value)
        self._options.append(option)

    def short_circuit_opt(self, forms: Iterable[str]) -> None:
        option = self._ShortCircuitOption(forms)
        self._options.append(option)

    def stop_opt(self, forms: Iterable[str]) -> None:
        option = self._StopOption(forms)
        self._options.append(option)

    class _Option:
        def __init__(self, forms: Iterable[str], default_value: Any, allow_arg: bool, require_arg: bool) -> None:
            self.forms = list(forms)
            self.default_value = default_value
            self.allow_arg   = allow_arg
            self.require_arg = require_arg

    class _SimpleOption(_Option):
        def __init__(self, forms: Iterable[str], value: Any, default_value: Any) -> None:
            allow_arg   = False
            require_arg = False
            Spec._Option.__init__(self, forms, default_value, allow_arg, require_arg)
            self.value = value

    class _OptionWithArgument(_Option):
        def __init__(self, forms: Iterable[str], value_mapper, default_value: Any) -> None:
            allow_arg   = True
            require_arg = True
            Spec._Option.__init__(self, forms, default_value, allow_arg, require_arg)
            self.value_mapper = value_mapper

    class _ShortCircuitOption(_SimpleOption):
        def __init__(self, forms: Iterable[str]) -> None:
            value         = True
            default_value = False
            Spec._SimpleOption.__init__(self, forms, value, default_value)

    class _StopOption(_Option):
        def __init__(self, forms: Iterable[str]) -> None:
            default_value = None
            allow_arg     = False
            require_arg   = False
            Spec._Option.__init__(self, forms, default_value, allow_arg, require_arg)


class Result:
    def __init__(self) -> None:
        self.args: list[str] = []
        self._option_map: dict[str, int] = {}
        self._option_values: list[str] = []

    def get_opt(self, form: str) -> Any:
        index = self._option_map[form]
        return self._option_values[index]


def _parse(args: list[str], spec: Spec, result: Result, logger: log.Logger) -> bool:
    for i, option in enumerate(spec._options):
        for form in option.forms:
            assert re.fullmatch(r"--[0-9A-Za-z-]*|-[0-9A-Za-z]", form)
            assert form not in result._option_map
            result._option_map[form] = i
        result._option_values.append(option.default_value)

    short_circuit = False
    no_more_options = False
    errors = []
    def error(message: str, *args: Any) -> None:
        errors.append(message % args)

    next_arg_index = 0
    current_arg = None
    current_arg_offset = None
    def process_option(form: str, option_arg_in_current_arg: bool, option_arg_is_forced: bool):
        nonlocal short_circuit, no_more_options, next_arg_index, current_arg_offset
        assert current_arg is not None

        option_index = result._option_map.get(form)
        if option_index is None:
            if form == current_arg:
                error("Unknown command-line option %s", form)
            else:
                error("Unknown command-line option %s in %s", form, base.quote(current_arg))
            current_arg_offset = len(current_arg)
            return

        option = spec._options[option_index]
        option_arg = None
        if option_arg_is_forced or option.allow_arg:
            if option_arg_in_current_arg:
                option_arg = current_arg[current_arg_offset:]
                current_arg_offset = len(current_arg)
            elif next_arg_index < len(args):
                option_arg = args[next_arg_index]
                next_arg_index += 1
            else:
                assert not option_arg_is_forced

        if option.require_arg and option_arg is None:
            if form == current_arg:
                error("Missing argument for command-line option %s", form)
            else:
                error("Missing argument for command-line option %s in %s", form, base.quote(current_arg))
            return

        if not option.allow_arg and option_arg is not None:
            assert option_arg_is_forced
            if form == current_arg:
                error("No argument (%s) allowed for command-line option %s", base.quote(option_arg), form)
            else:
                error("No argument (%s) allowed for command-line option %s in %s", base.quote(option_arg), form,
                      base.quote(current_arg))
            return

        if isinstance(option, Spec._SimpleOption):
            assert option_arg is None
            result._option_values[option_index] = option.value
            if isinstance(option, Spec._ShortCircuitOption):
                short_circuit = True
            return

        if isinstance(option, Spec._OptionWithArgument):
            assert option_arg is not None
            try:
                result._option_values[option_index] = option.value_mapper(option_arg)
            except ValueError:
                if form == current_arg:
                    error("Invalid argument (%s) for command-line option %s", base.quote(option_arg), form)
                else:
                    error("Invalid argument (%s) for command-line option %s in %s", base.quote(option_arg), form,
                          base.quote(current_arg))
            return

        if isinstance(option, Spec._StopOption):
            assert option_arg is None
            no_more_options = True
            return

        assert False

    while next_arg_index < len(args):
        current_arg = args[next_arg_index]
        next_arg_index += 1
        if no_more_options or len(current_arg) < 2 or current_arg[0] != "-":
            result.args.append(current_arg)
            continue
        if current_arg[1] == "-":
            form = current_arg
            option_arg_in_current_arg = False
            option_arg_is_forced = False
            i = current_arg.find("=", 2)
            if i >= 0:
                form = current_arg[:i]
                current_arg_offset = i + 1
                option_arg_in_current_arg = True
                option_arg_is_forced = True
            process_option(form, option_arg_in_current_arg, option_arg_is_forced)
            continue
        current_arg_offset = 1
        while current_arg_offset < len(current_arg):
            form = "-%s" % current_arg[current_arg_offset]
            current_arg_offset += 1
            option_arg_in_current_arg = current_arg_offset < len(current_arg)
            option_arg_is_forced = option_arg_in_current_arg and current_arg[current_arg_offset] == "-"
            process_option(form, option_arg_in_current_arg, option_arg_is_forced)
    if short_circuit or not errors:
        return True
    for error_2 in errors:
        logger.error("%s", error_2)
    return False


def _show_help(spec: Spec, logger: log.Logger):
    lines = []
    def add(message: str, *args: Any) -> None:
        lines.append(message % args)
    add("Options:")
    for option in spec._options:
        add("  %s", ", ".join(option.forms))
    logger.info("%s", "\n".join(lines))
