from __future__ import annotations

import typing
import abc
import dataclasses
import collections
import enum
import copy
import string as _string
import re
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.version as _cve
import archon.cmake.policy as _cpo
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp
import archon.cmake.variable as _cv
import archon.cmake.argument as _ca
import archon.cmake.condition as _cc


# Process the specified CMake file (`cmake_source`) with respect to the source directory
# specified through the configuration object (`config.source_dir`). If specified paths are
# relative, they will be resolved using the specified application
# (`application.open_subfile()` or `application.resolve_path()`). `CMAKE_SOURCE_DIR` will be
# set to an absolute path even when a relative path is specified. See also
# `Config.binary_dir`.
#
# The operation carried out by this function corresponds to the configuration phase of an
# ordinary CMake run, but this function does it in a way that tracks uncertainty due to
# variables with unknown value. During an ordinary CMake run, a variable that has never been
# set or unset is taken to be definitely unset, and during an expansion its value is taken
# to be the empty string. This function takes a different approach where a variable that has
# never been set or unset is taken to have indefinite value and "unset" is one possible
# value.
#
# The purpose of this approach is to allow for extraction of particular information from a
# CMake buildsystem specification in a way that is environment independent. What information
# to extract is determined by the application. Depending on the details of a concrete
# buildsystem configuration, it may be impossible to extract particular information without
# providing definite values for certain variables. Therefore, it is possible to set initial
# values for certain variables (see `Config.initial_variables`), but the idea is that only a
# few variable will need to be set in practice, and that it can be done in a way that
# reasonably maintains environment independence. In general, the needed initial variables
# will depend on what information needs to be extracted.
#
# This function returns `True` when the processing of the CMake file succeeds, and `False`
# otherwise. It succeeds precisely when no errors are generated. An error is generated if
#
#   - a low-level syntactic error is encountered (exactly as in CMake), or
#
#   - an invocation of an unknown command or a command with uncertain definition was
#     encountered and the command is not on the command whitelist (see "Whitelists" below),
#     or
#
#   - an invocation of an unsupported command is encountered, or
#
#   - an invocation was determined to be invalid or to use unsupported syntax, or
#
#   - due to uncertainty in arguments, an invocation could neither be determined to be valid
#     nor to be invalid or to use unsupported syntax and lenient mode was not enabled (see
#     below), or
#
#   - an unsuppressed command invocation with an effect that cannot be accounted for in
#     terms of propagation of variable uncertainty had uncertain occurrence (see
#     Config.suppress_messages).
#
#
# In strict mode (the default), when the validity of a command invocation cannot be
# established due to argument uncertainty (best effort), an "uncertain argument" error is
# generated.
#
# In lenient mode (see Config.lenient_mode), when the validity of a command invocation
# cannot be established, but its effect can be accounted for in terms of propagation of
# variable uncertainty so long as the uncertain arguments can be such that no error would be
# generated under a regular CMake run, that command invocation will be assumed to be valid
# (optimistic approach) and no error is generated.
#
#
# Whitelists
# ----------
#
# By default, an invocation of an unknown command or a command with uncertain definition
# will generate an error and cause the processing of the CMake file to fail. To prevent
# this, the application can put a command on the *command whitelist*
# (`Config.command_whitelist`). By doing that, the application asserts that the command
#
#   - does not taint any function definitions, and
#
#   - does not taint any regular variables that are on the *variable whitelist*
#     (`Config.variable_whitelist`), and
#
#   - does not taint any parent scope variables, and
#
#   - has no effect other than what is covered by the tainted variables.
#
# Note that an invocation of an unknown / uncertain command that is on the whitelist will
# cause a taint of all cache variables, all environment variables, and all regular variables
# that are not on the variable whitelist.
#
def process(cmake_source: Source, application: Application, pos_resolver: PositionResolver,
            config: Config | None = None) -> bool:
    return _process(cmake_source, application, pos_resolver, config or Config())


@dataclasses.dataclass(slots=True, frozen=True)
class Source:
    input_: typing.TextIO
    path:   pathlib.Path


@dataclasses.dataclass(kw_only=True)
class Config:
    source_dir: pathlib.Path = pathlib.Path(".")
    binary_dir: pathlib.Path | None = None

    lenient_mode: bool = False

    suppress_messages: bool = False

    # If specified, must be between LOWEST_SUPPORTED_CMAKE_VERSION and
    # CMAKE_VERSION. Defaults to CMAKE_VERSION.
    cmake_version: _cve.Version | None = None

    initial_variables: dict[str, str | None] = dataclasses.field(default_factory=dict)

    command_whitelist: list[str] = dataclasses.field(default_factory=list)

    variable_whitelist: list[str] = dataclasses.field(default_factory=list)

    define_breakpoint_command: bool = False


