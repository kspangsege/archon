from __future__ import annotations

import typing
import abc
import dataclasses
import enum
import collections
import re

import archon.base as _b
import archon.log as _l


def parse(args: list[str], spec: Spec, logger: _l.Logger) -> tuple[bool, list[str]]:
    return _parse(args, spec, logger)


def show_help(spec: Spec, logger: _l.Logger) -> None:
    return _show_help(spec, logger)


class Spec:
    def __init__(self) -> None:
        self._options = list[_Option]()

    def opt(self, forms: list[str], action: OptionAction) -> None:
        self._options.append(_Option(list(forms), action))


class OptionAction(abc.ABC):
    def __init__(self, has_arg: HasArg) -> None:
        match has_arg:
            case OptionAction.HasArg.NEVER:
                self._allow_arg   = False
                self._require_arg = False
            case OptionAction.HasArg.OPTIONAL:
                self._allow_arg   = True
                self._require_arg = False
            case OptionAction.HasArg.ALWAYS:
                self._allow_arg   = True
                self._require_arg = True
            case _:
                typing.assert_never(has_arg)

    class HasArg(enum.Enum):
        NEVER    = 0
        OPTIONAL = 1
        ALWAYS   = 2

    @property
    def allow_arg(self) -> bool:
        return self._allow_arg

    @property
    def require_arg(self) -> bool:
        return self._require_arg

    @abc.abstractmethod
    def invoke(self, arg: str | None) -> None:
        ...


class Raise(OptionAction):
    def __init__(self, target: _b.Wrap[bool]) -> None:
        OptionAction.__init__(self, OptionAction.HasArg.NEVER)
        self._target = target

    @typing.override
    def invoke(self, arg: str | None) -> None:
        assert arg is None
        self._target.value = True


class Assign[T](OptionAction):
    def __init__(self, target: _b.Wrap[T], value: T) -> None:
        OptionAction.__init__(self, OptionAction.HasArg.NEVER)
        self._target = target
        self._value  = value

    @typing.override
    def invoke(self, arg: str | None) -> None:
        assert arg is None
        self._target.value = self._value


class AssignWithArg[T](OptionAction):
    def __init__(self, target: _b.Wrap[T], mapper: ValueMapper[T]) -> None:
        OptionAction.__init__(self, OptionAction.HasArg.ALWAYS)
        self._target = target
        self._mapper = mapper

    @typing.override
    def invoke(self, arg: str | None) -> None:
        assert arg is not None
        self._target.value = self._mapper(arg)


class Stop(OptionAction):
    def __init__(self) -> None:
        OptionAction.__init__(self, OptionAction.HasArg.NEVER)

    @typing.override
    def invoke(self, arg: str | None) -> None:
        assert False


class ShortCircuit(Raise):
    pass


type ValueMapper[T] = collections.abc.Callable[[str], T]


class SpecError(Exception):
    pass








def _parse(args: list[str], spec: Spec, logger: _l.Logger) -> tuple[bool, list[str]]:
    option_map = dict[str, int]()
    for i, option in enumerate(spec._options):
        for form in option.forms:
            if not re.fullmatch(r"--[0-9\-A-Za-z]*|-[0-9A-Za-z]", form):
                raise SpecError("Invalid option form (%s)", _b.quote(form))
            if form in option_map:
                raise SpecError("Reuse of option form (%s)", _b.quote(form))
            option_map[form] = i

    short_circuit = False
    no_more_options = False
    errors = []
    def error(message: str, *args: typing.Any) -> None:
        errors.append(message % args)

    next_arg_index = 0
    current_arg:        str | None = None
    current_arg_offset: int | None = None
    def process_option(form: str, option_arg_in_current_arg: bool, option_arg_is_forced: bool) -> None:
        nonlocal short_circuit, no_more_options, next_arg_index, current_arg_offset
        assert current_arg is not None

        option_index = option_map.get(form)
        if option_index is None:
            if form == current_arg:
                error("Unknown command-line option %s", form)
            else:
                error("Unknown command-line option %s in %s", form, _b.quote(current_arg))
            current_arg_offset = len(current_arg)
            return

        option = spec._options[option_index]
        option_arg: str | None = None
        if option_arg_is_forced or option.action.allow_arg:
            if option_arg_in_current_arg:
                option_arg = current_arg[current_arg_offset:]
                current_arg_offset = len(current_arg)
            elif next_arg_index < len(args):
                option_arg = args[next_arg_index]
                next_arg_index += 1
            else:
                assert not option_arg_is_forced

        if option.action.require_arg and option_arg is None:
            if form == current_arg:
                error("Missing argument for command-line option %s", form)
            else:
                error("Missing argument for command-line option %s in %s", form, _b.quote(current_arg))
            return

        if not option.action.allow_arg and option_arg is not None:
            assert option_arg_is_forced
            if form == current_arg:
                error("No argument (%s) allowed for command-line option %s", _b.quote(option_arg), form)
            else:
                error("No argument (%s) allowed for command-line option %s in %s", _b.quote(option_arg), form,
                      _b.quote(current_arg))
            return

        if isinstance(option.action, Stop):
            no_more_options = True
            return

        try:
            option.action.invoke(option_arg)
        except ValueError:
            assert option_arg is not None
            if form == current_arg:
                error("Invalid argument (%s) for command-line option %s", _b.quote(option_arg), form)
            else:
                error("Invalid argument (%s) for command-line option %s in %s", _b.quote(option_arg), form,
                      _b.quote(current_arg))

        if isinstance(option.action, ShortCircuit):
            short_circuit = True

    new_args = []
    while next_arg_index < len(args):
        current_arg = args[next_arg_index]
        next_arg_index += 1
        if no_more_options or len(current_arg) < 2 or current_arg[0] != "-":
            new_args.append(current_arg)
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
    if short_circuit:
        return True, []
    if not errors:
        return True, new_args
    for error_2 in errors:
        logger.error("%s", error_2)
    return False, []


def _show_help(spec: Spec, logger: _l.Logger) -> None:
    lines = []
    def add(message: str, *args: typing.Any) -> None:
        lines.append(message % args)
    add("Options:")
    for option in spec._options:
        add("  %s", ", ".join(option.forms))
    logger.info("%s", "\n".join(lines))


@dataclasses.dataclass(slots=True, frozen=True)
class _Option:
    forms:  list[str]
    action: OptionAction
