from __future__ import annotations

import typing
import abc
import dataclasses
import collections
import enum
import re
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp
import archon.cmake.variable as _cv
import archon.cmake.argument as _ca
import archon.cmake.condition as _cc


def process_file(cmake_path: pathlib.Path, application: Application, pos_resolver: PositionResolver,
                 logger: _l.Logger) -> bool:
    try:
        with open(cmake_path, "r") as file_:
            source = Source(file_, cmake_path)
            return process(source, application, pos_resolver, logger)
    except FileNotFoundError as e:
        logger.error("Failed to process %s: %s", _b.quote(str(cmake_path)), e.strerror)
        return False


def process(cmake_source: Source, application: Application, pos_resolver: PositionResolver, logger: _l.Logger) -> bool:
    return _process(cmake_source, application, pos_resolver, logger)


@dataclasses.dataclass(slots=True, frozen=True)
class Source:
    input_: typing.TextIO
    path:   pathlib.Path


class Application(abc.ABC):
    @abc.abstractmethod
    def message(self, pos: _cur.Position, occurrence_uncertainty: OccurrenceUncertainty, level: MessageLevel,
                message: str) -> None:
        ...


type OccurrenceUncertainty = _cur.ExpansionUncertaintyReason | None


class PositionResolver:
    def __init__(self) -> None:
        self._files = list[_SourceFile]()

    def resolve_text_pos(self, pos: _cur.Position) -> _tp.FullTextPos:
        return self._files[pos.file_index].pos_tracker.get_text_pos(pos.pos)

    def resolve_file_context(self, pos: _cur.Position) -> _tp.FileContext:
        return self._files[pos.file_index].pos_tracker.get_file_context(pos.pos)

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