class Application(abc.ABC):
    @abc.abstractmethod
    def open_subfile(self, path: pathlib.Path) -> typing.TextIO:
        ...

    @abc.abstractmethod
    def resolve_path(self, path: pathlib.Path) -> pathlib.Path:
        ...

    @abc.abstractmethod
    def message(self, pos: _cur.Position, occurrence_uncertainty: OccurrenceUncertainty, level: MessageLevel,
                message: str) -> None:
        ...

    @abc.abstractmethod
    def warn(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        ...

    @abc.abstractmethod
    def error(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        ...


class SimpleApplication(Application):
    def __init__(self, abs_base_dir: pathlib.Path, pos_resolver: PositionResolver, logger: _l.Logger) -> None:
        self._abs_base_dir = abs_base_dir
        self._pos_resolver = pos_resolver
        self._logger       = logger

    @typing.override
    def open_subfile(self, path: pathlib.Path) -> typing.TextIO:
        return open(self.resolve_path(path), "r")

    @typing.override
    def resolve_path(self, path: pathlib.Path) -> pathlib.Path:
        return (self._abs_base_dir / path).resolve()

    @typing.override
    def message(self, pos: _cur.Position, occurrence_uncertainty: OccurrenceUncertainty, level: MessageLevel,
                message: str) -> None:
        certainty = "Uncertain" if occurrence_uncertainty else "Certain"
        context = self._pos_resolver.resolve_file_context(pos)
        context_logger = _l.FileContextLogger(self._logger, context)
        context_logger.info("%s: Message(%s): %s", certainty, level.name, message)

    @typing.override
    def warn(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        context = self._pos_resolver.resolve_file_context(pos)
        _l.FileContextLogger(self._logger, context).warn(message, *args)

    @typing.override
    def error(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        context = self._pos_resolver.resolve_file_context(pos)
        _l.FileContextLogger(self._logger, context).error(message, *args)


type OccurrenceUncertainty = _cur.ExpansionUncertaintyReason | None


class PositionResolver:
    def __init__(self) -> None:
        self._files = list[_SourceFile]()

    def resolve_file_context(self, pos: _cur.Position) -> _l.FileContext:
        file_pos = self.resolve_file_pos(pos)
        return _l.FileContext(file_pos.path, _l.FullTextPos(file_pos.line_no, file_pos.pos_on_line))

    def resolve_file_pos(self, pos: _cur.Position) -> _tp.FilePos:
        return self._files[pos.file_index].pos_tracker.get_file_pos(pos.pos)

    def _append_file(self, file_: _SourceFile) -> int:
        file_index = len(self._files)
        self._files.append(file_)
        return file_index


class MessageLevel(enum.Enum):
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


CMAKE_VERSION                  = _cve.Version(4, 3)
LOWEST_SUPPORTED_CMAKE_VERSION = _cve.Version(3, 28)








# Clarification of principles of uncertainty handling that must be adhered to by command
# implementations:
#
# For each considered invocation:
#
#  1. If command has unknown or uncertain definition:
#
#       - If command is not on application-specified command whitelist, fail
#
#       - Taint all cache and environment variables as well as all regular variables that
#         are not on the application-specified variable whitelist
#
#       - Proceed to next invocation
#
#  2. If command is unsupported / not yet implemented, fail
#
#  3. Determine validity state of invocation (valid, invalid, unknown validity)
#
#  4. If invocation was determined to be invalid (invalid argument structure, unsupported
#     argument structure, invalid arguments), fail
#
#  5. If invocation was not determined to be valid and lenient mode is not enabled
#     (Config.lenient_mode), fail
#
#  6. If command is suppressed (message()), proceed to next invocation
#
#  7. If there is no argument or occurrence uncertainty, execute the command and proceed
#
#  8. If the command is a defined macro or function and the invocation is valid, which means
#     that the number of arguments is known and is greater than or equal to the number of
#     formal parameters, execute the command and proceed
#
#  9. If the effect of the command can be fully and reasonably accounted for in terms of
#     uncertainty propagation, propagate uncertainty and proceed
#
# 10. Consider special cases (message() can be handled in the presence of occurrence
#     uncertainty but not when there is argument uncertainty)
#
# 11. Fail
#
# Step 3: The validity state determination must be such that if an invocation is determined
# to be valid or invalid, the invocation is definitely valid or definitely invalid. On the
# other hand, the validity state determination is not required to determine validity or
# invalidity, even when it could in principle be determined given the available information
# including entanglement between argument uncertainties and occurrence uncertainty. As such,
# the validity state determination is performed in a best-effort way.
#
# It must be noted that in strict mode an error may be generated because an invocation, that
# in principle could be determined to be valid, was determined to have unknown validity. As
# such, false positives are possible in strict mode.
#
# Conversely, in lenient mode, an error may not be generated for an invocation that could in
# principle be determined to be invalid, because its validity status was determined as
# unknown. As such, false negatives are possible in lenient mode.
#
# Step 9: There is latitude for command implementations to decide what is reasonable when it
# comes to accounting for the effect of the command in terms of uncertainty propagation. For
# example, `macro("${unknown}")` could in principle be handled by tainting all command
# definitions, but this is deemed unreasonable behavior, so such an invocation will generate
# an error even in lenient mode.
#
# FIXME: What if an unknown command is on the whitelist but the invocation is invalid in
# fact. It seems like this leads to the possibility of false negatives even in strict
# mode. Is there any reasonable way to fix this? Should the whitelist idea be expanded allow
# for specification of per-command signature schema?      
#
# FIXME: It is not entirely clear whether `if()` should be regarded as a command whose
# effect can be fully accounted for in terms of uncertainty propagation, or as a special
# case.
#
def _process(cmake_source: Source, application: Application, pos_resolver: PositionResolver, config: Config) -> bool:
    errors_seen = False

    # A custom command (macro or function) that is in the current invocation path must be in
    # this map as (I, N), where I is the identifier of the custom command object
    # (`_CustomCommand`) and N is the number of times it is in the path. A custom command
    # that is not in the current invocation path should not be in this map.
    commands_in_invoc_path = dict[int, int]()

    def process_file(process_context: _ProcessContext, cmake_source: Source, is_root_dir: bool, state: _State,
                     occurrence_uncertainty: OccurrenceUncertainty, source_dir: pathlib.Path,
                     binary_dir: pathlib.Path | None) -> None:
        tracker = _tp.FilePosTracker(cmake_source.path)
        file_index = pos_resolver._append_file(_SourceFile(tracker))
        context = _InvocContext(process_context, file_index, is_root_dir, state, occurrence_uncertainty, source_dir,
                                binary_dir)
        def warning_handler(pos: int, message: str, *args: typing.Any) -> None:
            warn(file_index, pos, message, *args)
        def error_handler(pos: int, message: str, *args: typing.Any) -> None:
            error(file_index, pos, message, *args)
        for invoc in _clp.parse(cmake_source.input_, tracker, warning_handler, error_handler):
            exec_command(invoc, context)

    def exec_commands(invocations: collections.abc.Iterable[_clp.Invoc], context: _InvocContext) -> None:
        for invoc in invocations:
            exec_command(invoc, context)

    def exec_command(invoc: _clp.Invoc, context: _InvocContext) -> None:
        try:
            match invoc:
                case _clp.IfInvoc():
                    exec_if(invoc, context)
                    return
                case _clp.ForeachInvoc():
                    exec_foreach(invoc, context)
                    return
                case _clp.WhileInvoc():
                    exec_while(invoc, context)
                    return
                case _clp.MacroDefInvoc():
                    exec_macro_def(invoc, context)
                    return
                case _clp.FunctionDefInvoc():
                    exec_function_def(invoc, context)
                    return
                case _clp.BlockInvoc():
                    exec_block(invoc, context)
                    return
                case _clp.ReturnInvoc():
                    exec_return(invoc, context)
                    return
                case _clp.BreakInvoc():
                    exec_break(invoc, context)
                    return
                case _clp.ContinueInvoc():
                    exec_continue(invoc, context)
                    return
                case _clp.GenericInvoc():
                    exec_generic(invoc, context)
                    return
            typing.assert_never(invoc)
        except _UnsupportedInvocSyntaxException as e:
            if e.args:
                error(context.file_index, e.invoc.pos, "Unsupported %s() syntax: %s", e.invoc.command_name, e)
            else:
                error(context.file_index, e.invoc.pos, "Unsupported %s() syntax", e.invoc.command_name)
            return
        except _CommandExecutionFailedException as e:
            message = e.message % e.args
            error(context.file_index, e.pos, "Failed to execute %s(): %s", e.command_name, message)
            return
        except _cc.FatalParseError as e:
            message = e.message % e.args
            error(context.file_index, e.pos, "Failed to parse %s() condition: %s", e.command_name, message)
            return
        except _cc.FatalEvalError as e:
            message = e.message % e.args
            error(context.file_index, e.pos, "Failed to evaluate %s() condition: %s", e.command_name, message)
            return
        except (_ca.UncertainArgumentException, _cc.UncertaintyError, _UncertainVariableResolutionException) as e:
            position = e.reason.expansion_position
            error(position.file_index, position.pos, "Failed to invoke %s() due to expansion of %s with uncertain "
                  "value", e.reason.command_name, e.reason.get_qual_param_ref())
            if e.reason.value_uncertainty_reason:
                trace_value_uncertainty_causes(e.reason.value_uncertainty_reason)
            return
        except _UncertainPolicyException as e:
            definition = _cpo.get_definition(e.policy)
            error(context.file_index, e.pos, "Failed to execute %s() due to uncertain status of policy %s",
                  e.requesting_command, definition.name)
            trace_value_uncertainty_causes(e.cause)
            return

    def exec_if(invoc: _clp.IfInvoc, context: _InvocContext) -> None:
        def exec_level(subinvoc: _clp.IfInvoc | _clp.IfBranch, context: _InvocContext, next_elseif: int) -> None:
            def exec_else(context: _InvocContext) -> None:
                if next_elseif < len(invoc.elseif_branches):
                    exec_level(invoc.elseif_branches[next_elseif], context, next_elseif + 1)
                    return
                assert next_elseif == len(invoc.elseif_branches)
                if invoc.else_branch:
                    # CMake completely ignores any arguments passed to `else()`
                    exec_commands(invoc.else_branch.children, context)
            result = evaluate_condition(subinvoc, context)
            match result:
                case _cc.FalseResult():
                    exec_else(context)
                    return
                case _cc.TrueResult():
                    # CMake has short-circuiting evaluation behavior across if-branches,
                    # meaning that as soon as a branch condition evaluates to true, the
                    # remaining branch conditions are not evaluated.
                    exec_commands(subinvoc.children, context)
                    return
                case _cc.UncertainResult():
                    occurrence_uncertainty = context.occurrence_uncertainty or result.reason
                    if_state = _OccurrenceUncertaintyOverlayState(context.state, result.reason)
                    if_context = dataclasses.replace(context, state=if_state,
                                                     occurrence_uncertainty=occurrence_uncertainty)
                    exec_commands(subinvoc.children, if_context)
                    else_state = _OccurrenceUncertaintyOverlayState(context.state, result.reason)
                    else_context = dataclasses.replace(context, state=else_state,
                                                       occurrence_uncertainty=occurrence_uncertainty)
                    exec_else(else_context)
                    # Must prune taints in both substates before pushing any taints to the
                    # parent state. This is because the pruning operation needs access to
                    # the original untouched parent state.
                    if_state.prune_taints()
                    else_state.prune_taints()
                    if_state.cross_prune_taints(else_state)
                    if_state.push_taints()
                    else_state.push_taints()
                    return
            typing.assert_never(result)
        exec_level(invoc, context, 0)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        def iterate_list(loop_var: str, items: list[str]) -> None:
            orig_value = context.state.get_regular_variable(loop_var)
            for item in items:
                set_regular_variable(loop_var, item, invoc, context)
                exec_commands(invoc.children, context)
            match orig_value:
                case _cv.CertainValue():
                    set_regular_variable(loop_var, orig_value.string, invoc, context)
                case _cv.UncertainValue():
                    context.state.set_regular_variable(loop_var, orig_value)
                case _:
                    typing.assert_never(orig_value)
        server = create_argument_server(invoc, context)
        i = server.find_keyword({"IN"})
        if i == -1:
            loop_var = server.consume()
            if not loop_var:
                error(context.file_index, server.next_pos, "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if server.consume_keyword({"RANGE"}):
                raise _UnsupportedInvocSyntaxException(invoc, "RANGE keyword") from None
            items = []
            while True:
                arg = server.consume()
                if not arg:
                    break
                items.append(arg.string.string)
            iterate_list(loop_var.string.string, items)
            return
        loop_vars = []
        for _ in range(i):
            arg = server.consume()
            assert arg
            loop_vars.append(arg.string.string)
        arg = server.consume_keyword({"IN"})
        assert arg
        arg = server.consume()
        if not arg:
            return
        if arg.string.string in {"LISTS", "ITEMS"}:
            if not loop_vars:
                error(context.file_index, server.next_pos, "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if len(loop_vars) > 1:
                error(context.file_index, server.next_pos, "Too many loop variables in %s() invocation",
                      invoc.command_name)
                return
            items = []
            mode = arg.string.string
            while True:
                arg = server.consume_keyword({"LISTS", "ITEMS", "ZIP_LISTS"})
                if arg:
                    if arg.string.string == "ZIP_LISTS":
                        error(context.file_index, arg.pos, "ZIP_LISTS cannot be used with LISTS or ITEMS in "
                              "%s() invocation", invoc.command_name)
                        return
                    mode = arg.string.string
                arg = server.consume()
                if not arg:
                    break
                if mode == "ITEMS":
                    items.append(arg.string.string)
                    continue
                if mode == "LISTS":
                    var_name = arg.string.string
                    string = resolve_certain_variable(_cu.ResolutionType.GENERAL, var_name, arg.pos, invoc, context)
                    items += _cu.unescaping_list_split(string)
                    continue
                assert False
            iterate_list(loop_vars[0], items)
            return
        if arg.string.string == "ZIP_LISTS":
            raise _UnsupportedInvocSyntaxException(invoc, "ZIP_LISTS keyword") from None
        error(context.file_index, arg.pos, "Unrecognized keyword (%s) after IN in %s() invocation",
              _b.quote(arg.string.string), invoc.command_name)

    def exec_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_macro_def(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing macro name in %s() invocation", invoc.command_name)
            return
        name = arg.string.string
        name_pos = arg.pos
        name_cf = name.casefold()
        if _clp.is_flow_control_command(name_cf):
            error(context.file_index, name_pos, "Failed to define macro %s(): Built-in flow control commands cannot "
                  "be overridden", name)
            return
        parameters = []
        while True:
            arg = server.consume()
            if not arg:
                break
            parameters.append(arg.string.string)
        definition_position = _cur.Position(context.file_index, invoc.pos)
        orig_command = context.state.get_command(name_cf)
        orig_command_2: _DefinedCommand
        match orig_command:
            case _BuiltInCommand() | _CustomCommand():
                orig_command_2 = _CertainDefinedCommand(orig_command, invoc.command_name, definition_position)
            case _UncertainCommand():
                orig_command_2 = orig_command
        context.state.set_command("_" + name_cf, orig_command_2)
        command = _CustomCommand(_CustomCommand.Type.MACRO, name, parameters, invoc.children, definition_position)
        command_2 = _CertainDefinedCommand(command, invoc.command_name, definition_position)
        context.state.set_command(name_cf, command_2)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_function_def(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing function name in %s() invocation", invoc.command_name)
            return
        name = arg.string.string
        name_pos = arg.pos
        name_cf = name.casefold()
        if _clp.is_flow_control_command(name_cf):
            error(context.file_index, name_pos, "Failed to define function %s(): Built-in flow control commands "
                  "cannot be overridden", name)
            return
        parameters = []
        while True:
            arg = server.consume()
            if not arg:
                break
            parameters.append(arg.string.string)
        definition_position = _cur.Position(context.file_index, invoc.pos)
        orig_command = context.state.get_command(name_cf)
        orig_command_2: _DefinedCommand
        match orig_command:
            case _BuiltInCommand() | _CustomCommand():
                orig_command_2 = _CertainDefinedCommand(orig_command, invoc.command_name, definition_position)
            case _UncertainCommand():
                orig_command_2 = orig_command
        context.state.set_command("_" + name_cf, orig_command_2)
        command = _CustomCommand(_CustomCommand.Type.FUNCTION, name, parameters, invoc.children, definition_position)
        command_2 = _CertainDefinedCommand(command, invoc.command_name, definition_position)
        context.state.set_command(name_cf, command_2)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"SCOPE_FOR"}):
            raise _UnsupportedInvocSyntaxException(invoc, "SCOPE_FOR keyword") from None
        if not server.at_end:
            error(context.file_index, server.next_pos, "Unrecognized first argument in %s() invocation",
                  invoc.command_name)
            return
        state = _SubscopeState(context.state)
        exec_commands(invoc.children, context.with_state(state))
        exec_empty_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_return(invoc: _clp.ReturnInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if not server.at_end:
            raise _UnsupportedInvocSyntaxException(invoc) from None
        assert False        

    def exec_break(invoc: _clp.BreakInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_continue(invoc: _clp.ContinueInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_generic(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        command = context.state.get_command(invoc.command_name_cf)
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(context.file_index, invoc.pos, "Invocation of unsupported command %s()",
                              invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        exec_set(invoc, context)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        exec_unset(invoc, context)
                        return
                    case _BuiltInCommand.Which.STRING:
                        exec_string(invoc, context)
                        return
                    case _BuiltInCommand.Which.LIST:
                        exec_list(invoc, context)
                        return
                    case _BuiltInCommand.Which.MESSAGE:
                        exec_message(invoc, context)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        exec_include(invoc, context)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        exec_add_subdirectory(invoc, context)
                        return
                    case _BuiltInCommand.Which.CMAKE_MINIMUM_REQUIRED:
                        exec_cmake_minimum_required(invoc, context)
                        return
                    case _BuiltInCommand.Which.PROJECT:
                        exec_project(invoc, context)
                        return
                    case _BuiltInCommand.Which.BREAKPOINT:
                        breakpoint()
                        return
                typing.assert_never(which)
            case _CustomCommand():
                command_id = id(command)
                if errors_seen:
                    # Skip invocation if an error has occurred and this is a recursive
                    # invocation. This is in order to avoid unintended infinite recursion.
                    if command_id in commands_in_invoc_path:
                        return
                commands_in_invoc_path[command_id] = commands_in_invoc_path.get(command_id, 0) + 1
                try:
                    arguments, card_uncertainty = expand_arguments(invoc, context)
                    def check(qual: str) -> bool:
                        # Uncertainty in argument cardinality cannot be tolerated, even in
                        # lenient mode
                        if card_uncertainty:
                            error(context.file_index, invoc.pos, "Uncertain number of arguments in invocation of %s "
                                  "%s()", qual, invoc.command_name)
                            return False
                        if len(arguments) < len(command.parameters):
                            error(context.file_index, invoc.rparen_pos, "Too few arguments in invocation of %s %s()",
                                  qual, invoc.command_name)
                            position = command.definition_position
                            error(position.file_index, position.pos, "Definition of %s %s()", qual, invoc.command_name)
                            return False
                        return True
                    match command.type_:
                        case _CustomCommand.Type.MACRO:
                            if not check("macro"):
                                return
                            substitutions = _get_macro_substitutions(command.parameters, arguments)
                            for subinvoc in command.invocations:
                                subinvoc_2 = _macro_substitute_invoc(subinvoc, context, substitutions)
                                exec_command(subinvoc_2, context)
                            return
                        case _CustomCommand.Type.FUNCTION:
                            if not check("function"):
                                return
                            state = _SubscopeState(context.state)
                            _assign_function_arguments(command, arguments, state, invoc, context)
                            exec_commands(command.invocations, context.with_state(state))
                            return
                    typing.assert_never(command.type_)
                finally:
                    n = commands_in_invoc_path[command_id]
                    assert n > 0
                    if n == 1:
                        del commands_in_invoc_path[command_id]
                    else:
                        commands_in_invoc_path[command_id] = n - 1
            case _UncertainCommand(reason):
                error(context.file_index, invoc.pos, "Invocation failed due to uncertain definition of %s()",
                      invoc.command_name)
                if reason:
                    position = reason.definition_position
                    error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                          reason.defining_command_name)
                    trace_expansion_uncertainty_causes(reason.occurrence_uncertainty_reason)
                return
        typing.assert_never(command)

    def exec_closing_invoc(invoc: _clp.ClosingInvoc, opening_invoc: _clp.Invoc, context: _InvocContext) -> None:
        # For most block commands (all other than block()), CMake ignores arguments in a
        # closing invocation but generates a warning unless the closing invocation is either
        # empty or matches the corresponding opening invocation proto-argument for
        # proto-argument. In the case of endif(), if() is the corresponding opening
        # invocation, even if there are elseif() and/or else() invocations in between.
        if not invoc.arguments:
            return
        opening_args = [a.orig_text for a in opening_invoc.arguments]
        closing_args = [a.orig_text for a in invoc.arguments]
        if opening_args != closing_args:
            opening_position = _cur.Position(context.file_index, opening_invoc.pos)
            opening_line_no = pos_resolver.resolve_file_pos(opening_position).line_no
            warn(context.file_index, invoc.pos, "Closing invocation, %s(), has mismatching arguments (opening "
                 "invocation is on line %s)", invoc.command_name, opening_line_no)

    def exec_empty_closing_invoc(invoc: _clp.ClosingInvoc, opening_invoc: _clp.Invoc, context: _InvocContext) -> None:
        # For block() specifically, CMake requires that the closing invocation has no
        # arguments.
        if not invoc.arguments:
            return
        pos = invoc.arguments[0].pos
        opening_position = _cur.Position(context.file_index, opening_invoc.pos)
        opening_line_no = pos_resolver.resolve_file_pos(opening_position).line_no
        warn(context.file_index, pos, "Arguments in closing invocation %s() (opening invocation is on line %s)",
             invoc.command_name, opening_line_no)

    def exec_set(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing variable name in %s() invocation", invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(arg.string.string)
        var_ref_pos = arg.pos
        var_name = var_ref.variable_name
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                values = list[str]()
                parent_scope = False
                try:
                    while True:
                        arg = server.consume()
                        if not arg:
                            break
                        if arg.string.string == "CACHE":
                            raise _UnsupportedInvocSyntaxException(invoc, "CACHE keyword") from None
                        if server.at_end and arg.string.string == "PARENT_SCOPE":
                            parent_scope = True
                            break
                        values.append(arg.string.string)
                except _ca.UncertainArgumentException as e:
                    taint_regular_variable(var_name, e.reason, context)
                    if not context.state.is_root_scope():
                        taint_parent_scope_variable(var_name, e.reason, context)
                    return
                value = _cu.nonescaping_list_join(values)
                if not parent_scope:
                    set_regular_variable(var_name, value, invoc, context)
                    return
                set_parent_scope_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.CACHE:
                type_: str | None = None
                force = False
                while True:
                    if server.consume_keyword({"TYPE"}):
                        arg = server.consume()
                        if not arg:
                            error(context.file_index, server.next_pos, "Missing <type> argument after TYPE keyword in "
                                  "%s() invocation", invoc.command_name)
                            return
                        type_ = arg.string.string
                        continue
                    if server.consume_keyword({"HELP"}):
                        raise _UnsupportedInvocSyntaxException(invoc, "HELP keyword") from None
                    if server.consume_keyword({"FORCE"}):
                        force = True
                        continue
                    if server.consume_keyword({"VALUE"}):
                        break
                    arg = server.consume()
                    if arg:
                        error(context.file_index, arg.pos, "Unrecognized argument (%s) before VALUE keyword in %s() "
                              "invocation", _b.quote(arg.string.string), invoc.command_name)
                        return
                    error(context.file_index, server.next_pos, "Missing VALUE keyword in %s() invocation",
                          invoc.command_name)
                    return
                if type_ is not None and type_ != "STRING":
                    raise _UnsupportedInvocSyntaxException(invoc, "Non-STRING type") from None
                if not force:
                    raise _UnsupportedInvocSyntaxException(invoc, "No FORCE keyword") from None
                # CMake accepts an empty list of values. If the list of values is empty, the
                # variable will be set to the empty string. It will not be unset (contrast
                # this with regular variables).
                values = list[str]()
                try:
                    while True:
                        arg = server.consume()
                        if not arg:
                            break
                        values.append(arg.string.string)
                except _ca.UncertainArgumentException as e:
                    taint_cache_variable(var_name, e.reason, context)
                    return
                value = _cu.nonescaping_list_join(values) or ""
                set_cache_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.ENV:
                # CMake accepts zero values. If zero values are specified, the effect is the
                # same as if a single empty string is specified. When an empty string is
                # specified and the environment variable is undefined, the environment
                # variable remains undefined. Otherwise the environment variable is set to
                # the specified string. Arguments beyond a single value are accepted by
                # CMake, but generate a warning.
                value = ""
                try:
                    arg = server.consume()
                    if arg:
                        value = arg.string.string
                        if not server.at_end:
                            warn(context.file_index, server.next_pos, "Extraneous unused arguments in %s() invocation",
                                 invoc.command_name)
                except _ca.UncertainArgumentException as e:
                    taint_env_variable(var_name, e.reason, context)
                    return
                if not value:
                    # Value is empty or absent
                    try:
                        orig_value = resolve_certain_variable(_cu.ResolutionType.ENV, var_name, var_ref_pos, invoc,
                                                              context)
                    except _UncertainVariableResolutionException as e:
                        # If the target variable was tainted, it remains tainted. Nothing
                        # further to do.
                        return
                    if not orig_value:
                        return
                set_env_variable(var_name, value, invoc, context)
                return
        typing.assert_never(var_ref.resolution_type)

    def exec_unset(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing variable name in %s() invocation", invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(arg.string.string)
        var_name = var_ref.variable_name
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                parent_scope = False
                try:
                    arg = server.consume()
                    if arg:
                        if arg.string.string == "CACHE":
                            raise _UnsupportedInvocSyntaxException(invoc, "CACHE keyword") from None
                        if server.at_end and arg.string.string == "PARENT_SCOPE":
                            parent_scope = True
                        else:
                            error(context.file_index, server.next_pos, "Too many arguments in %s() invocation",
                                  invoc.command_name)
                except _ca.UncertainArgumentException as e:
                    # FIXME: If the variable is unset already, then no taint is needed
                    # (applies separately to parent scope)
                    taint_regular_variable(var_name, e.reason, context)
                    if not context.state.is_root_scope():
                        taint_parent_scope_variable(var_name, e.reason, context)
                    return
                value = None
                if not parent_scope:
                    set_regular_variable(var_name, value, invoc, context)
                    return
                set_parent_scope_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.CACHE:
                # CMake bizarrely accepts one extra unused argument without even issuing a
                # warning. If more than one extra argument is passed, CMake errors.
                server.consume()
                if not server.at_end:
                    error(context.file_index, server.next_pos, "Too many arguments in %s(CACHE{}) invocation",
                          invoc.command_name)
                value = None
                set_cache_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.ENV:
                # CMake bizarrely accepts one extra unused argument without even issuing a
                # warning. If more than one extra argument is passed, CMake errors.
                server.consume()
                if not server.at_end:
                    error(context.file_index, server.next_pos, "Too many arguments in %s(ENV{}) invocation",
                          invoc.command_name)
                value = None
                set_env_variable(var_name, value, invoc, context)
                return
        typing.assert_never(var_ref.resolution_type)

    def exec_string(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Too few arguments in %s() invocation", invoc.command_name)
            return
        func = arg.string.string
        if func in {"APPEND", "PREPEND"}:
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            var_name_pos = arg.pos
            # In CMake, these variable modifying operations load the original value either
            # from a regular variable or from a cache variable if a regular variable does
            # not exist, and, regardless of where the value was loaded from, the result is
            # stored in a regular variable.
            try:
                orig_string = resolve_certain_variable(_cu.ResolutionType.GENERAL, var_name, var_name_pos, invoc,
                                                       context)
            except _UncertainVariableResolutionException as e:
                # If the target variable was tainted, it remains tainted. Nothing further to
                # do.
                return
            elements = list[str]()
            try:
                while True:
                    arg = server.consume()
                    if not arg:
                        break
                    elements.append(arg.string.string)
            except _ca.UncertainArgumentException as e:
                taint_regular_variable(var_name, e.reason, context)
                return
            # In CMake, if no elements are appended or prepended, the variable is
            # unchanged. If it was unset, it remains unset.
            if not elements:
                return
            match func:
                case "APPEND":
                    string = (orig_string or "") + "".join(elements)
                case "PREPEND":
                    string = "".join(elements) + (orig_string or "")
                case _:
                    assert False
            set_regular_variable(var_name, string, invoc, context)
            return
        if func == "CONCAT":
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            var_name_pos = arg.pos
            elements = list[str]()
            try:
                while True:
                    arg = server.consume()
                    if not arg:
                        break
                    elements.append(arg.string.string)
            except _ca.UncertainArgumentException as e:
                taint_regular_variable(var_name, e.reason, context)
                return
            # In CMake, if no elements are concatenated, the variable is set to the empty string.
            string = "".join(elements)
            set_regular_variable(var_name, string, invoc, context)
            return
        if func in {"TOLOWER", "TOUPPER"}:
            uncertainty: _cur.ExpansionUncertaintyReason | None = None
            try:
                arg = server.consume()
                if not arg:
                    error(context.file_index, server.next_pos, "Missing <string> argument in %s(%s) invocation",
                          invoc.command_name, func)
                    return
                string = arg.string.string
            except _ca.UncertainArgumentException as e:
                if e.was_bare:
                    raise
                uncertainty = e.reason
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            if uncertainty:
                taint_regular_variable(var_name, uncertainty, context)
                return
            # NOTE: CMake completely ignores additional arguments for these signatures
            match func:
                case "TOLOWER":
                    string_2 = string.translate(_ASCII_LOWER_MAP)
                case "TOUPPER":
                    string_2 = string.translate(_ASCII_UPPER_MAP)
                case _:
                    assert False
            set_regular_variable(var_name, string_2, invoc, context)
            return
        raise _UnsupportedInvocSyntaxException(invoc) from None

    def exec_list(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Too few arguments in %s() invocation", invoc.command_name)
            return
        func = arg.string.string
        if func == "GET":
            arg = server.consume_last()
            if not arg:
                error(context.file_index, server.next_pos, "Missing final <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            # Validity cannot be establish unless all arguments have certain values (because
            # indexes need to be within range), therefore, in strict mode, any uncertainty
            # generates an error. In lenient mode, however, list contents can be unknown,
            # the number of indexes can be unknown as long as there is at least one, and the
            # actual index values can be unknown.
            uncertainty: _cur.ExpansionUncertaintyReason | None = None
            have_list_name = False
            try:
                arg = server.consume()
                if not arg:
                    error(context.file_index, server.next_pos, "Missing <list> argument in %s(%s) invocation",
                          invoc.command_name, func)
                    return
                list_name = arg.string.string
                list_name_pos = arg.pos
                have_list_name = True
            except _ca.UncertainArgumentException as e:
                if not context.process.lenient_mode or e.was_bare:
                    raise
                if not uncertainty:
                    uncertainty = e.reason
            have_list_value = False
            if have_list_name:
                try:
                    string = resolve_certain_variable(_cu.ResolutionType.GENERAL, list_name, list_name_pos, invoc,
                                                      context)
                    elements = _cu.unescaping_list_split(string)
                    n = len(elements)
                    have_list_value = True
                except _UncertainVariableResolutionException as e:
                    if not context.process.lenient_mode:
                        raise
                    if not uncertainty:
                        uncertainty = e.reason
            indexes = list[int]()
            definitely_no_indexes = True
            while True:
                try:
                    arg = server.consume()
                    if not arg:
                        break
                    value = _cu.parse_index_arg(arg.string.string)
                    if value is None:
                        error(context.file_index, arg.pos, "Invalid index (%s) in %s(%s) invocation",
                              _b.quote(arg.string.string), invoc.command_name, func)
                        return
                    if have_list_value and not (-n <= value < n):
                        error(context.file_index, arg.pos, "Index (%s) is out of range in %s(%s) invocation", value,
                              invoc.command_name, func)
                        return
                    definitely_no_indexes = False
                    indexes.append(value)
                except _ca.UncertainArgumentException as e:
                    if not context.process.lenient_mode:
                        raise
                    definitely_no_indexes = False
                    if not uncertainty:
                        uncertainty = e.reason
            if definitely_no_indexes:
                error(context.file_index, server.next_pos, "Too few indexes in %s(%s) invocation", invoc.command_name,
                      func)
                return
            if uncertainty:
                assert context.process.lenient_mode
                taint_regular_variable(var_name, uncertainty, context)
                return
            assert have_list_value
            elements_2 = list[str]()
            for index in indexes:
                elements_2.append(elements[index])
            assert elements_2
            string_2 = _cu.nonescaping_list_join(elements_2)
            set_regular_variable(var_name, string_2, invoc, context)
            return
        if func in {"APPEND", "PREPEND"}:
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            var_name_pos = arg.pos
            # In CMake, these variable modifying operations load the original value either
            # from a regular variable or from a cache variable if a regular variable does
            # not exist, and, regardless of where the value was loaded from, the result is
            # stored in a regular variable.
            try:
                orig_string = resolve_certain_variable(_cu.ResolutionType.GENERAL, var_name, var_name_pos, invoc,
                                                       context)
            except _UncertainVariableResolutionException as e:
                # If the target variable was tainted, it remains tainted. Nothing further to
                # do.
                return
            elements = list[str]()
            try:
                while True:
                    arg = server.consume()
                    if not arg:
                        break
                    elements.append(arg.string.string)
            except _ca.UncertainArgumentException as e:
                taint_regular_variable(var_name, e.reason, context)
                return
            # In CMake, if no elements are appended or prepended, the original value is
            # unchanged. If it was unset, it remains unset.
            if not elements:
                return
            # In CMake, when at least one element is appended or prepended and the list
            # variable was unset or its original value was the empty string, the new list
            # value becomes the semicolon-join of the appended / prepended
            # elements. Otherwise, when at least one element is appended, the result is the
            # original value plus semicolon plus the semicolon-join of the appended
            # elements. Likewise, when at least one element is prepended, the result is he
            # semicolon-join of the prepended elements plus semicolon plus the original
            # value.
            string_1 = _cu.nonescaping_list_join(elements)
            assert string_1 is not None
            match func:
                case "APPEND":
                    string_2 = (orig_string + ";" + string_1 if orig_string else string_1)
                case "PREPEND":
                    string_2 = (string_1 + ";" + orig_string if orig_string else string_1)
                case _:
                    assert False
            set_regular_variable(var_name, string_2, invoc, context)
            return
        raise _UnsupportedInvocSyntaxException(invoc) from None

    def exec_message(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedInvocSyntaxException(invoc, "Non-NOTICE level") from None
        level = MessageLevel.NOTICE
        arg = server.consume_keyword(_MESSAGE_LEVEL_MAP.keys())
        if arg:
            level = _MESSAGE_LEVEL_MAP[arg.string.string]
        if context.process.suppress_messages:
            return
        message = ""
        while True:
            arg = server.consume()
            if not arg:
                break
            message += arg.string.string
        position = _cur.Position(context.file_index, invoc.pos)
        application.message(position, context.occurrence_uncertainty, level, message)

    def exec_include(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing file or module in %s()", invoc.command_name)
            return
        file_or_module = arg.string.string
        file_or_module_pos = arg.pos
        if not re.fullmatch(r".*\.cmake", file_or_module):
            raise _UnsupportedInvocSyntaxException(invoc, "Non-path argument") from None
        if not server.at_end:
            raise _UnsupportedInvocSyntaxException(invoc) from None
        cmake_path = context.source_dir / file_or_module
        try:
            with context.open_subfile(cmake_path) as file_:
                cmake_source = Source(file_, cmake_path)
                process_file(context.process, cmake_source, context.is_root_dir, context.state,
                             context.occurrence_uncertainty, context.source_dir, context.binary_dir)
        except FileNotFoundError as e:
            error(context.file_index, file_or_module_pos, "Failed to include %s (%s): %s", _b.quote(file_or_module),
                  _b.quote(str(cmake_path)), e.strerror)

    def exec_add_subdirectory(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing source directory in %s()", invoc.command_name)
            return
        source_dir = arg.string.string
        source_dir_pos = arg.pos
        if not server.at_end:
            raise _UnsupportedInvocSyntaxException(invoc) from None
        binary_dir = source_dir
        source_dir_2 = context.source_dir / source_dir
        binary_dir_2: pathlib.Path | None = None
        if context.binary_dir is not None:
            binary_dir_2 = context.binary_dir / binary_dir
        initial_variables = dict[str, str | None]()
        initial_variables["CMAKE_CURRENT_SOURCE_DIR"] = str(context.resolve_path(source_dir_2))
        if binary_dir_2 is not None:
            initial_variables["CMAKE_CURRENT_BINARY_DIR"] = str(context.resolve_path(binary_dir_2))
        state = _SubscopeState(context.state, initial_variables)
        cmake_path = source_dir_2 / "CMakeLists.txt"
        try:
            with context.open_subfile(cmake_path) as file_:
                cmake_source = Source(file_, cmake_path)
                is_root_dir = False
                process_file(context.process, cmake_source, is_root_dir, state, context.occurrence_uncertainty,
                             source_dir_2, binary_dir_2)
        except FileNotFoundError as e:
            error(context.file_index, source_dir_pos, "Failed to add subdirectory %s (%s): %s", _b.quote(source_dir),
                  _b.quote(str(cmake_path)), e.strerror)

    def exec_cmake_minimum_required(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        # In the case of cmake_minimum_required(), CMake allows for keywords to occur in any
        # order and for keywords to recur any number of times. Also, keyword status takes
        # precedence over value status meaning that a keyword cannot occur as a value of
        # another keyword.
        class Keyword(enum.Enum):
            VERSION     = 0
            FATAL_ERROR = 1
        keywords = set(Keyword.__members__.keys())
        version: _tp.PosMappedString | None = None
        while True:
            arg = server.consume()
            if not arg:
                break
            keyword = Keyword.__members__.get(arg.string.string)
            match keyword:
                case Keyword.VERSION:
                    # Replicating very quirky CMake behavior here: Error if VERSION keyword is
                    # not followed by a version argument unless the next argument is another
                    # VERSION keyword
                    while True:
                        if not server.consume_keyword({"VERSION"}):
                            break
                    arg = server.consume_not_keyword(keywords)
                    if not arg:
                        error(context.file_index, server.next_pos, "Missing version after VERSION keyword in %s() "
                              "invocation", invoc.command_name)
                        return
                    version = arg.string
                    continue
                case Keyword.FATAL_ERROR:
                    # Superfluous since CMake 2.6
                    continue
                case None:
                    error(context.file_index, arg.pos, "Unexpected argument (%s) in %s() invocation",
                          _b.quote(arg.string.string), invoc.command_name)
                    return
            typing.assert_never(keyword)
        # In CMake, if a cmake_minimum_required() invocation specifies no version, the
        # invocation has no effect at all.
        if version is None:
            return
        ellipsis = "..."
        ellipsis_pos = version.string.find(ellipsis)
        min_version_string = version.string if ellipsis_pos < 0 else version.string[:ellipsis_pos]
        try:
            min_version = _cve.parse(min_version_string)
        except ValueError:
            ref_pos = version.begin_ref_pos
            error(context.file_index, ref_pos, "Unsupported version syntax in specified minimum version (%s) in %s() "
                  "invocation", _b.quote(min_version_string), invoc.command_name)
            return
        if ellipsis_pos < 0:
            max_policy_version_pos    = 0
            max_policy_version_string = min_version_string
            max_policy_version        = min_version
        else:
            max_policy_version_pos    = ellipsis_pos + len(ellipsis)
            max_policy_version_string = version.string[max_policy_version_pos:]
            try:
                max_policy_version = _cve.parse(max_policy_version_string)
            except ValueError:
                ref_pos = version.ref_pos(max_policy_version_pos)
                error(context.file_index, ref_pos, "Unsupported version syntax in specified maximum policy version "
                      "(%s) in %s() invocation", _b.quote(max_policy_version_string), invoc.command_name)
                return
            if max_policy_version < min_version:
                ref_pos = version.begin_ref_pos
                error(context.file_index, ref_pos, "Specified maximum policy version (%s) is lower than specified "
                      "minimum version (%s) in %s() invocation", max_policy_version_string, min_version_string,
                      invoc.command_name)
                return
        if min_version > context.process.cmake_version:
            ref_pos = version.begin_ref_pos
            error(context.file_index, ref_pos, "Specified minimum version (%s) is higher than highest supported CMake "
                  "version (%s) in %s() invocation", min_version_string, context.process.cmake_version,
                  invoc.command_name)
            return
        if max_policy_version < LOWEST_SUPPORTED_CMAKE_VERSION:
            ref_pos = version.ref_pos(max_policy_version_pos)
            error(context.file_index, ref_pos, "Specified maximum policy version (%s) is lower than lowest supported "
                  "CMake version (%s) in %s() invocation", max_policy_version_string, LOWEST_SUPPORTED_CMAKE_VERSION,
                  invoc.command_name)
            return
        set_regular_variable("CMAKE_MINIMUM_REQUIRED_VERSION", min_version_string, invoc, context)
        policy_version = min(max_policy_version, context.process.cmake_version)
        set_policy_version(policy_version, invoc, context)

    def exec_project(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        # CMake 4.3 warns if project() is invoked from the root directory but without a
        # preceding cmake_minimum_required() invocation. CMake checks this by examining the
        # variable CMAKE_MINIMUM_REQUIRED_VERSION.
        if context.is_root_dir:
            value = resolve_certain_variable(_cu.ResolutionType.GENERAL, "CMAKE_MINIMUM_REQUIRED_VERSION", invoc.pos,
                                             invoc, context, undefined_is_certain=True)
            if value is None:
                warn(context.file_index, invoc.pos, "Variable CMAKE_MINIMUM_REQUIRED_VERSION not set prior to %s() "
                     "invocation", invoc.command_name)

        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing project name in %s() invocation", invoc.command_name)
            return
        name = arg.string.string
        # In the case of the keyword-based project() signature, CMake allows for keywords to
        # occur in any order but does not allow for keywords to recur. Keyword status takes
        # precedence over value status meaning that a keyword cannot occur as a value of
        # another keyword. For keywords other than LANGUAGES, a missing value is tolerated
        # but generates a warning.
        class Keyword(enum.Enum):
            VERSION        = 0
            COMPAT_VERSION = 1
            SPDX_LICENSE   = 2
            DESCRIPTION    = 3
            HOMEPAGE_URL   = 4
            LANGUAGES      = 5
        keywords = set(Keyword.__members__.keys())
        version:      str | None = None
        description:  str | None = None
        homepage_url: str | None = None
        languages = list[str]()
        no_default_languages = False
        if not server.has_keyword(keywords):
            while True:
                arg = server.consume()
                if not arg:
                    break
                if arg.string.string == "NONE":
                    no_default_languages = True
                    continue
                languages.append(arg.string.string)
        else:
            # Replicating quirky CMake behavior: A stray value argument anywhere is treated
            # as a language argument. When a stray value argument is encountered, if there
            # is no LANGUAGES keyword, an error is generated. When the LANGUAGES keyword is
            # present, a warning is generated instead.
            has_languages_keyword = server.has_keyword({"LANGUAGES"})
            if has_languages_keyword:
                no_default_languages = True
            languages_keyword_seen = False
            while True:
                arg = server.consume()
                if not arg:
                    break
                keyword = Keyword.__members__.get(arg.string.string)
                match keyword:
                    case Keyword.VERSION:
                        if version is not None:
                            error(context.file_index, arg.pos, "More than one VERSION keyword in %s() invocation",
                                  invoc.command_name)
                            return
                        arg = server.consume_not_keyword(keywords)
                        if arg:
                            version = arg.string.string
                            # CMake accepts an empty string or a version with up to 4 purely
                            # numeric components
                            m = re.fullmatch(r"(?:(\d+)(?:\.(\d+)(?:\.(\d+)(?:\.(\d+))?)?)?)?", version, re.ASCII)
                            if not m:
                                error(context.file_index, arg.pos, "Invalid version syntax (%s) in %s() invocation",
                                      _b.quote(version), invoc.command_name)
                                return
                            version_major = m.group(1) or ""
                            version_minor = m.group(2) or ""
                            version_patch = m.group(3) or ""
                            version_tweak = m.group(4) or ""
                        else:
                            warn(context.file_index, server.next_pos, "Missing version after VERSION keyword in %s() "
                                 "invocation", invoc.command_name)
                            version       = ""
                            version_major = ""
                            version_minor = ""
                            version_patch = ""
                            version_tweak = ""
                        continue
                    case Keyword.COMPAT_VERSION:
                        raise _UnsupportedInvocSyntaxException(invoc, "COMPAT_VERSION keyword") from None
                    case Keyword.SPDX_LICENSE:
                        raise _UnsupportedInvocSyntaxException(invoc, "SPDX_LICENSE keyword") from None
                    case Keyword.DESCRIPTION:
                        if description is not None:
                            error(context.file_index, arg.pos, "More than one DESCRIPTION keyword in %s() invocation",
                                  invoc.command_name)
                            return
                        arg = server.consume_not_keyword(keywords)
                        if arg:
                            description = arg.string.string
                        else:
                            warn(context.file_index, server.next_pos, "Missing description after DESCRIPTION keyword "
                                 "in %s() invocation", invoc.command_name)
                            description = ""
                        continue
                    case Keyword.HOMEPAGE_URL:
                        if homepage_url is not None:
                            error(context.file_index, arg.pos, "More than one HOMEPAGE_URL keyword in %s() invocation",
                                  invoc.command_name)
                            return
                        arg = server.consume_not_keyword(keywords)
                        if arg:
                            homepage_url = arg.string.string
                        else:
                            warn(context.file_index, server.next_pos, "Missing URL after HOMEPAGE_URL keyword in %s() "
                                 "invocation", invoc.command_name)
                            homepage_url = ""
                        continue
                    case Keyword.LANGUAGES:
                        if languages_keyword_seen:
                            error(context.file_index, arg.pos, "More than one LANGUAGES keyword in %s() invocation",
                                  invoc.command_name)
                            return
                        languages_keyword_seen = True
                        while True:
                            arg = server.consume_not_keyword(keywords)
                            if not arg:
                                break
                            if arg.string.string != "NONE":
                                languages.append(arg.string.string)
                        continue
                    case None:
                        if not has_languages_keyword:
                            error(context.file_index, arg.pos, "Language argument (%s) without LANGUAGES keyword in "
                                  "%s() invocation", _b.quote(arg.string.string), invoc.command_name)
                            return
                        warn(context.file_index, arg.pos, "Misplaced language argument (%s) in %s() invocation",
                             _b.quote(arg.string.string), invoc.command_name)
                        if arg.string.string != "NONE":
                            languages.append(arg.string.string)
                        continue
                typing.assert_never(keyword)

        if not languages and not no_default_languages:
            languages = ["C", "CXX"]

        def set_regular(var_name: str, value: str) -> None:
            set_regular_variable(var_name, value, invoc, context)

        # If variable is unset or project() is invoked from root directory, set cache
        # variable and unset regular variable. This is CMake's behavior for the
        # `CMAKE_PROJECT_` family of variables.
        def set_cache(var_name: str, value: str) -> None:
            if not context.is_root_dir:
                value_2, _ = resolve_variable(_cu.ResolutionType.GENERAL, var_name, invoc.pos, context)
                match value_2:
                    case _cv.CertainValue():
                        if value_2.string:
                            return
                    case _cv.UncertainValue():
                        if value_2.reason:
                            # The variable is tainted, so it remains tainted.
                            return
                        # When there is no uncertainty reason, it means that no value or
                        # uncertainty reason has yet been recorded for the variable. In the
                        # case of these well defined variables, we take that to mean that
                        # the variable is certainly unset.
                    case _:
                        typing.assert_never(value_2)
            set_cache_variable(var_name, value, invoc, context)
            set_regular_variable(var_name, None, invoc, context)

        # In CMake 4.3.4, the variables `<project name>_SOURCE_DIR`, `<project
        # name>_BINARY_DIR`, and `<project name>_IS_TOP_LEVEL` are handled
        # differently. Here, the cache variable of that name is set. Additionally, if the
        # regular variable is set or if policy CMP0180 is NEW, the regular variable is set
        # to the same value.
        cmp0180_is_certainly_new = False
        cmp0180_uncertainty: _cur.ValueUncertaintyReason | None = None
        policy_value = resolve_policy(_cpo.Policy.CMP0180, invoc, context)
        match policy_value:
            case _CertainResolvedPolicyValue():
                if policy_value.new:
                    cmp0180_is_certainly_new = True
            case _UncertainPolicyValue():
                cmp0180_uncertainty = policy_value.reason
            case _:
                typing.assert_never(policy_value)
        def set_special(var_name: str, value: str) -> None:
            regular_var_is_certainly_unset = False
            regular_var_is_certainly_set = False
            value_2, var_type = resolve_variable(_cu.ResolutionType.GENERAL, var_name, invoc.pos, context)
            match value_2:
                case _cv.CertainValue():
                    if value_2.string is None or var_type == _cv.VariableType.CACHE:
                        regular_var_is_certainly_unset = True
                    else:
                        regular_var_is_certainly_set = True
                case _cv.UncertainValue():
                    if value_2.reason is None or var_type == _cv.VariableType.CACHE:
                        regular_var_is_certainly_unset = True
                case _:
                    typing.assert_never(value_2)
            set_cache_variable(var_name, value, invoc, context)
            if regular_var_is_certainly_set or cmp0180_is_certainly_new:
                set_regular_variable(var_name, value, invoc, context)
            elif cmp0180_uncertainty and regular_var_is_certainly_unset:
                command_position = _cur.Position(context.file_index, invoc.pos)
                policy = _cpo.Policy.CMP0180
                reason = _cur.AssignmentPolicyUncertaintyReason(invoc.command_name, command_position, policy,
                                                                cmp0180_uncertainty)
                taint_regular_variable(var_name, reason, context)

        # Replicating quirky CMake 4.3 behavior: If the VERSION keyword is absent, version
        # variables are changed to the empty string if they have a nonempty string
        # value. Otherwise, they are left unchanged. When judging whether they have a
        # nonempty string value, the cache is consulted (general resolution type).
        def reset_special(var_name: str) -> None:
            value, _ = resolve_variable(_cu.ResolutionType.GENERAL, var_name, invoc.pos, context)
            match value:
                case _cv.CertainValue():
                    if value.string:
                        set_regular_variable(var_name, "", invoc, context)
                    return
                case _cv.UncertainValue():
                    # If there is no uncertainty reason, this variable can be taken to be
                    # definitely unset, in which case there is nothing to do. If there is an
                    # uncertainty reason, the variable is tainted. In that case, the
                    # variable must remain tainted.
                    return
            typing.assert_never(value)

        set_cache("CMAKE_PROJECT_NAME", name)
        set_regular("PROJECT_NAME", name)

        source_dir = str(context.resolve_path(context.source_dir))
        binary_dir = str(context.resolve_path(context.binary_dir)) if context.binary_dir is not None else None
        set_regular("PROJECT_SOURCE_DIR", source_dir)
        set_special(name + "_SOURCE_DIR", source_dir)
        if binary_dir is not None:
            set_regular("PROJECT_BINARY_DIR", binary_dir)
            set_special(name + "_BINARY_DIR", binary_dir)

        is_top_level = "ON" if context.is_root_dir else "OFF"
        set_regular("PROJECT_IS_TOP_LEVEL", is_top_level)
        set_special(name + "_IS_TOP_LEVEL", is_top_level)

        if version is None:
            if context.is_root_dir:
                reset_special("CMAKE_PROJECT_VERSION")
                reset_special("CMAKE_PROJECT_VERSION_MAJOR")
                reset_special("CMAKE_PROJECT_VERSION_MINOR")
                reset_special("CMAKE_PROJECT_VERSION_PATCH")
                reset_special("CMAKE_PROJECT_VERSION_TWEAK")
            reset_special("PROJECT_VERSION")
            reset_special("PROJECT_VERSION_MAJOR")
            reset_special("PROJECT_VERSION_MINOR")
            reset_special("PROJECT_VERSION_PATCH")
            reset_special("PROJECT_VERSION_TWEAK")
            reset_special(name + "_VERSION")
            reset_special(name + "_VERSION_MAJOR")
            reset_special(name + "_VERSION_MINOR")
            reset_special(name + "_VERSION_PATCH")
            reset_special(name + "_VERSION_TWEAK")
        else:
            set_cache("CMAKE_PROJECT_VERSION", version)
            set_cache("CMAKE_PROJECT_VERSION_MAJOR", version_major)
            set_cache("CMAKE_PROJECT_VERSION_MINOR", version_minor)
            set_cache("CMAKE_PROJECT_VERSION_PATCH", version_patch)
            set_cache("CMAKE_PROJECT_VERSION_TWEAK", version_tweak)
            set_regular("PROJECT_VERSION", version)
            set_regular("PROJECT_VERSION_MAJOR", version_major)
            set_regular("PROJECT_VERSION_MINOR", version_minor)
            set_regular("PROJECT_VERSION_PATCH", version_patch)
            set_regular("PROJECT_VERSION_TWEAK", version_tweak)
            set_regular(name + "_VERSION", version)
            set_regular(name + "_VERSION_MAJOR", version_major)
            set_regular(name + "_VERSION_MINOR", version_minor)
            set_regular(name + "_VERSION_PATCH", version_patch)
            set_regular(name + "_VERSION_TWEAK", version_tweak)

        # With CMake 4.3, "description" and "homepage URL" variables are set to the empty
        # string when the respective keywords are absent. These variables are never left in
        # the unset state (with value `None`).
        set_cache("CMAKE_PROJECT_DESCRIPTION", description or "")
        set_cache("CMAKE_PROJECT_HOMEPAGE_URL", homepage_url or "")
        set_regular("PROJECT_DESCRIPTION", description or "")
        set_regular("PROJECT_HOMEPAGE_URL", homepage_url or "")
        set_regular(name + "_DESCRIPTION", description or "")
        set_regular(name + "_HOMEPAGE_URL", homepage_url or "")

    def set_policy_version(version: _cve.Version, invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        assert not context.occurrence_uncertainty
        # Emulate CMake 4.3 behavior, which is to set behavior to NEW for all policies that
        # were introduced at or before the negotiated policy version. All other policies get
        # their behavior unset, so that the behavior gets determined dynamically on
        # demand. This happens in determine_default_policy_state() below.
        for definition in _cpo.get_definitions():
            if definition.toggleable:
                new: bool | None = None
                if definition.intro_version <= version:
                    new = True
                set_policy(definition.toggleable, new, invoc, context)

    def evaluate_condition(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _cc.Result:
        arguments, card_uncertainty = expand_arguments(invoc, context)
        condition = _cc.parse(invoc.command_name, arguments, invoc.rparen_pos)
        class State(_cv.VariableState):
            @typing.override
            def get(self, resolution_type: _cu.ResolutionType, variable_name: str,
                    pos: int) -> tuple[_cv.Value, _cv.VariableType]:
                return resolve_variable(resolution_type, variable_name, pos, context)
            @typing.override
            def set_(self, variable_name: str, value: str | None) -> None:
                set_regular_variable(variable_name, value, invoc, context)
            @typing.override
            def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
                taint_regular_variable(variable_name, reason, context)
        state = State()
        return _cc.evaluate(condition, invoc.command_name, context.file_index, state, context.process.lenient_mode)

    def create_argument_server(invoc: _clp.Invoc, context: _InvocContext) -> _ca.ArgumentServer:
        arguments, card_uncertainty = expand_arguments(invoc, context)
        return _ca.ArgumentServer(invoc, arguments, context.file_index)

    def expand_arguments(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> tuple[list[_ca.Argument], bool]:
        arguments = list[_ca.Argument]()
        card_uncertainty = False
        for protoarg in invoc.arguments:
            arg: _ca.Argument
            match protoarg:
                case _clp.CertainProtoargument():
                    match protoarg.type_:
                        case _clp.ProtoargumentType.BARE:
                            was_bare = True
                            result = expand_string(protoarg.string, invoc, context)
                            match result:
                                case _CertainExpansionResult():
                                    is_derived = protoarg.is_derived or result.is_derived
                                    segments, is_derived_2 = _list_split(result.string, is_derived)
                                    for segment in segments:
                                        arg = _ca.CertainArgument(protoarg.pos, was_bare, segment, is_derived_2)
                                        arguments.append(arg)
                                    continue
                                case _UncertainExpansionResult():
                                    card_uncertainty = True
                                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                                    arguments.append(arg)
                                    continue
                            typing.assert_never(result)
                        case _clp.ProtoargumentType.QUOTED:
                            was_bare = False
                            result = expand_string(protoarg.string, invoc, context)
                            match result:
                                case _CertainExpansionResult():
                                    is_derived = protoarg.is_derived or result.is_derived
                                    arg = _ca.CertainArgument(protoarg.pos, was_bare, result.string, is_derived)
                                    arguments.append(arg)
                                    continue
                                case _UncertainExpansionResult():
                                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                                    arguments.append(arg)
                                    continue
                            typing.assert_never(result)
                        case _clp.ProtoargumentType.BRACKETED:
                            assert not protoarg.is_derived
                            was_bare = False
                            is_derived = False
                            arg = _ca.CertainArgument(protoarg.pos, was_bare, protoarg.string, is_derived)
                            arguments.append(arg)
                            continue
                    typing.assert_never(protoarg.type_)
                case _clp.UncertainProtoargument():
                    match protoarg.type_:
                        case _clp.ProtoargumentType.BARE:
                            was_bare = True
                            card_uncertainty = True
                        case _clp.ProtoargumentType.QUOTED:
                            was_bare = False
                        case _clp.ProtoargumentType.BRACKETED:
                            assert False  # Not affected by macro substitutions
                        case _:
                            typing.assert_never(protoarg.type_)
                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, protoarg.reason)
                    arguments.append(arg)
                    continue
            typing.assert_never(protoarg)
        return arguments, card_uncertainty

    def expand_string(string: _tp.PosMappedString, invoc: _clp.GeneralizedInvoc,
                      context: _InvocContext) -> _ExpansionResult:
        def expand(expr: _csp.Expr) -> _ExpansionResult:
            if isinstance(expr, _csp.StringExpr):
                is_derived = False
                return _CertainExpansionResult(expr.string, is_derived)
            if isinstance(expr, _csp.CompositeExpr):
                string_builder = _tp.PosMappedStringBuilder()
                for part in expr.parts:
                    result = expand(part)
                    if isinstance(result, _CertainExpansionResult):
                        string_builder.add_pos_mapped_string(result.string)
                        continue
                    if isinstance(result, _UncertainExpansionResult):
                        return result
                    typing.assert_never(result)
                is_derived = True
                return _CertainExpansionResult(string_builder.finalize_and_get(), is_derived)
            if isinstance(expr, _csp.ExpansionExpr):
                result = expand(expr.name_expr)
                if isinstance(result, _CertainExpansionResult):
                    name = result.string.string
                    value, variable_type = resolve_variable(expr.resolution_type, name, expr.name_expr.pos, context)
                    if isinstance(value, _cv.CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _cv.UncertainValue):
                        param_type = _cv.variable_to_param_type(variable_type)
                        position = _cur.Position(context.file_index, expr.pos)
                        reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, name, position,
                                                                 value.reason)
                        return _UncertainExpansionResult(reason)
                    typing.assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result
                typing.assert_never(result)
            typing.assert_never(expr)
        def error_handler(pos: int, message: str, *args: typing.Any) -> None:
            error(context.file_index, pos, message, *args)
        return expand(_csp.parse(string, error_handler))

    def resolve_certain_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                                 invoc: _clp.GeneralizedInvoc, context: _InvocContext,
                                 undefined_is_certain: bool = False) -> str | None:
        value, variable_type = resolve_variable(resolution_type, variable_name, pos, context)
        match value:
            case _cv.CertainValue():
                return value.string
            case _cv.UncertainValue():
                if not value.reason and undefined_is_certain:
                    return None
                param_type = _cv.variable_to_param_type(variable_type)
                expansion_position = _cur.Position(context.file_index, pos)
                reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, variable_name,
                                                         expansion_position, value.reason)
                raise _UncertainVariableResolutionException(reason) from None
        typing.assert_never(value)

    def resolve_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                         context: _InvocContext) -> tuple[_cv.Value, _cv.VariableType]:
        match resolution_type:
            case _cu.ResolutionType.GENERAL:
                value = context.state.get_regular_variable(variable_name)
                match value:
                    case _cv.CertainValue(string):
                        if string is not None:
                            return value, _cv.VariableType.REGULAR
                    case _cv.UncertainValue():
                        return value, _cv.VariableType.REGULAR
                    case _:
                        typing.assert_never(value)
                return context.state.get_cache_variable(variable_name), _cv.VariableType.CACHE
            case _cu.ResolutionType.CACHE:
                return context.state.get_cache_variable(variable_name), _cv.VariableType.CACHE
            case _cu.ResolutionType.ENV:
                return context.state.get_env_variable(variable_name), _cv.VariableType.ENV
        typing.assert_never(resolution_type)

    def set_regular_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                             context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_regular_variable(variable_name, value_2)

    def taint_regular_variable(variable_name: str, reason: _cur.ValueUncertaintyReason,
                               context: _InvocContext) -> None:
        context.state.set_regular_variable(variable_name, _cv.UncertainValue(reason))

    def set_parent_scope_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                                  context: _InvocContext) -> None:
        if context.state.is_root_scope():
            warn(context.file_index, invoc.pos, "%s() invocation skipped: No parent scope exists", invoc.command_name)
            return
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_parent_scope_variable(variable_name, value_2)

    def taint_parent_scope_variable(variable_name: str, reason: _cur.ValueUncertaintyReason,
                                    context: _InvocContext) -> None:
        assert not context.state.is_root_scope()
        context.state.set_parent_scope_variable(variable_name, _cv.UncertainValue(reason))

    def set_cache_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                           context: _InvocContext) -> None:
        # FIXME: Tend to policy CMP0126. If `value` is not `None` and policy behavior is
        # OLD, unset the regular variable of the same name. If policy behavior is uncertain
        # and regular variable is definitely not unset, taint the regular variable. If
        # policy behavior is uncertain and the regular variable is definitely unset or is in
        # an uncertain state, leave the regular variable in its current state.          
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_cache_variable(variable_name, value_2)

    def taint_cache_variable(variable_name: str, reason: _cur.ValueUncertaintyReason, context: _InvocContext) -> None:
        context.state.set_cache_variable(variable_name, _cv.UncertainValue(reason))

    def set_env_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                         context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_env_variable(variable_name, value_2)

    def taint_env_variable(variable_name: str, reason: _cur.ValueUncertaintyReason, context: _InvocContext) -> None:
        context.state.set_env_variable(variable_name, _cv.UncertainValue(reason))

    def resolve_certain_policy(policy: _cpo.Policy, invoc: _clp.GeneralizedInvoc, context: _InvocContext,
                               suppress_warning: bool = False) -> bool:
        value = resolve_policy(policy, invoc, context, suppress_warning)
        match value:
            case _CertainResolvedPolicyValue():
                return value.new
            case _UncertainPolicyValue():
                raise _UncertainPolicyException(invoc.command_name, policy, invoc.pos, value.reason) from None
        typing.assert_never(value)

    def resolve_policy(policy: _cpo.Policy, invoc: _clp.GeneralizedInvoc, context: _InvocContext,
                       suppress_warning: bool = False) -> _ResolvedPolicyValue:
        value = context.state.get_policy(policy)
        match value:
            case _CertainPolicyValue():
                if value.new is not None:
                    new = value.new
                else:
                    new = determine_default_policy_state(policy, invoc, context, suppress_warning)
                return _CertainResolvedPolicyValue(new)
            case _UncertainPolicyValue():
                return value
        typing.assert_never(value)

    def set_policy(policy: _cpo.Policy, new: bool | None, invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value = _CertainAssignedPolicyValue(new, invoc.command_name, assignment_position)
        context.state.set_policy(policy, value)

    def determine_default_policy_state(policy: _cpo.Policy, invoc: _clp.GeneralizedInvoc, context: _InvocContext,
                                       suppress_warning: bool) -> bool:
        definition = _cpo.get_definition(policy)

        # If the policy is not yet introduced in the simulated CMake version
        # (`context.process.cmake_version`), its state is definitely OLD
        if definition.intro_version > context.process.cmake_version:
            return False  # OLD

        # The policy is known in the simulated CMake version, so we need to consult the
        # variable that affects its default state
        name = "CMAKE_POLICY_DEFAULT_" + definition.name
        value = resolve_certain_variable(_cu.ResolutionType.GENERAL, name, invoc.pos, invoc, context,
                                         undefined_is_certain=True)
        if value == "NEW":
            return True  # NEW
        if value == "OLD":
            if definition.force_version is not None and definition.force_version <= context.process.cmake_version:
                raise _CommandExecutionFailedException(invoc.command_name, invoc.pos, "Opt out from policy %s not "
                                                       "possible in CMake %s", definition.name,
                                                       context.process.cmake_version) from None
            return False  # OLD

        if definition.force_version is not None and definition.force_version <= context.process.cmake_version:
            # If the policy has become forced in the simulated CMake version, the state of
            # the policy is forced to NEW even if it would have been OLD based on the
            # negotiated policy version. This appears to be in line with CMake's behavior.
            return True  # NEW

        if not definition.suppress_warning and not suppress_warning:
            warn(context.file_index, invoc.pos, "Policy %s is not in effect: %s", definition.name,
                 definition.description)
        return False  # OLD

    def trace_value_uncertainty_causes(cause: _cur.ValueUncertaintyReason) -> None:
        match cause:
            case _cur.ExpansionUncertaintyReason():
                trace_expansion_uncertainty_causes(cause)
                return
            case _cur.AssignmentOccurrenceUncertaintyReason():
                position = cause.assignment_position
                error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                      cause.command_name)
                trace_expansion_uncertainty_causes(cause.occurrence_uncertainty_reason)
                return
            case _cur.AssignmentPolicyUncertaintyReason():
                position = cause.command_position
                error(position.file_index, position.pos, "Caused by execution of %s() with uncertain state of policy "
                      "%s", cause.command_name, _cpo.get_name(cause.policy))
                trace_value_uncertainty_causes(cause.policy_uncertainty_reason)
                return
        typing.assert_never(cause)

    def trace_expansion_uncertainty_causes(cause: _cur.ExpansionUncertaintyReason) -> None:
        position = cause.expansion_position
        error(position.file_index, position.pos, "Caused by expansion of %s with uncertain value in invocation of "
              "%s()", cause.get_qual_param_ref(), cause.command_name)
        if cause.value_uncertainty_reason:
            trace_value_uncertainty_causes(cause.value_uncertainty_reason)

    def warn(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        position = _cur.Position(file_index, pos)
        application.warn(position, message, *args)

    def error(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        nonlocal errors_seen
        errors_seen = True
        position = _cur.Position(file_index, pos)
        application.error(position, message, *args)

    cmake_version = CMAKE_VERSION
    if config.cmake_version is not None:
        if not (LOWEST_SUPPORTED_CMAKE_VERSION <= config.cmake_version <= CMAKE_VERSION):
            raise ValueError("CMake version out of range")
        cmake_version = config.cmake_version

    source_dir = str(application.resolve_path(config.source_dir))
    binary_dir = str(application.resolve_path(config.binary_dir)) if config.binary_dir is not None else None

    initial_variables = config.initial_variables.copy()
    initial_variables["CMAKE_VERSION"] = str(cmake_version)  # Always on 3-component form
    initial_variables["CMAKE_MAJOR_VERSION"] = str(cmake_version.major)
    initial_variables["CMAKE_MINOR_VERSION"] = str(cmake_version.minor)
    initial_variables["CMAKE_PATCH_VERSION"] = str(cmake_version.patch)
    initial_variables["CMAKE_TWEAK_VERSION"] = "0"
    initial_variables["CMAKE_SOURCE_DIR"] = source_dir
    initial_variables["CMAKE_CURRENT_SOURCE_DIR"] = source_dir
    if binary_dir is not None:
        initial_variables["CMAKE_BINARY_DIR"] = binary_dir
        initial_variables["CMAKE_CURRENT_BINARY_DIR"] = binary_dir

    definitely_unset_variables = [
        "CMAKE_MINIMUM_REQUIRED_VERSION",

        "CMAKE_PROJECT_NAME",
        "CMAKE_PROJECT_VERSION",
        "CMAKE_PROJECT_VERSION_MAJOR",
        "CMAKE_PROJECT_VERSION_MINOR",
        "CMAKE_PROJECT_VERSION_PATCH",
        "CMAKE_PROJECT_VERSION_TWEAK",
        "CMAKE_PROJECT_DESCRIPTION",
        "CMAKE_PROJECT_HOMEPAGE_URL",

        "PROJECT_NAME",
        "PROJECT_SOURCE_DIR",
        "PROJECT_BINARY_DIR",
        "PROJECT_IS_TOP_LEVEL",
        "PROJECT_VERSION",
        "PROJECT_VERSION_MAJOR",
        "PROJECT_VERSION_MINOR",
        "PROJECT_VERSION_PATCH",
        "PROJECT_VERSION_TWEAK",
        "PROJECT_DESCRIPTION",
        "PROJECT_HOMEPAGE_URL",
    ]
    for name in definitely_unset_variables:
        initial_variables.setdefault(name)

    is_root_dir = True
    state = _RootState(initial_variables, config.define_breakpoint_command)
    occurrence_uncertainty = None
    root_dir = config.source_dir
    process_context = _ProcessContext(application, pos_resolver, config.lenient_mode, config.suppress_messages,
                                      cmake_version, root_dir)
    process_file(process_context, cmake_source, is_root_dir, state, occurrence_uncertainty, config.source_dir,
                 config.binary_dir)
    return not errors_seen


_ASCII_LOWER_MAP = str.maketrans(_string.ascii_uppercase, _string.ascii_lowercase)
_ASCII_UPPER_MAP = str.maketrans(_string.ascii_lowercase, _string.ascii_uppercase)


@dataclasses.dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


# `is_root_dir` is true inside the immediately processed CMake file (the one whose path was
# passed to process()) and all files included by it using include() invocations. Everywhere
# else, that is inside files whose inclusion path involve add_subdirectory() invocations,
# `is_root_dir` is false.
#
@dataclasses.dataclass(slots=True, frozen=True)
class _InvocContext:
    process:                _ProcessContext
    file_index:             int
    is_root_dir:            bool
    state:                  _State
    occurrence_uncertainty: OccurrenceUncertainty
    source_dir:             pathlib.Path
    binary_dir:             pathlib.Path | None

    def with_state(self, state: _State) -> _InvocContext:
        return dataclasses.replace(self, state=state)

    def resolve_file_pos(self, pos: _cur.Position) -> _tp.FilePos:
        return self.process.pos_resolver.resolve_file_pos(pos)

    def open_subfile(self, path: pathlib.Path) -> typing.TextIO:
        return self.process.application.open_subfile(path)

    def resolve_path(self, path: pathlib.Path) -> pathlib.Path:
        return self.process.application.resolve_path(path)


@dataclasses.dataclass(slots=True, frozen=True)
class _ProcessContext:
    application:       Application
    pos_resolver:      PositionResolver
    lenient_mode:      bool
    suppress_messages: bool
    cmake_version:     _cve.Version
    root_dir:          pathlib.Path


class _State(abc.ABC):
    @abc.abstractmethod
    def is_root_scope(self) -> bool:
        ...

    @abc.abstractmethod
    def get_regular_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_cache_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_env_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_command(self, name_cf: str) -> _Command:
        ...

    @abc.abstractmethod
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        ...

    @abc.abstractmethod
    def get_policy(self, policy: _cpo.Policy) -> _PolicyValue:
        ...

    @abc.abstractmethod
    def set_policy(self, policy: _cpo.Policy, value: _AssignedPolicyValue) -> None:
        ...


class _RootState(_State):
    def __init__(self, initial_variables: dict[str, str | None], define_breakpoint_command: bool) -> None:
        self._policies          = dict[_cpo.Policy, _PolicyValue]()
        self._commands          = dict[str, _Command]()
        self._env_variables     = dict[str, _cv.Value]()
        self._cache_variables   = dict[str, _cv.Value]()
        self._regular_variables = dict[str, _cv.Value]()
        for name, value in initial_variables.items():
            self._cache_variables[name] = _cv.CertainValue(None)
            self._regular_variables[name] = _cv.CertainValue(value)
        self._define_built_in_commands(define_breakpoint_command)

    @typing.override
    def is_root_scope(self) -> bool:
        return True

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._regular_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._regular_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        assert False

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        assert False

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        value = self._cache_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._cache_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._cache_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        value = self._env_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._env_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._env_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            return command
        reason = None
        return _UncertainCommand(reason)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        match command:
            case _CertainDefinedCommand():
                self._commands[name_cf] = command.command
                return
            case _UncertainCommand():
                self._commands[name_cf] = command
                return
        typing.assert_never(command)

    @typing.override
    def get_policy(self, policy: _cpo.Policy) -> _PolicyValue:
        value = self._policies.get(policy)
        return value or _CertainPolicyValue(None)

    @typing.override
    def set_policy(self, policy: _cpo.Policy, value: _AssignedPolicyValue) -> None:
        match value:
            case _CertainAssignedPolicyValue():
                if value.new is None:
                    self._policies.pop(policy, None)
                else:
                    self._policies[policy] = _CertainPolicyValue(value.new)
                return
            case _UncertainPolicyValue():
                self._policies[policy] = value
                return
        typing.assert_never(value)

    def _define_built_in_commands(self, define_breakpoint_command: bool) -> None:
        def define(name_cf: str, which: _BuiltInCommand.Which) -> None:
            self._commands[name_cf] = _BuiltInCommand(which)
        define("set",                    _BuiltInCommand.Which.SET)
        define("unset",                  _BuiltInCommand.Which.UNSET)
        define("string",                 _BuiltInCommand.Which.STRING)
        define("list",                   _BuiltInCommand.Which.LIST)
        define("message",                _BuiltInCommand.Which.MESSAGE)
        define("include",                _BuiltInCommand.Which.INCLUDE)
        define("add_subdirectory",       _BuiltInCommand.Which.ADD_SUBDIRECTORY)
        define("cmake_minimum_required", _BuiltInCommand.Which.CMAKE_MINIMUM_REQUIRED)
        define("project",                _BuiltInCommand.Which.PROJECT)
        define("option",                 _BuiltInCommand.Which.UNSUPPORTED)
        if define_breakpoint_command:
            define("breakpoint", _BuiltInCommand.Which.BREAKPOINT)


class _SubscopeState(_State):
    def __init__(self, parent_state: _State, initial_variables: dict[str, str | None] = {}) -> None:
        self._parent_state      = parent_state
        self._regular_variables = dict[str, _cv.Value]()
        for name, value in initial_variables.items():
            self._regular_variables[name] = _cv.CertainValue(value)

    @typing.override
    def is_root_scope(self) -> bool:
        return False

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            return value
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._regular_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._regular_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        # Copy original value from parent scope before changing it in the parent scope. This
        # is necessary in order to uphold the illusion that the subscope holds a complete
        # copy of the original parent variable state.
        if name not in self._regular_variables:
            self._regular_variables[name] = self._parent_state.get_regular_variable(name)
        self._parent_state.set_regular_variable(name, value)

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_cache_variable(name)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_cache_variable(name, value)

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_env_variable(name)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_env_variable(name, value)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        self._parent_state.set_command(name_cf, command)

    @typing.override
    def get_policy(self, policy: _cpo.Policy) -> _PolicyValue:
        return self._parent_state.get_policy(policy)

    @typing.override
    def set_policy(self, policy: _cpo.Policy, value: _AssignedPolicyValue) -> None:
        self._parent_state.set_policy(policy, value)


class _OccurrenceUncertaintyOverlayState(_State):
    def __init__(self, parent_state: _State, occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason) -> None:
        self._parent_state                   = parent_state
        self._occurrence_uncertainty_reason  = occurrence_uncertainty_reason
        self._policies                       = dict[_cpo.Policy, _AssignedPolicyValue]()
        self._tainted_policies               = dict[_cpo.Policy, _UncertainPolicyValue]()
        self._commands                       = dict[str, _DefinedCommand]()
        self._tainted_commands               = dict[str, _UncertainCommand]()
        self._env_variables                  = dict[str, _AssignedValue]()
        self._tainted_env_variables          = dict[str, _cv.UncertainValue]()
        self._cache_variables                = dict[str, _AssignedValue]()
        self._tainted_cache_variables        = dict[str, _cv.UncertainValue]()
        self._regular_variables              = dict[str, _AssignedValue]()
        self._tainted_regular_variables      = dict[str, _cv.UncertainValue]()
        self._parent_scope_variables         = dict[str, _AssignedValue]()
        self._tainted_parent_scope_variables = dict[str, _cv.UncertainValue]()

    def prune_taints(self) -> None:
        value:        _AssignedPolicyValue | _AssignedValue
        parent_value: _PolicyValue | _cv.Value
        for policy, value in list(self._policies.items()):
            match value:
                case _CertainAssignedPolicyValue():
                    parent_value = self._parent_state.get_policy(policy)
                    match parent_value:
                        case _CertainPolicyValue():
                            if parent_value.new == value.new:
                                del self._policies[policy]
                                del self._tainted_policies[policy]
                            continue
                        case _UncertainPolicyValue():
                            continue
                    typing.assert_never(parent_value)
                case _UncertainPolicyValue():
                    continue
            typing.assert_never(value)
        # Command taints are uncancellable because it is impossible in CMake to assign a
        # command to a name that is the same command as the name once referred to. One can
        # imagine assigning a new macro with the same macro body, but it is still not the
        # same macro because the metadata (position in source code of definition) is
        # necessarily different. Also, the new body may be nested inside another macro body,
        # which means that substitutions may occur.
        for name, value in list(self._env_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_env_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._env_variables[name]
                                del self._tainted_env_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._cache_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_cache_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._cache_variables[name]
                                del self._tainted_cache_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._regular_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_regular_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._regular_variables[name]
                                del self._tainted_regular_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._parent_scope_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_parent_scope_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._parent_scope_variables[name]
                                del self._tainted_parent_scope_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)

    def cross_prune_taints(self, else_state: _OccurrenceUncertaintyOverlayState) -> None:
        value:      _AssignedPolicyValue | _AssignedValue
        else_value: _AssignedPolicyValue | _AssignedValue | None
        assert self._parent_state is else_state._parent_state
        for policy, value in list(self._policies.items()):
            match value:
                case _CertainAssignedPolicyValue():
                    else_value = else_state._policies.get(policy)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedPolicyValue():
                            if value.new == else_value.new:
                                self._parent_state.set_policy(policy, value)
                                del self._policies[policy]
                                del self._tainted_policies[policy]
                                del else_state._policies[policy]
                                del else_state._tainted_policies[policy]
                            continue
                        case _UncertainPolicyValue():
                            continue
                    typing.assert_never(else_value)
                case _UncertainPolicyValue():
                    continue
            typing.assert_never(value)
        # Commands are uncomparable. See prune_taints().
        for name, value in list(self._env_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._env_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_env_variable(name, value)
                                del self._env_variables[name]
                                del self._tainted_env_variables[name]
                                del else_state._env_variables[name]
                                del else_state._tainted_env_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._cache_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._cache_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_cache_variable(name, value)
                                del self._cache_variables[name]
                                del self._tainted_cache_variables[name]
                                del else_state._cache_variables[name]
                                del else_state._tainted_cache_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._regular_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._regular_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_regular_variable(name, value)
                                del self._regular_variables[name]
                                del self._tainted_regular_variables[name]
                                del else_state._regular_variables[name]
                                del else_state._tainted_regular_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._parent_scope_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._parent_scope_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_parent_scope_variable(name, value)
                                del self._parent_scope_variables[name]
                                del self._tainted_parent_scope_variables[name]
                                del else_state._parent_scope_variables[name]
                                del else_state._tainted_parent_scope_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)

    def push_taints(self) -> None:
        value: _AssignedPolicyValue | _DefinedCommand | _AssignedValue
        for policy, value in self._tainted_policies.items():
            self._parent_state.set_policy(policy, value)
        for name_cf, value in self._tainted_commands.items():
            self._parent_state.set_command(name_cf, value)
        for name, value in self._tainted_env_variables.items():
            self._parent_state.set_env_variable(name, value)
        for name, value in self._tainted_cache_variables.items():
            self._parent_state.set_cache_variable(name, value)
        for name, value in self._tainted_regular_variables.items():
            self._parent_state.set_regular_variable(name, value)
        for name, value in self._tainted_parent_scope_variables.items():
            self._parent_state.set_parent_scope_variable(name, value)

    @typing.override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if not value:
            return self._parent_state.get_regular_variable(name)
        match value:
            case _CertainAssignedValue():
                return _cv.CertainValue(value.string)
            case _cv.UncertainValue():
                return value
        typing.assert_never(value)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        self._regular_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_regular_variables[name] = taint_value

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        value = self._parent_scope_variables.get(name)
        if not value:
            return self._parent_state.get_parent_scope_variable(name)
        match value:
            case _CertainAssignedValue():
                return _cv.CertainValue(value.string)
            case _cv.UncertainValue():
                return value
        typing.assert_never(value)

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_scope_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_parent_scope_variables[name] = taint_value

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        value = self._cache_variables.get(name)
        if not value:
            return self._parent_state.get_cache_variable(name)
        match value:
            case _CertainAssignedValue():
                return _cv.CertainValue(value.string)
            case _cv.UncertainValue():
                return value
        typing.assert_never(value)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        self._cache_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_cache_variables[name] = taint_value

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        value = self._env_variables.get(name)
        if not value:
            return self._parent_state.get_env_variable(name)
        match value:
            case _CertainAssignedValue():
                return _cv.CertainValue(value.string)
            case _cv.UncertainValue():
                return value
        typing.assert_never(value)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        self._env_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_env_variables[name] = taint_value

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if not command:
            return self._parent_state.get_command(name_cf)
        match command:
            case _CertainDefinedCommand():
                return command.command
            case _UncertainCommand():
                return command
        typing.assert_never(command)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        self._commands[name_cf] = command
        match command:
            case _CertainDefinedCommand():
                reason = _CommandDefinitionUncertaintyReason(command.defining_command_name,
                                                             command.definition_position,
                                                             self._occurrence_uncertainty_reason)
                taint_command = _UncertainCommand(reason)
            case _UncertainCommand():
                taint_command = command
            case _:
                typing.assert_never(command)
        self._tainted_commands[name_cf] = taint_command

    @typing.override
    def get_policy(self, policy: _cpo.Policy) -> _PolicyValue:
        value = self._policies.get(policy)
        if not value:
            return self._parent_state.get_policy(policy)
        match value:
            case _CertainAssignedPolicyValue():
                return _CertainPolicyValue(value.new)
            case _UncertainPolicyValue():
                return value
        typing.assert_never(value)

    @typing.override
    def set_policy(self, policy: _cpo.Policy, value: _AssignedPolicyValue) -> None:
        self._policies[policy] = value
        match value:
            case _CertainPolicyValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _UncertainPolicyValue(reason)
            case _UncertainPolicyValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_policies[policy] = taint_value


type _PolicyValue = _CertainPolicyValue | _UncertainPolicyValue

type _ResolvedPolicyValue = _CertainResolvedPolicyValue | _UncertainPolicyValue

type _AssignedPolicyValue = _CertainAssignedPolicyValue | _UncertainPolicyValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainPolicyValue:
    new: bool | None

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainResolvedPolicyValue:
    new: bool

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainAssignedPolicyValue(_CertainPolicyValue):
    assigning_command_name: str
    assignment_position:    _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainPolicyValue:
    reason: _cur.ValueUncertaintyReason


type _Command = _CertainCommand | _UncertainCommand

type _CertainCommand = _BuiltInCommand | _CustomCommand

type _DefinedCommand = _CertainDefinedCommand | _UncertainCommand

@dataclasses.dataclass(slots=True, frozen=True)
class _BuiltInCommand:
    class Which(enum.Enum):
        UNSUPPORTED            =  0
        SET                    =  1
        UNSET                  =  2
        STRING                 =  3
        LIST                   =  4
        MESSAGE                =  5
        INCLUDE                =  6
        ADD_SUBDIRECTORY       =  7
        CMAKE_MINIMUM_REQUIRED =  8
        PROJECT                =  9
        BREAKPOINT             = 10  # Non-standard
    which: Which

@dataclasses.dataclass(slots=True, frozen=True)
class _CustomCommand:
    class Type(enum.Enum):
        MACRO    = 0
        FUNCTION = 1
    type_:               Type
    name:                str
    parameters:          list[str]
    invocations:         list[_clp.Invoc]
    definition_position: _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainDefinedCommand:
    command:               _CertainCommand
    defining_command_name: str
    definition_position:   _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainCommand:
    reason: _CommandDefinitionUncertaintyReason | None


@dataclasses.dataclass(slots=True, frozen=True)
class _CommandDefinitionUncertaintyReason:
    defining_command_name:         str
    definition_position:           _cur.Position
    occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason


type _AssignedValue = _CertainAssignedValue | _cv.UncertainValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainAssignedValue(_cv.CertainValue):
    assigning_command_name: str
    assignment_position:    _cur.Position


class _UnsupportedInvocSyntaxException(Exception):
    def __init__(self, invoc: _clp.GeneralizedInvoc, *args: typing.Any) -> None:
        Exception.__init__(self, *args)
        self.invoc = invoc


class _CommandExecutionFailedException(Exception):
    def __init__(self, command_name: str, pos: int, message: str, *args: typing.Any) -> None:
        Exception.__init__(self)
        self.command_name = command_name
        self.pos          = pos
        self.message      = message
        self.args         = args


class _UncertainVariableResolutionException(Exception):
    def __init__(self, reason: _cur.ExpansionUncertaintyReason) -> None:
        Exception.__init__(self)
        self.reason = reason


class _UncertainPolicyException(Exception):
    def __init__(self, requesting_command: str, policy: _cpo.Policy, pos: int,
                 cause: _cur.ValueUncertaintyReason) -> None:
        Exception.__init__(self)
        self.requesting_command = requesting_command
        self.policy             = policy
        self.pos                = pos
        self.cause              = cause


type _ExpansionResult = _CertainExpansionResult | _UncertainExpansionResult

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainExpansionResult:
    string:     _tp.PosMappedString
    is_derived: bool

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainExpansionResult:
    reason: _cur.ExpansionUncertaintyReason


def _list_split(string: _tp.PosMappedString, is_derived: bool) -> tuple[list[_tp.PosMappedString], bool]:
    if not string.string:
        return [], True
    string_builder = _tp.PosMappedStringBuilder()
    parts = []
    def flush() -> None:
        parts.append(string_builder.finalize_and_get().map_through(string.pos_map))
    is_derived_2 = is_derived
    i = 0
    while True:
        string_builder.bump_ref_pos_to(i)
        j = string.string.find(";", i)
        if j == -1:
            string_builder.add_linear(string.string[i:])
            break
        is_derived_2 = True
        if j > i and string.string[j-1] == "\\":
            string_builder.add_linear(string.string[i:j-1])
            string_builder.add_nonlinear(";")
            i = j + 1
            continue
        string_builder.add_linear(string.string[i:j])
        flush()
        i = j + 1
        string_builder = _tp.PosMappedStringBuilder()
    flush()
    return parts, is_derived_2


_MESSAGE_LEVEL_MAP = {
    "FATAL_ERROR":    MessageLevel.FATAL_ERROR,
    "SEND_ERROR":     MessageLevel.SEND_ERROR,
    "WARNING":        MessageLevel.WARNING,
    "AUTHOR_WARNING": MessageLevel.AUTHOR_WARNING,
    "DEPRECATION":    MessageLevel.DEPRECATION,
    "NOTICE":         MessageLevel.NOTICE,
    "STATUS":         MessageLevel.STATUS,
    "VERBOSE":        MessageLevel.VERBOSE,
    "DEBUG":          MessageLevel.DEBUG,
    "TRACE":          MessageLevel.TRACE,
}


def _get_macro_substitutions(parameters: list[str], arguments: list[_ca.Argument]) -> dict[str, _SubstitutionValue]:
    substitutions = dict[str, _SubstitutionValue]()

    for i, param in enumerate(parameters):
        assert i < len(arguments)
        arg = arguments[i]
        match arg:
            case _ca.CertainArgument():
                substitutions[param] = _CertainSubstitutionValue(arg.string.string)
                continue
            case _ca.UncertainArgument():
                substitutions[param] = _UncertainSubstitutionValue(arg.reason)
                continue
        typing.assert_never(arg)

    # Special variables will shadow formal parameters of the same name. This is consistent
    # with CMake behavior.
    all_args   = list[str]()
    extra_args = list[str]()
    all_uncertainty:   _cur.ExpansionUncertaintyReason | None = None
    extra_uncertainty: _cur.ExpansionUncertaintyReason | None = None
    for i, arg in enumerate(arguments):
        name = "ARGV%s" % i
        match arg:
            case _ca.CertainArgument():
                substitutions[name] = _CertainSubstitutionValue(arg.string.string)
                all_args.append(arg.string.string)
                if i >= len(parameters):
                    extra_args.append(arg.string.string)
                continue
            case _ca.UncertainArgument():
                assert not arg.was_bare
                substitutions[name] = _UncertainSubstitutionValue(arg.reason)
                if not all_uncertainty:
                    all_uncertainty = arg.reason
                if i >= len(parameters):
                    if not extra_uncertainty:
                        extra_uncertainty = arg.reason
                continue
        typing.assert_never(arg)
    if not all_uncertainty:
        string = _cu.nonescaping_list_join(all_args) or ""
        substitutions["ARGV"] = _CertainSubstitutionValue(string)
    else:
        substitutions["ARGV"] = _UncertainSubstitutionValue(all_uncertainty)
    if not extra_uncertainty:
        string = _cu.nonescaping_list_join(extra_args) or ""
        substitutions["ARGN"] = _CertainSubstitutionValue(string)
    else:
        substitutions["ARGN"] = _UncertainSubstitutionValue(extra_uncertainty)
    substitutions["ARGC"] = _CertainSubstitutionValue(str(len(arguments)))

    return substitutions


def _macro_substitute_invoc(invoc: _clp.Invoc, context: _InvocContext,
                            substitutions: dict[str, _SubstitutionValue]) -> _clp.Invoc:
    def subst_invoc[T: _clp.GeneralizedInvoc](invoc: T) -> tuple[T, bool]:
        new_arguments, arguments_changed = subst_arguments(invoc)
        match invoc:
            case _clp.IfInvoc():
                new_children, children_changed = subst_children(invoc)
                new_closing_invoc, closing_invoc_cahnged = subst_invoc(invoc.closing_invoc)
                new_elseif_branches = list[_clp.IfBranch]()
                elseif_branches_changed = False
                for branch in invoc.elseif_branches:
                    new_branch, branch_changed = subst_invoc(branch)
                    new_elseif_branches.append(new_branch)
                    elseif_branches_changed |= branch_changed
                new_else_branch: _clp.IfBranch | None = None
                else_branch_changed = False
                if invoc.else_branch:
                    new_else_branch, else_branch_changed = subst_invoc(invoc.else_branch)
                if not arguments_changed and not children_changed and not elseif_branches_changed and \
                   not else_branch_changed and not closing_invoc_cahnged:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children,
                                           closing_invoc=new_closing_invoc, elseif_branches=new_elseif_branches,
                                           else_branch=new_else_branch), True
            case _clp.IfBranch():
                new_children, children_changed = subst_children(invoc)
                if not arguments_changed and not children_changed:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children), True
            case _clp.ForeachInvoc() | _clp.WhileInvoc() | _clp.MacroDefInvoc() | _clp.FunctionDefInvoc() | \
                 _clp.BlockInvoc():
                new_children, children_changed = subst_children(invoc)
                new_closing_invoc, closing_invoc_cahnged = subst_invoc(invoc.closing_invoc)
                if not arguments_changed and not children_changed and not closing_invoc_cahnged:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children,
                                           closing_invoc=new_closing_invoc), True
            case _clp.ClosingInvoc() | _clp.ReturnInvoc() | _clp.BreakInvoc() | _clp.ContinueInvoc() | \
                 _clp.GenericInvoc():
                if not arguments_changed:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments), True
        typing.assert_never(invoc)

    def subst_arguments(invoc: _clp.InvocBase) -> tuple[list[_clp.Protoargument], bool]:
        new_arguments = list[_clp.Protoargument]()
        arguments_changed = False
        for arg in invoc.arguments:
            match arg.type_:
                case _clp.ProtoargumentType.BARE | _clp.ProtoargumentType.QUOTED:
                    new_arg, arg_changed = subst_arg(arg, invoc)
                    new_arguments.append(new_arg)
                    arguments_changed |= arg_changed
                    continue
                case _clp.ProtoargumentType.BRACKETED:
                    # CMake does not apply macro substitutions inside bracketed arguments
                    new_arguments.append(arg)
                    continue
            typing.assert_never(arg.type_)
        if arguments_changed:
            return new_arguments, True
        return invoc.arguments, False

    def subst_arg(arg: _clp.Protoargument, invoc: _clp.InvocBase) -> tuple[_clp.Protoargument, bool]:
        match arg:
            case _clp.CertainProtoargument():
                is_changed = False
                is_derived = arg.is_derived
                builder = _tp.PosMapBuilder()
                pos = 0
                def replacer(m: re.Match[str]) -> str:
                    nonlocal is_changed, is_derived, pos
                    param = m.group(1)
                    value = substitutions.get(param)
                    if value is None:
                        return m.group(0)
                    match_pos = m.start()
                    match value:
                        case _CertainSubstitutionValue():
                            is_changed = True
                            is_derived = True
                            builder.add_linear(match_pos - pos, pos)
                            builder.add_nonlinear(len(value.string), match_pos)
                            pos = m.end()
                            return value.string
                        case _UncertainSubstitutionValue():
                            param_type = _cur.ParamType.MACRO_PARAM
                            ref_pos = arg.string.ref_pos(match_pos)
                            expansion_position = _cur.Position(context.file_index, ref_pos)
                            reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, param,
                                                                     expansion_position, value.reason)
                            raise _UncertainSubstitutionException(reason) from None
                    typing.assert_never(value)
                try:
                    new_string = _MACRO_SUBSTITUTE_REGEX.sub(replacer, arg.string.string)
                except _UncertainSubstitutionException as e:
                    return _clp.UncertainProtoargument(arg.type_, arg.orig_text, arg.pos, e.reason), True
                if not is_changed:
                    return arg, False
                builder.add_linear(len(arg.string.string) - pos, pos)
                pos_map = arg.string.pos_map.compose_with(builder.finalize_and_get())
                new_string_2 = _tp.PosMappedString(new_string, pos_map)
                return _clp.CertainProtoargument(arg.type_, arg.orig_text, arg.pos, new_string_2, is_derived), True
            case _clp.UncertainProtoargument():
                return arg, False
        typing.assert_never(arg)

    def subst_children(invoc: _clp.StructuredInvoc) -> tuple[list[_clp.Invoc], bool]:
        new_children = list[_clp.Invoc]()
        children_changed = False
        for child in invoc.children:
            new_child, child_changed = subst_invoc(child)
            new_children.append(new_child)
            children_changed |= child_changed
        if children_changed:
            return new_children, True
        return invoc.children, False

    new_invoc, invoc_changed = subst_invoc(invoc)
    return new_invoc


type _SubstitutionValue = _CertainSubstitutionValue | _UncertainSubstitutionValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainSubstitutionValue:
    string: str

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainSubstitutionValue:
    reason: _cur.ExpansionUncertaintyReason


class _UncertainSubstitutionException(Exception):
    def __init__(self, reason: _cur.ExpansionUncertaintyReason) -> None:
        Exception.__init__(self)
        self.reason = reason


_MACRO_SUBSTITUTE_REGEX = re.compile(r"\$\{([^{}]+)\}")


def _assign_function_arguments(command: _CustomCommand, arguments: list[_ca.Argument], state: _SubscopeState,
                               invoc: _clp.Invoc, context: _InvocContext) -> None:
    assignment_position = _cur.Position(context.file_index, invoc.pos)
    def set_certain_param(name: str, string: str) -> None:
        value = _CertainAssignedValue(string, invoc.command_name, assignment_position)
        state.set_regular_variable(name, value)
    def set_uncertain_param(name: str, reason: _cur.ExpansionUncertaintyReason) -> None:
        value = _cv.UncertainValue(reason)
        state.set_regular_variable(name, value)

    for i, param in enumerate(command.parameters):
        assert i < len(arguments)
        arg = arguments[i]
        match arg:
            case _ca.CertainArgument():
                set_certain_param(param, arg.string.string)
                continue
            case _ca.UncertainArgument():
                set_uncertain_param(param, arg.reason)
                continue
        typing.assert_never(arg)

    # Special variables will shadow formal parameters of the same name. This is consistent
    # with CMake behavior.
    all_args   = list[str]()
    extra_args = list[str]()
    all_uncertainty:   _cur.ExpansionUncertaintyReason | None = None
    extra_uncertainty: _cur.ExpansionUncertaintyReason | None = None
    for i, arg in enumerate(arguments):
        name = "ARGV%s" % i
        match arg:
            case _ca.CertainArgument():
                set_certain_param(name, arg.string.string)
                all_args.append(arg.string.string)
                if i >= len(command.parameters):
                    extra_args.append(arg.string.string)
                continue
            case _ca.UncertainArgument():
                assert not arg.was_bare
                set_uncertain_param(name, arg.reason)
                if not all_uncertainty:
                    all_uncertainty = arg.reason
                if i >= len(command.parameters):
                    if not extra_uncertainty:
                        extra_uncertainty = arg.reason
                continue
        typing.assert_never(arg)
    if not all_uncertainty:
        string = _cu.nonescaping_list_join(all_args) or ""
        set_certain_param("ARGV", string)
    else:
        set_uncertain_param("ARGV", all_uncertainty)
    if not extra_uncertainty:
        string = _cu.nonescaping_list_join(extra_args) or ""
        set_certain_param("ARGN", string)
    else:
        set_uncertain_param("ARGN", extra_uncertainty)
    set_certain_param("ARGC", str(len(arguments)))

    # Introspection parameters (these also shadow clashing formal parameters)
    file_pos = context.resolve_file_pos(command.definition_position)
    abs_path = context.resolve_path(file_pos.path)
    set_certain_param("CMAKE_CURRENT_FUNCTION", command.name)
    set_certain_param("CMAKE_CURRENT_FUNCTION_LIST_DIR", str(abs_path.parent))
    set_certain_param("CMAKE_CURRENT_FUNCTION_LIST_FILE", str(abs_path))
    set_certain_param("CMAKE_CURRENT_FUNCTION_LIST_LINE", str(file_pos.line_no))
