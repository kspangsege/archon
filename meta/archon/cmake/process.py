from __future__ import annotations
from typing import Protocol, Any, assert_never
from dataclasses import dataclass

import enum
import pathlib

import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.lowlevel_parser as _clp


def process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    return _process(cmake_path, logger)


def _process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    commands    = dict[str, _Command]()
    cache       = dict[str, _CacheEntry]()
    environment = dict[str, _Value]()
    _define_built_in_commands(commands)

    def process_file(cmake_path: pathlib.Path, directory: _Directory) -> None:
        def error_handler(pos: _l.FullFilePos, message: str, *args: Any) -> None:
            error(cmake_path, pos, message, *args)
        for invoc in _clp.parse(cmake_path, error_handler):
            match invoc:
                case _clp.SimpleInvoc():
                    process_simple(invoc, cmake_path, directory)
                    continue
                case _clp.IfInvoc():
                    process_if(invoc, cmake_path, directory)
                    continue
                case _clp.ForeachInvoc():
                    process_foreach(invoc, cmake_path, directory)
                    continue
                case _clp.WhileInvoc():
                    process_while(invoc, cmake_path, directory)
                    continue
                case _clp.MacroDefInvoc():
                    process_macro(invoc, cmake_path, directory)
                    continue
                case _clp.FunctionDefInvoc():
                    process_function(invoc, cmake_path, directory)
                    continue
                case _clp.BlockInvoc():
                    process_block(invoc, cmake_path, directory)
                    continue
            assert_never(invoc)

    def process_simple(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        command = commands.get(invoc.command_name_cf)
        if not command:
            error(cmake_path, invoc.pos, "Invocation of undefined command %s()", invoc.command_name)
            return
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(cmake_path, invoc.pos, "Invocation of unsupported command %s()", invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        process_set(invoc, cmake_path, directory)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        process_unset(invoc, cmake_path, directory)
                        return
                    case _BuiltInCommand.Which.MESSAGE:
                        process_message(invoc, cmake_path, directory)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        process_include(invoc, cmake_path, directory)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        process_add_subdirectory(invoc, cmake_path, directory)
                        return
                assert_never(which)
        assert_never(command)

    def process_if(invoc: _clp.IfInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_foreach(invoc: _clp.ForeachInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_while(invoc: _clp.WhileInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_macro(invoc: _clp.MacroDefInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_function(invoc: _clp.FunctionDefInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_block(invoc: _clp.BlockInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_set(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        arguments = expand_arguments(invoc, cmake_path, directory)
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_unset(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_message(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_include(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_add_subdirectory(invoc: _clp.SimpleInvoc, cmake_path: pathlib.Path, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def expand_arguments(invoc: _clp.InvocBase, cmake_path: pathlib.Path, directory: _Directory) -> list[_Argument]:
        def evaluate(expr: _clp.Expr) -> tuple[_Value, bool]:
            if isinstance(expr, _clp.StringExpr):
                is_derived = False
                return _CertainValue(expr.string), is_derived
            if isinstance(expr, _clp.CompositeExpr):
                string = ""
                for part in expr.parts:
                    value, _ = evaluate(part)
                    if isinstance(value, _CertainValue):
                        if value.string is not None:
                            string += value.string
                        continue
                    if isinstance(value, _UncertainValue):
                        is_derived = False
                        return value, is_derived
                    assert_never(value)
                is_derived = True
                return _CertainValue(string), is_derived
            if isinstance(expr, _clp.ExpansionExpr):
                value, _ = evaluate(expr.name_expr)
                if isinstance(value, _CertainValue):
                    name = value.string or ""
                    value_2 = resolve_variable(expr.resolution_type, name, cmake_path, expr.pos, directory)
                    is_derived = True
                    return value_2, is_derived
                if isinstance(value, _UncertainValue):
                    is_derived = False
                    return value, is_derived
                assert_never(value)
            assert_never(expr)

        arguments: list[_Argument] = []
        for protoarg in invoc.arguments:
            value, is_derived = evaluate(protoarg.expr)
            arg: _Argument
            if isinstance(value, _CertainValue):
                string = value.string or ""
                substrings = []
                if protoarg.was_quoted_or_bracketed:
                    substrings = [string]
                else:
                    substrings = [s for s in _cu.list_split(string) if s]
                    is_derived = True
                for substring in substrings:
                    arg = _CertainArgument(protoarg.was_quoted_or_bracketed, protoarg.pos, substring, is_derived)
                    arguments.append(arg)
                continue
            if isinstance(value, _UncertainValue):
                arg = _UncertainArgument(protoarg.was_quoted_or_bracketed, protoarg.pos, value.reason)
                arguments.append(arg)
                continue
            assert_never(value)
        return arguments

    def resolve_variable(resolution_type: _clp.ResolutionType, variable_name: str, cmake_path: pathlib.Path,
                         pos: _l.FullFilePos, directory: _Directory) -> _Value:
        match resolution_type:
            case _clp.ResolutionType.GENERAL:
                value = directory.resolve_variable(variable_name)
                if value:
                    return value
                entry = cache.get(variable_name)
                if entry:
                    return entry.value
                assert False        
            case _clp.ResolutionType.CACHE:
                entry = cache.get(variable_name)
                if entry:
                    return entry.value
                assert False        
            case _clp.ResolutionType.ENV:
                value = environment.get(variable_name)
                if value:
                    return value
                assert False        
        assert_never(resolution_type)

    errors_seen = False
    def error(cmake_path: pathlib.Path, pos: _l.FullFilePos, message: str, *args: Any):
        nonlocal errors_seen
        errors_seen = True
        context = _l.FileContext(cmake_path, pos)
        _l.FileContextLogger(logger, context).error(message, *args)

    directory = _Directory()
    process_file(cmake_path, directory)
    return not errors_seen


type _Command = _BuiltInCommand

@dataclass(slots=True, frozen=True)
class _BuiltInCommand:
    class Which(enum.Enum):
        UNSUPPORTED      = 0
        SET              = 1
        UNSET            = 2
        MESSAGE          = 3
        INCLUDE          = 4
        ADD_SUBDIRECTORY = 5
    which: Which


class _CacheEntry:
    value: _Value


type _Argument = _CertainArgument | _UncertainArgument

@dataclass(slots=True, frozen=True)
class _ArgumentBase:
    was_quoted_or_bracketed: bool
    pos:                     _l.FullFilePos

@dataclass(slots=True, frozen=True)
class _CertainArgument(_ArgumentBase):
    string:     str
    is_derived: bool

@dataclass(slots=True, frozen=True)
class _UncertainArgument(_ArgumentBase):
    reason: _ValueUncertaintyReason


class _Directory:
    def __init__(self, parent: _Directory | None = None) -> None:
        self._parent = parent
        self._variables:         dict[str, _CertainValue]           = {}
        self._tainted_variables: dict[str, _ValueUncertaintyReason] = {}

    def resolve_variable(self, name: str) -> _Value | None:
        reason = self._tainted_variables.get(name)
        if reason:
            return _UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            return value
        if self._parent:
            return self._parent.resolve_variable(name)
        return None


type _Value = _CertainValue | _UncertainValue

@dataclass(slots=True, frozen=True)
class _CertainValue:
    string: str | None

@dataclass(slots=True, frozen=True)
class _UncertainValue:
    reason: _ValueUncertaintyReason


@dataclass(slots=True, frozen=True)
class _ValueUncertaintyReason:
    pass


def _define_built_in_commands(commands: dict[str, _Command]) -> None:
    def define(name_cf: str, which: _BuiltInCommand.Which):
        commands[name_cf] = _BuiltInCommand(which)
    define("set",                    _BuiltInCommand.Which.SET)
    define("unset",                  _BuiltInCommand.Which.UNSET)
    define("message",                _BuiltInCommand.Which.MESSAGE)
    define("include",                _BuiltInCommand.Which.INCLUDE)
    define("add_subdirectory",       _BuiltInCommand.Which.ADD_SUBDIRECTORY)
    define("cmake_minimum_required", _BuiltInCommand.Which.UNSUPPORTED)
    define("project",                _BuiltInCommand.Which.UNSUPPORTED)
    define("option",                 _BuiltInCommand.Which.UNSUPPORTED)