def _process(cmake_source: Source, application: Application, pos_resolver: PositionResolver,
             logger: _l.Logger) -> bool:
    def process_file(cmake_source: Source, state: _State, occurrence_uncertainty: OccurrenceUncertainty,
                     base_path: pathlib.Path) -> None:
        tracker = _tp.FilePosTracker(cmake_source.path)
        file_index = pos_resolver._append_file(_SourceFile(tracker))
        context = _InvocContext(file_index, state, occurrence_uncertainty, base_path)
        def warning_handler(pos: int, message: str, *args: typing.Any) -> None:
            warning(file_index, pos, message, *args)
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
            error(context.file_index, e.invoc.pos, "Unsupported %s() syntax", e.invoc.command_name)
            return
        except _ConditionParseError as e:
            error(context.file_index, e.pos, "Failed to parse %s() condition: %s", e.command_name, e.message)
            return
        except _ConditionEvalError as e:
            error(context.file_index, e.pos, "Failed to evaluate %s() condition: %s", e.command_name, e.message)
            return
        except _ca.UncertainArgumentException as e:
            position = e.reason.expansion_position
            error(position.file_index, position.pos, "Failed to invoke %s() due to expansion of variable %s with "
                  "uncertain value", e.reason.command_name, _b.quote(e.reason.variable_name))
            if e.reason.value_uncertainty_reason:
                trace_value_uncertainty_causes(e.reason.value_uncertainty_reason)
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
                    if_context = _InvocContext(context.file_index, if_state, occurrence_uncertainty, context.base_path)
                    exec_commands(subinvoc.children, if_context)
                    else_state = _OccurrenceUncertaintyOverlayState(context.state, result.reason)
                    else_context = _InvocContext(context.file_index, else_state, occurrence_uncertainty,
                                                 context.base_path)
                    exec_else(else_context)
                    if_state.push_taints()
                    else_state.push_taints()
                    return
            typing.assert_never(result)
        exec_level(invoc, context, 0)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        def iterate_list(loop_var: str, items: list[str]) -> None:
            for item in items:
                state = _VariableOverlayState(context.state, {
                    loop_var: item,
                })
                exec_commands(invoc.children, context.with_state(state))
        server = create_argument_server(invoc, context)
        i = server.find_keyword({"IN"})
        if i == -1:
            loop_var = server.consume()
            if not loop_var:
                error(context.file_index, server.next_pos(), "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if server.consume_keyword({"RANGE"}):
                raise _UnsupportedInvocSyntaxException(invoc) from None
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
                error(context.file_index, server.next_pos(), "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if len(loop_vars) > 1:
                error(context.file_index, server.next_pos(), "Too many loop variables in %s() invocation",
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
                    value = resolve_variable(_cu.ResolutionType.GENERAL, var_name, arg.pos, context)
                    match value:
                        case _cv.CertainValue():
                            string = value.string or ""
                        case _cv.UncertainValue():
                            expansion_position = _cur.Position(context.file_index, arg.pos)
                            reason = _cur.ExpansionUncertaintyReason(invoc.command_name, var_name, expansion_position,
                                                                     value.reason)
                            raise _ca.UncertainArgumentException(reason)
                        case _:
                            typing.assert_never(value)
                    items += _cu.list_split(string)
                    continue
                assert False
            iterate_list(loop_vars[0], items)
            return
        if arg.string.string == "ZIP_LISTS":
            raise _UnsupportedInvocSyntaxException(invoc) from None
        error(context.file_index, arg.pos, "Unrecognized keyword (%s) after IN in %s() invocation",
              _b.quote(arg.string.string), invoc.command_name)

    def exec_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_macro_def(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        name = server.consume()
        if not name:
            error(context.file_index, server.next_pos(), "Missing macro name in %s() invocation", invoc.command_name)
            return
        name_cf = name.string.string.casefold()
        if _clp.is_flow_control_command(name_cf):
            error(context.file_index, name.pos, "Failed to define macro %s(): Built-in flow control commands cannot "
                  "be overridden", name.string.string)
            return
        parameters = []
        while True:
            arg = server.consume()
            if not arg:
                break
            parameters.append(arg.string.string)
        # FIXME: "Push" old definition, if any, to same name but with underscore prefix          
        defining_command_name = invoc.command_name
        definition_position = _cur.Position(context.file_index, invoc.pos)
        command = _CustomCommand(_CustomCommand.Type.MACRO, parameters, invoc.children, definition_position)
        context.state.set_command(name_cf, command, defining_command_name, definition_position)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_function_def(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_return(invoc: _clp.ReturnInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if not server.at_end():
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
                    case _BuiltInCommand.Which.MESSAGE:
                        exec_message(invoc, context)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        exec_include(invoc, context)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        exec_add_subdirectory(invoc, context)
                        return
                typing.assert_never(which)
            case _CustomCommand():
                arguments = expand_arguments(invoc, context)
                match command.type_:
                    case _CustomCommand.Type.MACRO:
                        if len(arguments) < len(command.parameters):
                            error(context.file_index, invoc.rparen_pos, "Too few arguments in invocation of "
                                  "macro %s()", invoc.command_name)
                            position = command.definition_position
                            error(position.file_index, position.pos, "Definition of macro %s()", invoc.command_name)
                            return
                        substitutions = _get_macro_substitutions(command.parameters, arguments)
                        for subinvoc in command.invocations:
                            subinvoc_2 = _macro_substitute_invoc(subinvoc)
                            exec_command(subinvoc_2, context)
                        return
                    case _CustomCommand.Type.FUNCTION:
                        assert False                        
                typing.assert_never(command.type_)
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
        # CMake ignores arguments in a closing invocation but generates a warning unless the
        # closing invocation is either empty or matches the corresponding opening invocation
        # proto-argument for proto-argument. if() is the corresponding opening invocation
        # for endif() even if there are elseif() and/or else() invocations in between.
        if not invoc.arguments:
            return
        opening_args = [a.text for a in opening_invoc.arguments]
        closing_args = [a.text for a in invoc.arguments]
        if opening_args != closing_args:
            opening_position = _cur.Position(context.file_index, opening_invoc.pos)
            opening_line_no = pos_resolver.resolve_text_pos(opening_position).line_no
            warning(context.file_index, invoc.pos, "Closing invocation, %s(), has mismatching arguments (opening "
                    "invocation is on line %s)", invoc.command_name, opening_line_no)

    def exec_set(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern
        variable = server.consume()
        if not variable:
            error(context.file_index, server.next_pos(), "Missing variable name in %s() invocation",
                  invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(variable.string.string)
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                pass
            case _cu.ResolutionType.CACHE | _cu.ResolutionType.ENV:
                raise _UnsupportedInvocSyntaxException(invoc) from None
            case _:
                typing.assert_never(var_ref.resolution_type)
        var_name = var_ref.variable_name
        values = []
        parent_scope = False
        try:
            while True:
                arg = server.consume()
                if not arg:
                    break
                if arg.string.string == "CACHE":
                    raise _UnsupportedInvocSyntaxException(invoc) from None
                if server.at_end() and arg.string.string == "PARENT_SCOPE":
                    parent_scope = True
                    break
                values.append(arg.string.string)
        except _ca.UncertainArgumentException as e:
            context.state.taint_regular_variable(var_name, e.reason, parent_scope=False)
            if not context.state.is_root_scope():
                context.state.taint_regular_variable(var_name, e.reason, parent_scope=True)
            return
        value = ";".join(values) if values else None
        set_regular_variable(var_name, value, parent_scope, invoc, context)

    def exec_unset(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        variable = server.consume()
        if not variable:
            error(context.file_index, server.next_pos(), "Missing variable name in %s() invocation",
                  invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(variable.string.string)
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                pass
            case _cu.ResolutionType.CACHE | _cu.ResolutionType.ENV:
                raise _UnsupportedInvocSyntaxException(invoc) from None
            case _:
                typing.assert_never(var_ref.resolution_type)
        var_name = var_ref.variable_name
        parent_scope = False
        try:
            arg = server.consume()
            if arg:
                if arg.string.string == "CACHE":
                    raise _UnsupportedInvocSyntaxException(invoc) from None
                if server.at_end() and arg.string.string == "PARENT_SCOPE":
                    parent_scope = True
                else:
                    error(context.file_index, server.next_pos(), "Too many arguments in %s() invocation",
                          invoc.command_name)
        except _ca.UncertainArgumentException as e:
            context.state.taint_regular_variable(var_name, e.reason, parent_scope=False)
            if not context.state.is_root_scope():
                context.state.taint_regular_variable(var_name, e.reason, parent_scope=True)
            return
        value = None
        set_regular_variable(var_name, value, parent_scope, invoc, context)

    def exec_message(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedInvocSyntaxException(invoc) from None
        level = MessageLevel.NOTICE
        arg = server.consume_keyword(_MESSAGE_LEVEL_MAP.keys())
        if arg:
            level = _MESSAGE_LEVEL_MAP[arg.string.string]
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
        file_or_module = server.consume()
        if not file_or_module:
            error(context.file_index, server.next_pos(), "Missing file or module in %s()", invoc.command_name)
            return None
        if not re.fullmatch(r".*\.cmake", file_or_module.string.string):
            raise _UnsupportedInvocSyntaxException(invoc) from None
        if not server.at_end():
            raise _UnsupportedInvocSyntaxException(invoc) from None
        cmake_path = context.base_path.parent / file_or_module.string.string
        try:
            with open(cmake_path, "r") as file_:
                cmake_source = Source(file_, cmake_path)
                process_file(cmake_source, context.state, context.occurrence_uncertainty, context.base_path)
        except FileNotFoundError as e:
            error(context.file_index, invoc.pos, "Failed to include %s: %s", _b.quote(str(cmake_path)), e.strerror)

    def exec_add_subdirectory(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        assert False        

    def evaluate_condition(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _cc.Result:
        try:
            arguments = expand_arguments(invoc, context)
            condition = _cc.parse(arguments, invoc.rparen_pos)
            class State(_cv.VariableState):
                @typing.override
                def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> _cv.Value:
                    return resolve_variable(resolution_type, variable_name, pos, context)
                @typing.override
                def set_(self, variable_name: str, value: str | None) -> None:
                    parent_scope = False
                    assigning_command_name = invoc.command_name
                    assignment_position = _cur.Position(context.file_index, invoc.pos)
                    context.state.set_regular_variable(variable_name, value, parent_scope, assigning_command_name,
                                                       assignment_position)
                @typing.override
                def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
                    context.state.taint_regular_variable(variable_name, reason, parent_scope=False)
            state = State()
            return _cc.evaluate(condition, invoc.command_name, context.file_index, state)
        except _cc.FatalParseError as e:
            raise _ConditionParseError(e.pos, invoc.command_name, e.message % e.args) from None
        except _cc.FatalEvalError as e:
            raise _ConditionEvalError(e.pos, invoc.command_name, e.message % e.args) from None

    def create_argument_server(invoc: _clp.Invoc, context: _InvocContext) -> _ca.ArgumentServer:
        arguments = expand_arguments(invoc, context)
        return _ca.ArgumentServer(invoc, arguments, context.file_index)

    def expand_arguments(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> list[_ca.Argument]:
        arguments = list[_ca.Argument]()
        for protoarg in invoc.arguments:
            i = protoarg.prefix_size
            j = len(protoarg.text) - protoarg.suffix_size
            string = protoarg.text[i:j]
            pos = protoarg.pos + i
            arg: _ca.Argument
            match protoarg.type_:
                case _clp.Protoargument.Type.BARE:
                    was_bare = True
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            segments, is_derived = _list_split(result.string, result.is_derived)
                            for segment in segments:
                                arg = _ca.CertainArgument(protoarg.pos, was_bare, segment, is_derived)
                                arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            # Note that an uncertain unquoted proto-argument stands in for
                            # any number of actual arguments, including zero.
                            arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                            arguments.append(arg)
                            continue
                    typing.assert_never(result)
                case _clp.Protoargument.Type.QUOTED:
                    was_bare = False
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            arg = _ca.CertainArgument(protoarg.pos, was_bare, result.string, result.is_derived)
                            arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                            arguments.append(arg)
                            continue
                    typing.assert_never(result)
                case _clp.Protoargument.Type.BRACKETED:
                    was_bare = False
                    string_2 = _tp.PosMappedString.from_linear_string(string, pos)
                    is_derived = False
                    arg = _ca.CertainArgument(protoarg.pos, was_bare, string_2, is_derived)
                    arguments.append(arg)
                    continue
            typing.assert_never(protoarg.type_)
        return arguments

    def expand_string(string: str, pos: int, invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _ExpansionResult:
        def expand(expr: _csp.Expr) -> _ExpansionResult:
            if isinstance(expr, _csp.StringExpr):
                is_derived = False
                return _CertainExpansionResult(expr.string, is_derived)
            if isinstance(expr, _csp.CompositeExpr):
                string_builder = _tp.PosMappedStringBuilder(expr.pos)
                for part in expr.parts:
                    result = expand(part)
                    if isinstance(result, _CertainExpansionResult):
                        string_builder.add_pos_mapped_string(result.string)
                        continue
                    if isinstance(result, _UncertainExpansionResult):
                        return result
                    typing.assert_never(result)
                is_derived = True
                return _CertainExpansionResult(string_builder.get(), is_derived)
            if isinstance(expr, _csp.ExpansionExpr):
                result = expand(expr.name_expr)
                if isinstance(result, _CertainExpansionResult):
                    name = result.string.string
                    value = resolve_variable(expr.resolution_type, name, expr.name_expr.pos, context)
                    if isinstance(value, _cv.CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _cv.UncertainValue):
                        position = _cur.Position(context.file_index, expr.pos)
                        reason = _cur.ExpansionUncertaintyReason(invoc.command_name, name, position, value.reason)
                        return _UncertainExpansionResult(reason)
                    typing.assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result
                typing.assert_never(result)
            typing.assert_never(expr)
        def error_handler(pos: int, message: str, *args: typing.Any) -> None:
            error(context.file_index, pos, message, *args)
        return expand(_csp.parse(string, pos, error_handler))

    def resolve_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                         context: _InvocContext) -> _cv.Value:
        match resolution_type:
            case _cu.ResolutionType.GENERAL:
                value = context.state.get_regular_variable(variable_name)
                match value:
                    case _cv.CertainValue(string):
                        if string is not None:
                            return value
                    case _cv.UncertainValue():
                        return value
                    case _:
                        typing.assert_never(value)
                return context.state.get_cache_variable(variable_name)
            case _cu.ResolutionType.CACHE:
                return context.state.get_cache_variable(variable_name)
            case _cu.ResolutionType.ENV:
                return context.state.get_environment_variable(variable_name)
        typing.assert_never(resolution_type)

    def set_regular_variable(variable_name: str, value: str | None, parent_scope: bool, invoc: _clp.GeneralizedInvoc,
                             context: _InvocContext) -> None:
        if parent_scope and context.state.is_root_scope():
            warning(context.file_index, invoc.pos, "%s() invocation skipped: No parent scope exists",
                    invoc.command_name)
            return
        assigning_command_name = invoc.command_name
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        context.state.set_regular_variable(variable_name, value, parent_scope, assigning_command_name,
                                           assignment_position)

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
        typing.assert_never(cause)

    def trace_expansion_uncertainty_causes(cause: _cur.ExpansionUncertaintyReason) -> None:
        position = cause.expansion_position
        error(position.file_index, position.pos, "Caused by expansion of variable %s with uncertain value in "
              "invocation of %s()", _b.quote(cause.variable_name), cause.command_name)
        if cause.value_uncertainty_reason:
            trace_value_uncertainty_causes(cause.value_uncertainty_reason)

    def warning(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        context = pos_resolver.resolve_file_context(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).warn(message, *args)

    errors_seen = False
    def error(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        nonlocal errors_seen
        errors_seen = True
        context = pos_resolver.resolve_file_context(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).error(message, *args)

    state = _RootState()
    occurrence_uncertainty = None
    base_path = cmake_source.path
    process_file(cmake_source, state, occurrence_uncertainty, base_path)
    return not errors_seen


@dataclasses.dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


@dataclasses.dataclass(slots=True, frozen=True)
class _InvocContext:
    file_index:             int
    state:                  _State
    occurrence_uncertainty: OccurrenceUncertainty
    base_path:              pathlib.Path

    def with_state(self, state: _State) -> _InvocContext:
        return _InvocContext(self.file_index, state, self.occurrence_uncertainty, self.base_path)


class _State(abc.ABC):
    @abc.abstractmethod
    def is_root_scope(self) -> bool:
        ...

    @abc.abstractmethod
    def get_regular_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        ...

    @abc.abstractmethod
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        ...

    @abc.abstractmethod
    def get_cache_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def get_environment_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def get_command(self, name_cf: str) -> _Command:
        ...

    @abc.abstractmethod
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        ...

    @abc.abstractmethod
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        ...


class _RootState(_State):
    def __init__(self) -> None:
        self._commands    = dict[str, _Command]()
        self._environment = _Environment()
        self._cache       = _Cache()
        self._directory   = _Directory()
        _define_built_in_commands(self._commands)

    @typing.override
    def is_root_scope(self) -> bool:
        return True

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        return self._directory.get_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        assert not parent_scope
        self._directory.set_variable(name, value)

    @typing.override
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        assert not parent_scope
        self._directory.taint_variable(name, reason)

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        assert False    

    @typing.override
    def get_environment_variable(self, name: str) -> _cv.Value:
        assert False    

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            return command
        reason = None
        return _UncertainCommand(reason)

    @typing.override
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        self._commands[name_cf] = command

    @typing.override
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        self._commands[name_cf] = _UncertainCommand(reason)


class _VariableOverlayState(_State):
    def __init__(self, parent_state: _State, variables: dict[str, str]) -> None:
        self._parent_state = parent_state
        self._variables    = dict[str, _cv.Value]()
        for name, value in variables.items():
            self._variables[name] = _cv.CertainValue(value)

    @typing.override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._variables.get(name)
        if value is not None:
            return value
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        if not parent_scope and name in self._variables:
            self._variables[name] = _cv.CertainValue(value)
            return
        self._parent_state.set_regular_variable(name, value, parent_scope, assigning_command_name, assignment_position)

    # FIXME: Generally do not replace a taint with a later one (retain the taint that occurs first)                   
    @typing.override
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        if not parent_scope and name in self._variables:
            self._variables[name] = _cv.UncertainValue(reason)
            return
        self._parent_state.taint_regular_variable(name, reason, parent_scope)

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_cache_variable(name)

    @typing.override
    def get_environment_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_environment_variable(name)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        self._parent_state.set_command(name_cf, command, defining_command_name, definition_position)

    @typing.override
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        self._parent_state.taint_command(name_cf, reason)


class _OccurrenceUncertaintyOverlayState(_State):
    def __init__(self, parent_state: _State, occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason) -> None:
        self._parent_state                   = parent_state
        self._occurrence_uncertainty_reason  = occurrence_uncertainty_reason
        self._commands                       = dict[str, _Command]()
        self._tainted_commands               = dict[str, _CommandDefinitionUncertaintyReason]()
        self._regular_variables              = dict[str, _cv.Value]()
        self._tainted_regular_variables      = dict[str, _cur.ValueUncertaintyReason]()
        self._tainted_parent_scope_variables = dict[str, _cur.ValueUncertaintyReason]()

    def push_taints(self) -> None:
        reason: typing.Any
        for name_cf, reason in self._tainted_commands.items():
            self._parent_state.taint_command(name_cf, reason)
        for name, reason in self._tainted_regular_variables.items():
            self._parent_state.taint_regular_variable(name, reason, parent_scope=False)
        for name, reason in self._tainted_parent_scope_variables.items():
            self._parent_state.taint_regular_variable(name, reason, parent_scope=True)

    @typing.override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            return value
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        reason = _cur.AssignmentOccurrenceUncertaintyReason(assigning_command_name, assignment_position,
                                                            self._occurrence_uncertainty_reason)
        if parent_scope:
            self._tainted_parent_scope_variables[name] = reason
        else:
            self._regular_variables[name] = _cv.CertainValue(value)
            self._tainted_regular_variables[name] = reason

    @typing.override
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        if parent_scope:
            self._tainted_parent_scope_variables[name] = reason
        else:
            self._regular_variables[name] = _cv.UncertainValue(reason)
            self._tainted_regular_variables[name] = reason

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        assert False    

    @typing.override
    def get_environment_variable(self, name: str) -> _cv.Value:
        assert False    

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            return command
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        self._commands[name_cf] = command
        reason = _CommandDefinitionUncertaintyReason(defining_command_name, definition_position,
                                                     self._occurrence_uncertainty_reason)
        self._tainted_commands[name_cf] = reason

    @typing.override
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        self._commands[name_cf] = _UncertainCommand(reason)
        self._tainted_commands[name_cf] = reason


type _Command = _CertainCommand | _UncertainCommand

type _CertainCommand = _BuiltInCommand | _CustomCommand

@dataclasses.dataclass(slots=True, frozen=True)
class _BuiltInCommand:
    class Which(enum.Enum):
        UNSUPPORTED      = 0
        SET              = 1
        UNSET            = 2
        MESSAGE          = 3
        INCLUDE          = 4
        ADD_SUBDIRECTORY = 5
    which: Which

@dataclasses.dataclass(slots=True, frozen=True)
class _CustomCommand:
    class Type(enum.Enum):
        MACRO    = 0
        FUNCTION = 1
    type_:               Type
    parameters:          list[str]
    invocations:         list[_clp.Invoc]
    definition_position: _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainCommand:
    reason: _CommandDefinitionUncertaintyReason | None


@dataclasses.dataclass(slots=True, frozen=True)
class _CommandDefinitionUncertaintyReason:
    defining_command_name:         str
    definition_position:           _cur.Position
    occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason


def _define_built_in_commands(commands: dict[str, _Command]) -> None:
    def define(name_cf: str, which: _BuiltInCommand.Which) -> None:
        commands[name_cf] = _BuiltInCommand(which)
    define("set",                    _BuiltInCommand.Which.SET)
    define("unset",                  _BuiltInCommand.Which.UNSET)
    define("message",                _BuiltInCommand.Which.MESSAGE)
    define("include",                _BuiltInCommand.Which.INCLUDE)
    define("add_subdirectory",       _BuiltInCommand.Which.ADD_SUBDIRECTORY)
    define("cmake_minimum_required", _BuiltInCommand.Which.UNSUPPORTED)
    define("project",                _BuiltInCommand.Which.UNSUPPORTED)
    define("option",                 _BuiltInCommand.Which.UNSUPPORTED)


class _Environment:
    def resolve(self, name: str) -> _cv.Value:
        assert False        


class _Cache:
    def resolve(self, name: str) -> _cv.Value:
        assert False        


class _Directory:
    def __init__(self, parent: _Directory | None = None) -> None:
        self._parent            = parent
        self._variables         = dict[str, _cv.CertainValue]()
        self._tainted_variables = dict[str, _cur.ValueUncertaintyReason]()

    @property
    def parent(self) -> _Directory | None:
        return self._parent

    def get_variable(self, name: str) -> _cv.Value:
        reason: _cur.ValueUncertaintyReason | None
        reason = self._tainted_variables.get(name)
        if reason:
            return _cv.UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            if value.string is None and self._parent:
                return self._parent.get_variable(name)
            return value
        reason = None
        return _cv.UncertainValue(reason)

    def set_variable(self, variable_name: str, value: str | None) -> None:
        self._variables[variable_name] = _cv.CertainValue(value)
        self._tainted_variables.pop(variable_name, None)

    def taint_variable(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        self._variables.pop(variable_name, None)
        self._tainted_variables[variable_name] = reason


class _UnsupportedInvocSyntaxException(Exception):
    def __init__(self, invoc: _clp.GeneralizedInvoc) -> None:
        self.invoc = invoc


class _ConditionParseError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


class _ConditionEvalError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


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
        string_2 = string_builder.get()
        pos_map = string.pos_map.compose_with(string_2.pos_map)
        parts.append(_tp.PosMappedString(string_2.string, pos_map))
    is_derived_2 = is_derived
    i = 0
    while True:
        j = string.string.find(";", i)
        if j == -1:
            string_builder.add_linear(string.string[i:], i)
            break
        is_derived_2 = True
        if j > i and string.string[j-1] == "\\":
            string_builder.add_linear(string.string[i:j-1], i)
            string_builder.add_nonlinear(";", j - 1)
            i = j + 1
            continue
        string_builder.add_linear(string.string[i:j], i)
        flush()
        i = j + 1
        string_builder = _tp.PosMappedStringBuilder(i)
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
    all_args   = []
    extra_args = []
    all_uncertainty:   _cur.ExpansionUncertaintyReason | None = None
    extra_uncertainty: _cur.ExpansionUncertaintyReason | None = None
    for arg in arguments:
        name = "ARGV%s" % i
        match arg:
            case _ca.CertainArgument():
                substitutions[name] = _CertainSubstitutionValue(arg.string.string)
                all_args.append(arg.string.string)
                if i >= len(parameters):
                    extra_args.append(arg.string.string)
                continue
            case _ca.UncertainArgument():
                substitutions[name] = _UncertainSubstitutionValue(arg.reason)
                if not all_uncertainty:
                    all_uncertainty = arg.reason
                if i >= len(parameters):
                    if not extra_uncertainty:
                        extra_uncertainty = arg.reason
                continue
        typing.assert_never(arg)
    if all_uncertainty:
        substitutions["ARGV"] = _UncertainSubstitutionValue(all_uncertainty)
    else:
        substitutions["ARGV"] = _CertainSubstitutionValue(";".join(all_args))
    if extra_uncertainty:
        substitutions["ARGN"] = _UncertainSubstitutionValue(extra_uncertainty)
    else:
        substitutions["ARGN"] = _CertainSubstitutionValue(";".join(extra_args))
    substitutions["ARGC"] = _CertainSubstitutionValue(str(len(arguments)))

    return substitutions


def _macro_substitute_invoc(invoc: _clp.Invoc) -> _clp.Invoc:
    assert False        


type _SubstitutionValue = _CertainSubstitutionValue | _UncertainSubstitutionValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainSubstitutionValue:
    string: str

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainSubstitutionValue:
    reason: _cur.ExpansionUncertaintyReason
