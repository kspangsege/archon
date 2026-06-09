from __future__ import annotations
from typing import Any, Protocol, assert_never
from collections.abc import Iterable, Callable, Collection, Container
from dataclasses import dataclass

import enum
import re
import pathlib

import base
import log
import cmake_utils as _cu
import lowlevel_cmake_parser as _lcp


class Parser:
    def __init__(self, ignore_commands: Iterable[str], include_resolver: IncludeResolver,
                 error_handler: ErrorHandler) -> None:
        self._ignore_commands  = set(ignore_commands)
        self._include_resolver = include_resolver
        self._error_handler    = error_handler

    def parse(self, cmake_path: pathlib.Path, protoinvoc: _lcp.Protoinvoc,
              variable_resolver: VariableResolver) -> Invocation | None:
        name = protoinvoc.name.casefold()
        if name in self._ignore_commands:
            return None

        try:
            arguments = _expand_arguments(protoinvoc.arguments, variable_resolver)
            server = _ArgumentServer(cmake_path, protoinvoc, arguments, self._error_handler)

            if name == "set":
                return self._parse_set(protoinvoc, server)

            if name == "unset":
                return self._parse_unset(protoinvoc, server)

            if name == "message":
                return self._parse_message(protoinvoc, server)

            if name == "link_libraries":
                return self._parse_link_libraries(protoinvoc, server)

            if name == "add_library":
                return self._parse_add_library(protoinvoc, server)

            if name == "add_executable":
                return self._parse_add_executable(protoinvoc, server)

            if name == "set_target_properties":
                return self._parse_set_target_properties(protoinvoc, server)

            if name == "target_sources":
                return self._parse_target_sources(protoinvoc, server)

            if name == "target_link_libraries":
                return self._parse_target_link_libraries(protoinvoc, server)

            if name == "if":
                return self._parse_if(protoinvoc, server)

            if name == "elseif":
                return self._parse_elseif(protoinvoc, server)

            if name == "else":
                return self._parse_else(protoinvoc, server)

            if name == "endif":
                return self._parse_endif(protoinvoc, server)

            #       
            #
            # if name == "macro":
            #     return self._parse_macro(protoinvoc, server)

            if name == "include":
                return self._parse_include(protoinvoc, server)

            if name == "add_subdirectory":
                return self._parse_add_subdirectory(protoinvoc, server)

            context = log.FileContext(cmake_path, protoinvoc.line_no)
            self._error_handler(context, "Unsupported command %s()", protoinvoc.name)
            return None

        except _UnsupportedVariableExpansion as e:
            context = log.FileContext(cmake_path, e.line_no)
            self._error_handler(context, "Unsupported variable expansion in %s() invocation", protoinvoc.name)
            return None

        except _UnsupportedCommandSyntaxException:
            context = log.FileContext(cmake_path, protoinvoc.line_no)
            self._error_handler(context, "Unsupported %s() syntax", protoinvoc.name)
            return None

        except _UncertainArgumentException as e:
            def get_qual(reason: VariableUncertaintyReason) -> str:
                if isinstance(reason, UnspecifiedVariableValueReason):
                    return "unspecified"
                if isinstance(reason, UncertainVariableAssignmentReason):
                    return "uncertain"
                assert_never(reason)
            context = log.FileContext(cmake_path, e.reason.line_no)
            variable_uncertainty_reason = e.reason.variable_uncertainty_reason
            variable_name = variable_uncertainty_reason.variable_name
            qual = get_qual(variable_uncertainty_reason)
            self._error_handler(context, "Unable to parse %s() arguments due to expansion of variable (%s) with %s "
                                "value", protoinvoc.name, base.quote(variable_name), qual)
            while True:
                if isinstance(variable_uncertainty_reason, UnspecifiedVariableValueReason):
                    break
                if isinstance(variable_uncertainty_reason, UncertainVariableAssignmentReason):
                    assignment_uncertainty_reason = variable_uncertainty_reason.assignment_uncertainty_reason
                    if isinstance(assignment_uncertainty_reason, UncertainAssignedValueReason):
                        context = assignment_uncertainty_reason.assignment_context
                        variable_uncertainty_reason = assignment_uncertainty_reason.variable_uncertainty_reason
                        variable_name = variable_uncertainty_reason.variable_name
                        qual = get_qual(variable_uncertainty_reason)
                        self._error_handler(context, "Caused by assignment from variable (%s) with %s value",
                                            base.quote(variable_name), qual)
                        continue
                    if isinstance(assignment_uncertainty_reason, UncertainAssignmentOccurrenceReason):
                        context = assignment_uncertainty_reason.assignment_context
                        self._error_handler(context, "Caused by assignment with uncertain occurrence")
                        conditional_uncertainty_reason = assignment_uncertainty_reason.conditional_uncertainty_reason
                        context = conditional_uncertainty_reason.conditional_context
                        variable_uncertainty_reason = conditional_uncertainty_reason.variable_uncertainty_reason
                        variable_name = variable_uncertainty_reason.variable_name
                        qual = get_qual(variable_uncertainty_reason)
                        self._error_handler(context, "Caused by condition involving variable (%s) with %s value",
                                            base.quote(variable_name), qual)
                        continue
                    assert_never(assignment_uncertainty_reason)
                assert_never(variable_uncertainty_reason)
            return None

    def _parse_set(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> SetInvoc | None:
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern            
        variable = server.consume_any()
        if not variable:
            server.error(server.next_line_no(), "Missing variable name in set()")
            return None
        var_name = _cu.parse_var_name(variable.value)
        match var_name:
            case _cu.GeneralVarName(name):
                pass
            case _cu.CacheVarName() | _cu.EnvVarName():
                raise _UnsupportedCommandSyntaxException
            case _:
                assert_never(var_name)
        values = []
        parent_scope = False
        try:
            while True:
                if server.consume_keyword({"CACHE"}):
                    raise _UnsupportedCommandSyntaxException
                if server.num_remaining() == 1 and server.consume_keyword({"PARENT_SCOPE"}):
                    parent_scope = True
                    break
                value = server.consume_any()
                if not value:
                    break
                values.append(value)
        except _UncertainArgumentException as e:
            raise UnparsableSetInvocException(e.reason) from None
        return SetInvoc(variable, values, parent_scope, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_unset(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> UnsetInvoc | None:
        variable = server.consume_any()
        if not variable:
            server.error(server.next_line_no(), "Missing variable name in unset()")
            return None
        var_name = _cu.parse_var_name(variable.value)
        match var_name:
            case _cu.GeneralVarName(name):
                pass
            case _cu.CacheVarName() | _cu.EnvVarName():
                raise _UnsupportedCommandSyntaxException
            case _:
                assert_never(var_name)
        parent_scope = False
        if server.consume_keyword({"CACHE"}):
            raise _UnsupportedCommandSyntaxException
        if server.num_remaining() == 1 and server.consume_keyword({"PARENT_SCOPE"}):
            parent_scope = True
        if not server.at_end():
            server.error(server.next_line_no(), "Too many arguments in unset() invocation")
            return None
        return UnsetInvoc(variable, parent_scope, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_message(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> MessageInvoc:
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedCommandSyntaxException
        level = "NOTICE"
        arg = server.consume_keyword({"FATAL_ERROR", "SEND_ERROR", "WARNING", "AUTHOR_WARNING", "DEPRECATION",
                                      "NOTICE", "STATUS", "VERBOSE", "DEBUG", "TRACE"})
        if arg:
            level = arg.value
        message = ""
        while True:
            arg = server.consume_any()
            if not arg:
                break
            message += arg.value
        level_2 = _MESSAGE_LEVEL_MAP[level]
        return MessageInvoc(level_2, message, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_link_libraries(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> LinkLibrariesInvoc:
        items = []
        config = "general"
        while True:
            def config_pred(value: str) -> bool:
                return value in {"debug", "optimized", "general"}
            arg = server.consume_any_if(config_pred)
            if arg:
                config = arg.value
                continue
            item = server.consume_any()
            if not item:
                break
            config_2 = _CONFIG_MAP[config]
            items.append(LinkItem(Scope.PUBLIC, config_2, item.value, item.line_no))
        return LinkLibrariesInvoc(items, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_add_library(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> AddLibraryInvoc | None:
        target_name = server.consume_any()
        if not target_name:
            server.error(server.next_line_no(), "Missing target name in add_library()")
            return None
        library_type = LibraryType.NORMAL
        allow_source_files = False
        syntax_variant = server.consume_keyword({"OBJECT", "INTERFACE", "IMPORTED", "ALIAS"})
        if not syntax_variant:
            type_ = server.consume_keyword({"STATIC", "SHARED", "MODULE"})
            if type_:
                raise _UnsupportedCommandSyntaxException
            exclude_from_all = server.consume_keyword({"EXCLUDE_FROM_ALL"})
            if exclude_from_all:
                raise _UnsupportedCommandSyntaxException
            allow_source_files = True
        elif syntax_variant.value == "OBJECT":
            library_type = LibraryType.OBJECT
            allow_source_files = True
        elif syntax_variant.value == "INTERFACE":
            library_type = LibraryType.INTERFACE
        else:
            raise _UnsupportedCommandSyntaxException
        source_files = []
        if library_type != LibraryType.INTERFACE:
            while True:
                path = server.consume_any()
                if path is None:
                    break
                source_files.append(Path(pathlib.Path(path.value), path.line_no))
        else:
            if not server.at_end():
                server.error(server.next_line_no(), "Extraneous arguments in INTERFACE variant of add_library()")
                return None
        return AddLibraryInvoc(target_name.value, library_type, source_files, protoinvoc.line_no,
                               protoinvoc.closing_line_no)

    def _parse_add_executable(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> AddExecutableInvoc | None:
        target_name = server.consume_any()
        if not target_name:
            server.error(server.next_line_no(), "Missing target name in add_executable()")
            return None
        syntax_variant = server.consume_keyword({"IMPORTED", "ALIAS"})
        if syntax_variant:
            raise _UnsupportedCommandSyntaxException
        options = []
        while True:
            option = server.consume_keyword({"WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL"})
            if option is None:
                break
            options.append(option.value)
        if options:
            raise _UnsupportedCommandSyntaxException
        source_files = []
        while True:
            path = server.consume_any()
            if path is None:
                break
            source_files.append(Path(pathlib.Path(path.value), path.line_no))
        return AddExecutableInvoc(target_name.value, source_files, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_set_target_properties(self, protoinvoc: _lcp.Protoinvoc,
                                     server: _ArgumentServer) -> SetTargetPropertiesInvoc | None:
        targets = []
        while True:
            target = server.consume_not_keyword({"PROPERTIES"})
            if not target:
                break
            targets.append(Target(target.value, target.line_no))
        if not server.consume_keyword({"PROPERTIES"}):
            server.error(server.next_line_no(), "Missing PROPERTIES keyword in set_target_properties()")
            return None
        properties = []
        while True:
            name = server.consume_any()
            if not name:
                break
            value = server.consume_any()
            if not value:
                server.error(server.next_line_no(), "Missing value for property %s in set_target_properties()",
                             base.quote(name.value))
                break
            properties.append(Property(name, value))
        return SetTargetPropertiesInvoc(targets, properties, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_target_sources(self, protoinvoc: _lcp.Protoinvoc,
                              server: _ArgumentServer) -> (RegularTargetSourcesInvoc | FileSetTargetSourcesInvoc |
                                                           None):
        target_name = server.consume_any()
        if not target_name:
            server.error(server.next_line_no(), "Missing target name in target_sources()")
            return None
        scope_keywords = {"PRIVATE", "PUBLIC", "INTERFACE"}
        scope = ""
        while True:
            arg = server.consume_keyword(scope_keywords)
            if not arg:
                break
            scope = arg.value
        file_set = server.consume_keyword({"FILE_SET"}) if scope else None
        if not file_set:
            scoped_source_files = []
            if scope:
                while True:
                    arg = server.consume_keyword(scope_keywords)
                    if arg:
                        scope = arg.value
                        continue
                    path = server.consume_any()
                    if path is None:
                        break
                    scope_2  = _SCOPE_MAP[scope]
                    scoped_source_files.append(ScopedPath(scope_2, pathlib.Path(path.value), path.line_no))
            else:
                if not server.at_end():
                    server.error(server.next_line_no(), "Missing scope keyword in target_sources()")
                    return None
            return RegularTargetSourcesInvoc(target_name.value, scoped_source_files, protoinvoc.line_no,
                                             protoinvoc.closing_line_no)
        stop_keywords = scope_keywords | {"FILE_SET", "TYPE", "BASE_DIRS", "FILES"}
        file_sets = []
        while True:
            file_set_name = server.consume_not_keyword(stop_keywords)
            if not file_set_name:
                server.error(server.next_line_no(), "Missing file set name in target_sources()")
                return None
            type_keywords = {"HEADERS", "CXX_MODULES"}
            type_ = None
            if file_set_name.value in type_keywords:
                type_ = file_set_name.value
            elif not re.fullmatch(r'[0-9a-z][0-9A-Za-z_]*', file_set_name.value):
                server.error(file_set_name.line_no, "Invalid file set name (%s) in target_sources()",
                             base.quote(file_set_name.value))
                return None
            if server.consume_keyword({"TYPE"}):
                arg = server.consume_keyword(type_keywords)
                if not arg:
                    server.error(server.next_line_no(), "Missing file set type after TYPE in target_sources()")
                    return None
                type_ = arg.value
            else:
                if not type_:
                    server.error(file_set_name.line_no, "Type specification is mandatory when file set name is not "
                                 "a file set type in target_sources()")
                    return None
            base_dirs = []
            if server.consume_keyword({"BASE_DIRS"}):
                while True:
                    path = server.consume_not_keyword(stop_keywords)
                    if not path:
                        break
                    base_dirs.append(Path(pathlib.Path(path.value), path.line_no))
            files = []
            if server.consume_keyword({"FILES"}):
                while True:
                    path = server.consume_not_keyword(stop_keywords)
                    if not path:
                        break
                    files.append(Path(pathlib.Path(path.value), path.line_no))
            scope_2  = _SCOPE_MAP[scope]
            type_2 = _FILE_SET_TYPE_MAP[type_]
            closing_line_no = server.next_line_no()
            file_sets.append(FileSet(scope_2, file_set_name.value, type_2, base_dirs, files, file_set.line_no,
                                     closing_line_no))
            while True:
                arg = server.consume_keyword(scope_keywords)
                if not arg:
                    break
                scope = arg.value
            file_set = server.consume_keyword({"FILE_SET"})
            if not file_set:
                if not server.at_end():
                    server.error(server.next_line_no(), "Unexpected argument between file sets in target_sources()")
                    return None
                break
        return FileSetTargetSourcesInvoc(target_name.value, file_sets, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_target_link_libraries(self, protoinvoc: _lcp.Protoinvoc,
                                     server: _ArgumentServer) -> TargetLinkLibrariesInvoc | None:
        target_name = server.consume_any()
        if not target_name:
            server.error(server.next_line_no(), "Missing target name in target_link_libraries()")
            return None
        if server.consume_keyword({"LINK_PRIVATE", "LINK_PUBLIC", "LINK_INTERFACE_LIBRARIES"}):
            raise _UnsupportedCommandSyntaxException
        items = []
        scope_keywords = {"PRIVATE", "PUBLIC", "INTERFACE"}
        arg = server.consume_keyword(scope_keywords)
        if not arg and not server.at_end():
            raise _UnsupportedCommandSyntaxException
        if arg:
            scope = arg.value
            config = "general"
            while True:
                arg = server.consume_keyword(scope_keywords)
                if arg:
                    scope = arg.value
                    continue
                def config_pred(value: str) -> bool:
                    return value in {"debug", "optimized", "general"}
                arg = server.consume_any_if(config_pred)
                if arg:
                    config = arg.value
                    continue
                item = server.consume_any()
                if not item:
                    break
                scope_2  = _SCOPE_MAP[scope]
                config_2 = _CONFIG_MAP[config]
                items.append(LinkItem(scope_2, config_2, item.value, item.line_no))
        return TargetLinkLibrariesInvoc(target_name.value, items, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_if(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> IfInvoc:
        condition = _parse_condition(server.get_cmake_path(), server.get_arguments(), protoinvoc.closing_line_no)
        return IfInvoc(condition, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_elseif(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> ElseifInvoc:
        condition = _parse_condition(server.get_cmake_path(), server.get_arguments(), protoinvoc.closing_line_no)
        return ElseifInvoc(condition, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_else(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> ElseInvoc:
        if not server.at_end():
            raise _UnsupportedCommandSyntaxException
        return ElseInvoc(protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_endif(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> EndifInvoc:
        if not server.at_end():
            raise _UnsupportedCommandSyntaxException
        return EndifInvoc(protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_macro(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> MacroInvoc:
        assert False                 

    def _parse_include(self, protoinvoc: _lcp.Protoinvoc, server: _ArgumentServer) -> IncludeInvoc | None:
        file_or_module = server.consume_any()
        if not file_or_module:
            server.error(server.next_line_no(), "Missing file or module in include()")
            return None
        path = self._include_resolver(file_or_module.value)
        if path is None:
            return None
        if not server.at_end():
            raise _UnsupportedCommandSyntaxException
        return IncludeInvoc(path, protoinvoc.line_no, protoinvoc.closing_line_no)

    def _parse_add_subdirectory(self, protoinvoc: _lcp.Protoinvoc,
                                server: _ArgumentServer) -> AddSubdirectoryInvoc | None:
        source_dir = server.consume_any()
        if not source_dir:
            server.error(server.next_line_no(), "Missing directory path in add_subdirectory()")
            return None
        if not server.at_end():
            raise _UnsupportedCommandSyntaxException
        return AddSubdirectoryInvoc(pathlib.Path(source_dir.value), protoinvoc.line_no, protoinvoc.closing_line_no)


class IncludeResolver(Protocol):
    def __call__(self, file_or_module: str) -> pathlib.Path | None:
        ...


VariableType = _lcp.VariableExpansionExpr.Type

class VariableResolver(Protocol):
    def __call__(self, var_type: VariableType, var_name: str, line_no: int) -> Value:
        ...


class ErrorHandler(Protocol):
    def __call__(self, context: log.FileContext, message: str, *args: Any) -> None:
        ...


type Invocation = (SetInvoc | UnsetInvoc | MessageInvoc | LinkLibrariesInvoc | AddLibraryInvoc | AddExecutableInvoc |
                   SetTargetPropertiesInvoc | RegularTargetSourcesInvoc | FileSetTargetSourcesInvoc |
                   TargetLinkLibrariesInvoc | IfInvoc | ElseifInvoc | ElseInvoc | EndifInvoc | MacroInvoc |
                   IncludeInvoc | AddSubdirectoryInvoc)

class InvocationBase:
    def __init__(self, line_no: int, closing_line_no: int) -> None:
        self.line_no         = line_no
        self.closing_line_no = closing_line_no

class SetInvoc(InvocationBase):
    def __init__(self, variable: String, values: Iterable[String], parent_scope: bool, line_no: int,
                 closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.variable     = variable
        self.values       = list(values)
        self.parent_scope = parent_scope

class UnsetInvoc(InvocationBase):
    def __init__(self, variable: String, parent_scope: bool, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.variable     = variable
        self.parent_scope = parent_scope

class MessageInvoc(InvocationBase):
    class Level(enum.Enum):
        FATAL_ERROR    = 0
        SEND_ERROR     = 1
        WARNING        = 2
        AUTHOR_WARNING = 3
        DEPRECATION    = 4
        NOTICE         = 5
        STATUS         = 6
        VERBOSE        = 7
        DEBUG          = 8
        TRACE          = 9
    def __init__(self, level: MessageInvoc.Level, message: str, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.level   = level
        self.message = message

class LinkLibrariesInvoc(InvocationBase):
    def __init__(self, items: Iterable[LinkItem], line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.items = list(items)

class AddLibraryInvoc(InvocationBase):
    def __init__(self, target_name: str, library_type: LibraryType, source_files: Iterable[Path], line_no: int,
                 closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.target_name  = target_name
        self.library_type = library_type
        self.source_files = list(source_files)

class AddExecutableInvoc(InvocationBase):
    def __init__(self, target_name: str, source_files: Iterable[Path], line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.target_name  = target_name
        self.source_files = list(source_files)

class SetTargetPropertiesInvoc(InvocationBase):
    def __init__(self, targets: Iterable[Target], properties: Iterable[Property], line_no: int,
                 closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.targets    = list(targets)
        self.properties = list(properties)

class RegularTargetSourcesInvoc(InvocationBase):
    def __init__(self, target_name: str, scoped_source_files: Iterable[ScopedPath], line_no: int,
                 closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.target_name         = target_name
        self.scoped_source_files = list(scoped_source_files)

class FileSetTargetSourcesInvoc(InvocationBase):
    def __init__(self, target_name: str, file_sets: Iterable[FileSet], line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.target_name = target_name
        self.file_sets   = list(file_sets)

class TargetLinkLibrariesInvoc(InvocationBase):
    def __init__(self, target_name: str, items: Iterable[LinkItem], line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.target_name = target_name
        self.items       = list(items)

class IfInvoc(InvocationBase):
    def __init__(self, condition: Condition, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.condition = condition

class ElseifInvoc(InvocationBase):
    def __init__(self, condition: Condition, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.condition = condition

class ElseInvoc(InvocationBase):
    def __init__(self, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)

class EndifInvoc(InvocationBase):
    def __init__(self, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)

class MacroInvoc(InvocationBase):
    def __init__(self, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)

class IncludeInvoc(InvocationBase):
    def __init__(self, path: pathlib.Path, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.path = path

class AddSubdirectoryInvoc(InvocationBase):
    def __init__(self, path: pathlib.Path, line_no: int, closing_line_no: int) -> None:
        InvocationBase.__init__(self, line_no, closing_line_no)
        self.path = path


class LibraryType(enum.Enum):
    NORMAL    = 1
    OBJECT    = 2
    INTERFACE = 3


class Scope(enum.Enum):
    PRIVATE   = 1
    PUBLIC    = 2
    INTERFACE = 3


class String:
    def __init__(self, value: str, line_no: int) -> None:
        self.value   = value
        self.line_no = line_no


class LinkItem(String):
    class Config(enum.Enum):
        DEBUG     = 0
        OPTIMIZED = 1
        GENERAL   = 2
    def __init__(self, scope: Scope, config: LinkItem.Config, value: str, line_no: int) -> None:
        String.__init__(self, value, line_no)
        self.scope  = scope
        self.config = config


class Path:
    def __init__(self, path: pathlib.Path, line_no: int) -> None:
        self.path    = path
        self.line_no = line_no

class ScopedPath(Path):
    def __init__(self, scope: Scope, path: pathlib.Path, line_no: int) -> None:
        Path.__init__(self, path, line_no)
        self.scope = scope


class FileSet:
    class Type(enum.Enum):
        HEADERS     = 0
        CXX_MODULES = 1
    def __init__(self, scope: Scope, name: str, type_: FileSet.Type, base_dirs: Iterable[Path], files: Iterable[Path],
                 line_no: int, closing_line_no: int) -> None:
        self.scope           = scope
        self.name            = name
        self.type_           = type_
        self.base_dirs       = list(base_dirs)
        self.files           = list(files)
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class Target:
    def __init__(self, name: str, line_no: int) -> None:
        self.name    = name
        self.line_no = line_no


class Property:
    def __init__(self, name: String, value: String) -> None:
        self.name  = name
        self.value = value


type Condition = FalseCondition | UncertainCondition | ArgumentCondition | UnopCondition | BinopCondition

@dataclass(slots=True)
class ConditionBase:
    line_no: int

@dataclass(slots=True)
class FalseCondition(ConditionBase):
    pass

@dataclass(slots=True)
class UncertainCondition(ConditionBase):
    reason: VariableUncertaintyReason

@dataclass(slots=True)
class ArgumentCondition(ConditionBase):
    string:                  str
    was_quoted_or_bracketed: bool

@dataclass(slots=True)
class UnopCondition(ConditionBase):
    operator: UnaryConditionOperator
    operand:  Condition

@dataclass(slots=True)
class BinopCondition(ConditionBase):
    operator: BinaryConditionOperator
    left:     Condition
    right:    Condition


class UnaryConditionOperator(enum.Enum):
    COMMAND      =  0
    POLICY       =  1
    TARGET       =  2
    TEST         =  3
    DEFINED      =  4
    EXISTS       =  5
    IS_READABLE  =  6
    IS_WRITABLE  =  7
    IS_DIRECTORY =  8
    IS_ABSOLUTE  =  9
    NOT          = 10

class BinaryConditionOperator(enum.Enum):
    STREQUAL              =  0
    STRLESS               =  1
    STRGREATER            =  2
    STRLESS_EQUAL         =  3
    STRGREATER_EQUAL      =  4
    EQUAL                 =  5
    LESS                  =  6
    GREATER               =  7
    LESS_EQUAL            =  8
    GREATER_EQUAL         =  9
    VERSION_EQUAL         = 10
    VERSION_LESS          = 11
    VERSION_GREATER       = 12
    VERSION_LESS_EQUAL    = 13
    VERSION_GREATER_EQUAL = 14
    MATCHES               = 15
    IN_LIST               = 16
    IS_NEWER_THAN         = 17
    AND                   = 18
    OR                    = 19


type Value = CertainValue | UncertainValue

@dataclass(slots=True)
class CertainValue:
    string: str | None

@dataclass(slots=True)
class UncertainValue:
    reason: VariableUncertaintyReason


class UnparsableSetInvocException(Exception):
    def __init__(self, reason: ArgumentUncertaintyReason):
        self.reason = reason


@dataclass(slots=True)
class ArgumentUncertaintyReason:
    line_no:                     int
    variable_uncertainty_reason: VariableUncertaintyReason


type VariableUncertaintyReason = UnspecifiedVariableValueReason | UncertainVariableAssignmentReason

@dataclass(slots=True)
class VariableUncertaintyReasonBase:
    variable_name: str

@dataclass(slots=True)
class UnspecifiedVariableValueReason(VariableUncertaintyReasonBase):
    pass

@dataclass(slots=True)
class UncertainVariableAssignmentReason(VariableUncertaintyReasonBase):
    assignment_uncertainty_reason: AssignmentUncertaintyReason


type AssignmentUncertaintyReason = UncertainAssignedValueReason | UncertainAssignmentOccurrenceReason

@dataclass(slots=True)
class AssignmentUncertaintyReasonBase:
    assignment_context: log.FileContext

@dataclass(slots=True)
class UncertainAssignedValueReason(AssignmentUncertaintyReasonBase):
    variable_uncertainty_reason: VariableUncertaintyReason

@dataclass(slots=True)
class UncertainAssignmentOccurrenceReason(AssignmentUncertaintyReasonBase):
    conditional_uncertainty_reason: ConditionalUncertaintyReason


@dataclass(slots=True)
class ConditionalUncertaintyReason:
    conditional_context:         log.FileContext
    variable_uncertainty_reason: VariableUncertaintyReason


class FatalParseError(Exception):
    def __init__(self, context: log.FileContext, message: str, *args: Any):
        self.context = context
        self.message = message
        self.args    = args


type _Argument = _CertainArgument | _UncertainArgument

@dataclass(slots=True)
class _ArgumentBase:
    was_quoted_or_bracketed: bool
    line_no:                 int

@dataclass(slots=True)
class _CertainArgument(_ArgumentBase):
    string:     str
    is_derived: bool

@dataclass(slots=True)
class _UncertainArgument(_ArgumentBase):
    reason: VariableUncertaintyReason


def _expand_arguments(protoarguments: Iterable[_lcp.Protoargument],
                      variable_resolver: VariableResolver) -> list[_Argument]:
    arguments: list[_Argument] = []
    for protoarg in protoarguments:
        value, is_derived = _evaluate_string_expression(protoarg.expr, variable_resolver, protoarg.line_no)
        arg: _Argument
        if isinstance(value, CertainValue):
            string = value.string if value.string is not None else ""
            strings = []
            if protoarg.was_quoted_or_bracketed:
                strings = [string]
            else:
                strings = [s for s in _cu.list_split(string) if s]
                is_derived = True
            for string_2 in strings:
                arg = _CertainArgument(protoarg.was_quoted_or_bracketed, protoarg.line_no, string_2, is_derived)
                arguments.append(arg)
            continue
        if isinstance(value, UncertainValue):
            arg = _UncertainArgument(protoarg.was_quoted_or_bracketed, protoarg.line_no, value.reason)
            arguments.append(arg)
            continue
        assert_never(value)
    return arguments


def _evaluate_string_expression(expr: _lcp.Expr, variable_resolver: VariableResolver,
                                line_no: int) -> tuple[Value, bool]:
    if isinstance(expr, _lcp.StringExpr):
        is_derived = False
        return CertainValue(expr.string), is_derived
    if isinstance(expr, _lcp.CompositeExpr):
        string = ""
        for part in expr.parts:
            value, _ = _evaluate_string_expression(part, variable_resolver, line_no)
            if isinstance(value, CertainValue):
                if value.string is not None:
                    string += value.string
                continue
            if isinstance(value, UncertainValue):
                is_derived = False
                return value, is_derived
            assert_never(value)
        is_derived = True
        return CertainValue(string), is_derived
    if isinstance(expr, _lcp.VariableExpansionExpr):
        # FIXME: Oops, variable resolution must generally consult cache and environment
        # before giving up, or?                      
        if expr.type_ != _lcp.VariableExpansionExpr.Type.REGULAR:
            raise _UnsupportedVariableExpansion(line_no)
        value, _ = _evaluate_string_expression(expr.name_expr, variable_resolver, line_no)
        if isinstance(value, CertainValue):
            name = value.string if value.string is not None else ""
            value_2 = variable_resolver(expr.type_, name, line_no)
            is_derived = True
            return value_2, is_derived
        if isinstance(value, UncertainValue):
            is_derived = False
            return value, is_derived
        assert_never(value)
    assert_never(expr)


# FIXME: Looks like an unparsable `if()` command needs to throw a "taint bomb" because its
# operators may have had side effects.                               
class _ArgumentServer:
    def __init__(self, cmake_path: pathlib.Path, protoinvoc: _lcp.Protoinvoc, arguments: Iterable[_Argument],
                 error_handler: ErrorHandler) -> None:
        self._cmake_path    = cmake_path
        self._arguments     = list(arguments)
        self._begin         = 0
        self._end           = len(self._arguments)
        self._error_handler = error_handler
        self._command_name: str = protoinvoc.name
        self._prev_line_no: int = protoinvoc.opening_line_no
        self._end_line_no:  int = protoinvoc.closing_line_no

    def consume_any(self) -> String | None:
        def pred(value: str) -> bool:
            return True
        return self.consume_any_if(pred)

    def consume_any_if(self, pred: Callable[[str], bool]) -> String | None:
        def superpred(arg: _Argument) -> bool:
            if isinstance(arg, _CertainArgument):
                return pred(arg.string)
            if isinstance(arg, _UncertainArgument):
                reason = ArgumentUncertaintyReason(arg.line_no, arg.reason)
                raise _UncertainArgumentException(reason)
            assert_never(arg)
        arg = self._consume_if(superpred)
        if arg:
            assert isinstance(arg, _CertainArgument)
            return String(arg.string, arg.line_no)
        return None

    # FIXME: Devastating realization: Keywords are recognized even when quoted or bracketed,
    # except in conditions of control flow commands.                                                          
    def consume_keyword(self, keywords: Container[str]) -> String | None:
        def pred(arg: _Argument) -> bool:
            if isinstance(arg, _CertainArgument):
                return not arg.was_quoted_or_bracketed and arg.string in keywords
            if isinstance(arg, _UncertainArgument):
                if arg.was_quoted_or_bracketed:
                    return False
                reason = ArgumentUncertaintyReason(arg.line_no, arg.reason)
                raise _UncertainArgumentException(reason)
            assert_never(arg)
        arg = self._consume_if(pred)
        if arg:
            assert isinstance(arg, _CertainArgument)
            return String(arg.string, arg.line_no)
        return None

    def consume_not_keyword(self, keywords: Container[str]) -> String | None:
        def pred(arg: _Argument) -> bool:
            if isinstance(arg, _CertainArgument):
                return arg.was_quoted_or_bracketed or arg.string not in keywords
            if isinstance(arg, _UncertainArgument):
                reason = ArgumentUncertaintyReason(arg.line_no, arg.reason)
                raise _UncertainArgumentException(reason)
            assert_never(arg)
        arg = self._consume_if(pred)
        if arg:
            assert isinstance(arg, _CertainArgument)
            return String(arg.string, arg.line_no)
        return None

    # FIXME: Problem here because unquoted uncertain arguments are not definite in number                     
    def num_remaining(self) -> int:
        assert self._begin <= self._end
        return self._end - self._begin

    def at_end(self) -> bool:
        return self.num_remaining() == 0

    def curr_line_no(self) -> int:
        return self._prev_line_no

    def next_line_no(self) -> int:
        if self._begin < self._end:
            arg = self._arguments[self._begin]
            return arg.line_no
        return self._end_line_no

    def get_cmake_path(self) -> pathlib.Path:
        return self._cmake_path

    def get_arguments(self) -> list[_Argument]:
        return self._arguments

    def error(self, line_no: int, message: str, *args: Any) -> None:
        context = log.FileContext(self._cmake_path, line_no)
        self._error_handler(context, message, *args)

    def _consume_if(self, pred: Callable[[_Argument], bool]) -> _Argument | None:
        if self._begin == self._end:
            return None
        arg = self._arguments[self._begin]
        if not pred(arg):
            return None
        self._begin += 1
        self._prev_line_no = arg.line_no
        return arg


class _UncertainArgumentException(Exception):
    def __init__(self, reason: ArgumentUncertaintyReason) -> None:
        self.reason = reason


class _UnsupportedCommandSyntaxException(Exception):
    pass


class _UnsupportedVariableExpansion(Exception):
    def __init__(self, line_no):
        Exception.__init__(self)
        self.line_no = line_no


def _parse_condition(cmake_path: pathlib.Path, arguments: Iterable[_Argument], closing_line_no: int) -> Condition:
    def parse(conditions: list[Condition], closing_line_no: int) -> Condition:
        if len(conditions) < 1:
            return FalseCondition(closing_line_no)

        operator: Any

        # Parse for parentheses
        begin_index: int
        begin_line_no: int
        level = 0
        i = 0
        while i < len(conditions):
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                if cond.string == "(":
                    if level == 0:
                        begin_index = i
                        begin_line_no = cond.line_no
                    level += 1
                elif cond.string == ")":
                    if level == 0:
                        context = log.FileContext(cmake_path, cond.line_no)
                        raise FatalParseError(context, "Unmatched right parenhesis")
                    level -= 1
                    if level == 0:
                        end_index = i + 1
                        subconditions = conditions[begin_index + 1:end_index - 1]
                        cond_2 = parse(subconditions, cond.line_no)
                        conditions[begin_index:end_index] = [cond_2]
                        i = begin_index + 1
                        continue
            i += 1
        if level != 0:
            context = log.FileContext(cmake_path, begin_line_no)
            raise FatalParseError(context, "Unmatched left parenhesis")

        # Parse for unary operators
        i = 0
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _NONLOGICAL_UNARY_COND_OPER_MAP.get(cond.string)
                if operator is not None:
                    operand = conditions[i + 1]
                    cond_2 = UnopCondition(cond.line_no, operator, operand)
                    begin_index = i
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1


        # Parse for binary operators
        i = 1
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _NONLOGICAL_BINARY_COND_OPER_MAP.get(cond.string)
                if operator is not None:
                    left  = conditions[i - 1]
                    right = conditions[i + 1]
                    cond_2 = BinopCondition(cond.line_no, operator, left, right)
                    begin_index = i - 1
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1

        # Parse for NOT
        i = len(conditions) - 1
        while i > 0:
            cond = conditions[i - 1]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _LOGICAL_UNARY_COND_OPER_MAP.get(cond.string)
                if operator is not None:
                    operand = conditions[i]
                    cond_2 = UnopCondition(cond.line_no, operator, operand)
                    begin_index = i - 1
                    end_index   = i + 1
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index
                    continue
            i -= 1


        # Parse for AND and OR
        i = 1
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _LOGICAL_BINARY_COND_OPER_MAP.get(cond.string)
                if operator is not None:
                    left  = conditions[i - 1]
                    right = conditions[i + 1]
                    cond_2 = BinopCondition(cond.line_no, operator, left, right)
                    begin_index = i - 1
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1

        if len(conditions) > 1:
            cond = conditions[0]
            context = log.FileContext(cmake_path, cond.line_no)
            # FIXME: Find a way to format the currect list of conditions and insert them into the message    
            raise FatalParseError(context, "Unreducable condition")

        return conditions[0]

    conditions: list[Condition] = []
    for arg in arguments:
        if isinstance(arg, _CertainArgument):
            arg_2 = ArgumentCondition(arg.line_no, arg.string, arg.was_quoted_or_bracketed)
            conditions.append(arg_2)
            continue
        if isinstance(arg, _UncertainArgument):
            if not arg.was_quoted_or_bracketed:
                return UncertainCondition(arg.line_no, arg.reason)
            conditions.append(UncertainCondition(arg.line_no, arg.reason))
            continue
        assert_never(arg)
    return parse(conditions, closing_line_no)


_SCOPE_MAP = {
    "PRIVATE":   Scope.PRIVATE,
    "PUBLIC":    Scope.PUBLIC,
    "INTERFACE": Scope.INTERFACE,
}

_FILE_SET_TYPE_MAP = {
    "HEADERS":     FileSet.Type.HEADERS,
    "CXX_MODULES": FileSet.Type.CXX_MODULES,
}

_CONFIG_MAP = {
    "debug":     LinkItem.Config.DEBUG,
    "optimized": LinkItem.Config.OPTIMIZED,
    "general":   LinkItem.Config.GENERAL,
}

_MESSAGE_LEVEL_MAP = {
    "FATAL_ERROR":    MessageInvoc.Level.FATAL_ERROR,
    "SEND_ERROR":     MessageInvoc.Level.SEND_ERROR,
    "WARNING":        MessageInvoc.Level.WARNING,
    "AUTHOR_WARNING": MessageInvoc.Level.AUTHOR_WARNING,
    "DEPRECATION":    MessageInvoc.Level.DEPRECATION,
    "NOTICE":         MessageInvoc.Level.NOTICE,
    "STATUS":         MessageInvoc.Level.STATUS,
    "VERBOSE":        MessageInvoc.Level.VERBOSE,
    "DEBUG":          MessageInvoc.Level.DEBUG,
    "TRACE":          MessageInvoc.Level.TRACE,
}

_NONLOGICAL_UNARY_COND_OPER_MAP = {
    "COMMAND":      UnaryConditionOperator.COMMAND,
    "POLICY":       UnaryConditionOperator.POLICY,
    "TARGET":       UnaryConditionOperator.TARGET,
    "TEST":         UnaryConditionOperator.TEST,
    "DEFINED":      UnaryConditionOperator.DEFINED,
    "EXISTS":       UnaryConditionOperator.EXISTS,
    "IS_READABLE":  UnaryConditionOperator.IS_READABLE,
    "IS_WRITABLE":  UnaryConditionOperator.IS_WRITABLE,
    "IS_DIRECTORY": UnaryConditionOperator.IS_DIRECTORY,
    "IS_ABSOLUTE":  UnaryConditionOperator.IS_ABSOLUTE,
}

_NONLOGICAL_BINARY_COND_OPER_MAP = {
    "STREQUAL":              BinaryConditionOperator.STREQUAL,
    "STRLESS":               BinaryConditionOperator.STRLESS,
    "STRGREATER":            BinaryConditionOperator.STRGREATER,
    "STRLESS_EQUAL":         BinaryConditionOperator.STRLESS_EQUAL,
    "STRGREATER_EQUAL":      BinaryConditionOperator.STRGREATER_EQUAL,
    "EQUAL":                 BinaryConditionOperator.EQUAL,
    "LESS":                  BinaryConditionOperator.LESS,
    "GREATER":               BinaryConditionOperator.GREATER,
    "LESS_EQUAL":            BinaryConditionOperator.LESS_EQUAL,
    "GREATER_EQUAL":         BinaryConditionOperator.GREATER_EQUAL,
    "VERSION_EQUAL":         BinaryConditionOperator.VERSION_EQUAL,
    "VERSION_LESS":          BinaryConditionOperator.VERSION_LESS,
    "VERSION_GREATER":       BinaryConditionOperator.VERSION_GREATER,
    "VERSION_LESS_EQUAL":    BinaryConditionOperator.VERSION_LESS_EQUAL,
    "VERSION_GREATER_EQUAL": BinaryConditionOperator.VERSION_GREATER_EQUAL,
    "MATCHES":               BinaryConditionOperator.MATCHES,
    "IN_LIST":               BinaryConditionOperator.IN_LIST,
    "IS_NEWER_THAN":         BinaryConditionOperator.IS_NEWER_THAN,
}

_LOGICAL_UNARY_COND_OPER_MAP = {
    "NOT": UnaryConditionOperator.NOT,
}

_LOGICAL_BINARY_COND_OPER_MAP = {
    "AND": BinaryConditionOperator.AND,
    "OR":  BinaryConditionOperator.OR,
}
