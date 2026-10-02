from __future__ import annotations
from typing import Any, Never, assert_never
from collections.abc import Iterable
from dataclasses import dataclass

import enum
import copy
import re
import pathlib

import base
import log
import cmake_utils as _cu
import lowlevel_cmake_parser as _lcp
import cmake_invoc_parser as _cip
import cmake_regex as _cr


# Re-export
LibraryType = _cip.LibraryType
Scope       = _cip.Scope
Path        = _cip.Path
ScopedPath  = _cip.ScopedPath
LinkItem    = _cip.LinkItem


def extract_structure(cmake_dir_path: pathlib.Path, logger: log.Logger, emit_messages: bool) -> Group | None:
    return _extract_structure(cmake_dir_path, logger, emit_messages)


def dump_structure(group: Group, logger: log.Logger, log_level: log.LogLevel) -> None:
    if logger.will_log(log_level):
        _dump_structure(group, logger, log_level)


class Group:
    def __init__(self, cmake_path: pathlib.Path) -> None:
        self.cmake_path = cmake_path
        self.targets:   list[Target]   = []
        self.subgroups: list[Subgroup] = []

class Subgroup(Group):
    class Type(enum.Enum):
        INCLUSION = 1
        SUBDIR    = 2
    def __init__(self, type_: Subgroup.Type, line_no: int, cmake_path: pathlib.Path) -> None:
        Group.__init__(self, cmake_path)
        self.type_   = type_
        self.line_no = line_no


type Target = LibraryTarget | ExecutableTarget

class TargetBase:
    def __init__(self, cmake_name: str, source_files: Iterable[Path], line_no: int, closing_line_no: int) -> None:
        self.cmake_name      = cmake_name
        self.source_files    = list(source_files)
        self.line_no         = line_no
        self.closing_line_no = closing_line_no
        self.source_file_sets: list[SourceFileSet] = []
        self.header_file_sets: list[HeaderFileSet] = []
        self.link_item_sets:   list[LinkItemSet]   = []
        self.output_name: str | None = None

class LibraryTarget(TargetBase):
    def __init__(self, cmake_name: str, library_type: LibraryType, source_files: Iterable[Path], line_no: int,
                 closing_line_no: int) -> None:
        TargetBase.__init__(self, cmake_name, source_files, line_no, closing_line_no)
        self.library_type = library_type

class ExecutableTarget(TargetBase):
    def __init__(self, cmake_name: str, source_files: Iterable[Path], line_no: int, closing_line_no: int) -> None:
        TargetBase.__init__(self, cmake_name, source_files, line_no, closing_line_no)


class SourceFileSet:
    def __init__(self, subgroup_path: Iterable[int], files: Iterable[ScopedPath], line_no: int,
                 closing_line_no: int) -> None:
        self.subgroup_path   = list(subgroup_path)
        self.files           = list(files)
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class HeaderFileSet:
    def __init__(self, subgroup_path: Iterable[int], scope: Scope, name: str, base_dirs: Iterable[Path],
                 files: Iterable[Path], line_no: int, closing_line_no: int) -> None:
        self.subgroup_path   = list(subgroup_path)
        self.scope           = scope
        self.name            = name
        self.base_dirs       = list(base_dirs)
        self.files           = list(files)
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class LinkItemSet:
    def __init__(self, subgroup_path: Iterable[int], items: Iterable[LinkItem], line_no: int,
                 closing_line_no: int) -> None:
        self.subgroup_path   = list(subgroup_path)
        self.items           = list(items)
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


def _extract_structure(root_dir_path: pathlib.Path, logger: log.Logger, emit_messages: bool) -> Group | None:
    ignore_commands = {
        "cmake_minimum_required",
        "project",  
        "option",  
        "add_compile_options",
        "add_link_options",
        "add_test",
    }

    def include_resolver(file_or_module: str) -> pathlib.Path | None:
        if re.fullmatch(r".*\.cmake", file_or_module):
            return pathlib.Path(file_or_module)
        return None

    errors_seen = False
    def error_handler(context: log.FileContext, message: str, *args: Any) -> None:
        nonlocal errors_seen
        log.FileContextLogger(logger, context).error(message, *args)
        errors_seen = True

    invoc_parser = _cip.Parser(ignore_commands, include_resolver, error_handler)

    target_map: dict[str, Target] = {}

    conditional_stack = list[_ConditionalState]()
    conditional_state = _ConditionalState()

    def process_group(cmake_path: pathlib.Path, directory: _Directory, group: Group, subgroup_path: list[int]) -> None:
        nonlocal conditional_state
        base_conditional_level = len(conditional_stack)
        def resolve(path: pathlib.Path) -> pathlib.Path:
            return directory.path.parent / path
        def lowlevel_error_handler(line_no: int, message: str, *args: Any) -> None:
            context = log.FileContext(cmake_path, line_no)
            error_handler(context, message, *args)
        for protoinvoc in _lcp.parse(cmake_path, lowlevel_error_handler):
            def variable_resolver(var_type: _cip.VariableType, var_name: str, line_no: int) -> _cip.Value:
                if var_type == _cip.VariableType.REGULAR:
                    return directory.resolve_variable(var_name)
                context = log.FileContext(cmake_path, line_no)
                error_handler(context, "Unsupported variable type (%s)", var_type.name)
                raise _ParseError
            try:
                invoc = invoc_parser.parse(cmake_path, protoinvoc, variable_resolver)
            except _cip.UnparsableSetInvocException as e:
                directory.taint_variable(e.reason.variable_uncertainty_reason)
                if directory.parent:
                    directory.parent.taint_variable(e.reason.variable_uncertainty_reason)
                continue
            if not invoc:
                continue

            target: Target | None
            variable_uncertainty_reason: _cip.VariableUncertaintyReason | None

            if isinstance(invoc, _cip.SetInvoc):
                set_variable(invoc.variable.value, invoc.values, invoc.parent_scope, directory,
                             cmake_path, invoc.line_no)
                continue

            if isinstance(invoc, _cip.UnsetInvoc):
                values = list[_cip.String]()
                set_variable(invoc.variable.value, values, invoc.parent_scope, directory, cmake_path, invoc.line_no)
                continue

            if isinstance(invoc, _cip.MessageInvoc):
                if conditional_state.effective == base.Tristate.FALSE:
                    continue
                uncertain = conditional_state.effective == base.Tristate.UNCERTAIN
                if emit_messages:
                    certainty = "Uncertain" if uncertain else "Certain"
                    context = log.FileContext(cmake_path, invoc.line_no)
                    context_logger = log.FileContextLogger(logger, context)
                    context_logger.info("%s: Message(%s): %s", certainty, invoc.level.name, invoc.message)
                continue

            if isinstance(invoc, _cip.LinkLibrariesInvoc):
                # FIXME: Skip over here and below when conditional_status is FALSE
                # FIXME: In general, mark a section as "tainted" if its occurrence is based on
                # an uncertain condition        
                link_item_set = LinkItemSet(subgroup_path, invoc.items, invoc.line_no, invoc.closing_line_no)
                directory.add_link_item_set(link_item_set)
                continue

            if isinstance(invoc, _cip.AddLibraryInvoc):
                source_files = []
                for path in invoc.source_files:
                    source_files.append(Path(resolve(path.path), path.line_no))
                target = LibraryTarget(invoc.target_name, invoc.library_type, source_files, invoc.line_no,
                                       invoc.closing_line_no)
                target.link_item_sets += directory.get_link_item_sets()
                group.targets.append(target)
                target_map[invoc.target_name] = target
                continue

            if isinstance(invoc, _cip.AddExecutableInvoc):
                source_files = []
                for path in invoc.source_files:
                    source_files.append(Path(resolve(path.path), path.line_no))
                target = ExecutableTarget(invoc.target_name, source_files, invoc.line_no, invoc.closing_line_no)
                target.link_item_sets += directory.get_link_item_sets()
                group.targets.append(target)
                target_map[invoc.target_name] = target
                continue

            if isinstance(invoc, _cip.RegularTargetSourcesInvoc):
                target = target_map.get(invoc.target_name)
                if not target:
                    fatal(cmake_path, invoc.line_no, "Undefined target (%s)", invoc.target_name)
                scoped_files = []
                for path in invoc.scoped_source_files:
                    scoped_files.append(ScopedPath(path.scope, resolve(path.path), path.line_no))
                    target.source_file_sets.append(SourceFileSet(subgroup_path, scoped_files, invoc.line_no,
                                                                 invoc.closing_line_no))
                continue

            if isinstance(invoc, _cip.FileSetTargetSourcesInvoc):
                target = target_map.get(invoc.target_name)
                if not target:
                    fatal(cmake_path, invoc.line_no, "Undefined target (%s)", invoc.target_name)
                for file_set in invoc.file_sets:
                    if file_set.type_ != _cip.FileSet.Type.HEADERS:
                        fatal(cmake_path, file_set.line_no, "Unsupported file set type (%s)", file_set.type_.name)
                    base_dirs = []
                    for path in file_set.base_dirs:
                        base_dirs.append(Path(resolve(path.path), path.line_no))
                    files = []
                    for path in file_set.files:
                        files.append(Path(resolve(path.path), path.line_no))
                    header_file_set = HeaderFileSet(subgroup_path, file_set.scope, file_set.name, base_dirs, files,
                                                    file_set.line_no, file_set.closing_line_no)
                    target.header_file_sets.append(header_file_set)
                continue

            if isinstance(invoc, _cip.TargetLinkLibrariesInvoc):
                target = target_map.get(invoc.target_name)
                if not target:
                    fatal(cmake_path, invoc.line_no, "Undefined target (%s)", invoc.target_name)
                link_item_set = LinkItemSet(subgroup_path, invoc.items, invoc.line_no, invoc.closing_line_no)
                target.link_item_sets.append(link_item_set)
                continue

            if isinstance(invoc, _cip.SetTargetPropertiesInvoc):
                for target_2 in invoc.targets:
                    target = target_map.get(target_2.name)
                    if not target:
                        fatal(cmake_path, target_2.line_no, "Undefined target (%s) in set_target_properties()",
                              target_2.name)
                    for prop in invoc.properties:
                        if prop.name.value == "OUTPUT_NAME":
                            target.output_name = prop.value.value
                            continue
                        fatal(cmake_path, prop.name.line_no, "Unsupported target property (%s) in "
                              "set_target_properties()", base.quote(prop.name.value))
                continue

            # FIXME: Because the evaluation of the condition of a control flow command can
            # clobber variables in the local scope, an unparsable control flow command with
            # a condition needs to "throw a taint bomb". Alternatively, it must generate a
            # parse error.                   
            if isinstance(invoc, _cip.IfInvoc):
                # FIXME: Consider allowing for "conditionally certain variable overrides"
                # tied to current conditional level          
                conditional_stack.append(copy.copy(conditional_state))
                conditional_state.line_no = invoc.line_no
                conditional_state.accumulated = base.Tristate.FALSE
                conditional_state.else_seen = False
                state, variable_uncertainty_reason = evaluate_condition(invoc.condition, directory, cmake_path)
                update_conditional_state(state, variable_uncertainty_reason, cmake_path, invoc.line_no)
                continue

            if isinstance(invoc, _cip.ElseifInvoc):
                assert len(conditional_stack) >= base_conditional_level
                if len(conditional_stack) == base_conditional_level:
                    fatal(cmake_path, invoc.line_no, "Unmatched elseif()")
                if conditional_state.else_seen:
                    fatal(cmake_path, invoc.line_no, "elseif() after else()")
                state, variable_uncertainty_reason = evaluate_condition(invoc.condition, directory, cmake_path)
                update_conditional_state(state, variable_uncertainty_reason, cmake_path, invoc.line_no)
                continue

            if isinstance(invoc, _cip.ElseInvoc):
                assert len(conditional_stack) >= base_conditional_level
                if len(conditional_stack) == base_conditional_level:
                    fatal(cmake_path, invoc.line_no, "Unmatched else()")
                if conditional_state.else_seen:
                    fatal(cmake_path, invoc.line_no, "else() after else()")
                state, variable_uncertainty_reason = base.Tristate.TRUE, None
                update_conditional_state(state, variable_uncertainty_reason, cmake_path, invoc.line_no)
                conditional_state.else_seen = True
                continue

            if isinstance(invoc, _cip.EndifInvoc):
                assert len(conditional_stack) >= base_conditional_level
                if len(conditional_stack) == base_conditional_level:
                    fatal(cmake_path, invoc.line_no, "Unmatched endif()")
                conditional_state = conditional_stack.pop()
                continue

            if isinstance(invoc, _cip.MacroInvoc):
                assert False                                   

            if isinstance(invoc, _cip.IncludeInvoc):
                if conditional_state.effective == base.Tristate.FALSE:
                    continue
                cmake_path_2 = resolve(invoc.path)
                subgroup = Subgroup(Subgroup.Type.INCLUSION, invoc.line_no, cmake_path_2)
                index = len(group.subgroups)
                process_group(cmake_path_2, directory, subgroup, subgroup_path + [ index ])
                if subgroup.targets or subgroup.subgroups:
                    group.subgroups.append(subgroup)
                continue

            if isinstance(invoc, _cip.AddSubdirectoryInvoc):
                if conditional_state.effective == base.Tristate.FALSE:
                    continue
                cmake_path_2 = resolve(invoc.path) / "CMakeLists.txt"
                subdirectory =_Directory(cmake_path_2, directory)
                subgroup = Subgroup(Subgroup.Type.SUBDIR, invoc.line_no, cmake_path_2)
                index = len(group.subgroups)
                process_group(cmake_path_2, subdirectory, subgroup, subgroup_path + [ index ])
                group.subgroups.append(subgroup)
                continue

            assert_never(invoc)

        assert len(conditional_stack) >= base_conditional_level
        if len(conditional_stack) > base_conditional_level:
            assert conditional_state.line_no > 0
            fatal(cmake_path, conditional_state.line_no, "Unclosed if()")

    def set_variable(variable_name: str, values: list[_cip.String], parent_scope: bool, directory: _Directory,
                     cmake_path: pathlib.Path, invoc_line_no: int) -> None:
        if conditional_state.effective == base.Tristate.FALSE:
            return
        target = directory
        if parent_scope:
            if not directory.parent:
                context = log.FileContext(cmake_path, invoc_line_no)
                log.FileContextLogger(logger, context).warn("set() invocation skipped: No parent scope exists")
                return
            target = directory.parent
        uncertain = conditional_state.effective == base.Tristate.UNCERTAIN
        if not uncertain:
            value = _cu.list_join([v.value for v in values])
            target.set_variable(variable_name, value)
            return
        assignment_context = log.FileContext(cmake_path, invoc_line_no)
        conditional_uncertainty_reason = conditional_state.conditional_uncertainty_reason
        assert conditional_uncertainty_reason
        assignment_uncertainty_reason = _cip.UncertainAssignmentOccurrenceReason(
            assignment_context,
            conditional_uncertainty_reason,
        )
        variable_uncertainty_reason = _cip.UncertainVariableAssignmentReason(
            variable_name,
            assignment_uncertainty_reason,
        )
        # FIXME: Consider adding new value as alternative specific value        
        target.taint_variable(variable_uncertainty_reason)

    def evaluate_condition(cond: _cip.Condition, directory: _Directory,
                           cmake_path: pathlib.Path) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
        def eval_as_bool(cond: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            context = log.FileContext(cmake_path, cond.line_no)
            if isinstance(cond, _cip.FalseCondition):
                return base.Tristate.FALSE, None
            if isinstance(cond, _cip.UncertainCondition):
                return base.Tristate.UNCERTAIN, cond.reason
            if isinstance(cond, _cip.ArgumentCondition):
                if cond.was_quoted_or_bracketed:
                    if is_true_constant(cond.string, context):
                        return base.Tristate.TRUE, None
                    return base.Tristate.FALSE, None
                if is_true_constant(cond.string, context):
                    return base.Tristate.TRUE, None
                if is_false_constant(cond.string, context):
                    return base.Tristate.FALSE, None
                value = directory.resolve_variable(cond.string)
                if isinstance(value, _cip.CertainValue):
                    if value.string is None:
                        return base.Tristate.FALSE, None
                    if is_false_constant(value.string, context):
                        return base.Tristate.FALSE, None
                    return base.Tristate.TRUE, None
                if isinstance(value, _cip.UncertainValue):
                    return base.Tristate.UNCERTAIN, value.reason
                assert_never(value)
            if isinstance(cond, _cip.UnopCondition):
                match cond.operator:
                    case _cip.UnaryConditionOperator.COMMAND:
                        raise _cip.FatalParseError(context, "Unsupported condition operator COMMAND")
                    case _cip.UnaryConditionOperator.POLICY:
                        raise _cip.FatalParseError(context, "Unsupported condition operator POLICY")
                    case _cip.UnaryConditionOperator.TARGET:
                        raise _cip.FatalParseError(context, "Unsupported condition operator TARGET")
                    case _cip.UnaryConditionOperator.TEST:
                        raise _cip.FatalParseError(context, "Unsupported condition operator TEST")
                    case _cip.UnaryConditionOperator.DEFINED:
                        return eval_defined(cond.operand)
                    case _cip.UnaryConditionOperator.EXISTS:
                        raise _cip.FatalParseError(context, "Unsupported condition operator EXISTS")
                    case _cip.UnaryConditionOperator.IS_READABLE:
                        raise _cip.FatalParseError(context, "Unsupported condition operator IS_READABLE")
                    case _cip.UnaryConditionOperator.IS_WRITABLE:
                        raise _cip.FatalParseError(context, "Unsupported condition operator IS_WRITABLE")
                    case _cip.UnaryConditionOperator.IS_DIRECTORY:
                        raise _cip.FatalParseError(context, "Unsupported condition operator IS_DIRECTORY")
                    case _cip.UnaryConditionOperator.IS_ABSOLUTE:
                        raise _cip.FatalParseError(context, "Unsupported condition operator IS_ABSOLUTE")
                    case _cip.UnaryConditionOperator.NOT:
                        return eval_not(cond.operand)
                assert_never(cond.operator)
            if isinstance(cond, _cip.BinopCondition):
                match cond.operator:
                    case _cip.BinaryConditionOperator.STREQUAL:
                        return eval_strequal(cond.left, cond.right)
                    case _cip.BinaryConditionOperator.STRLESS:
                        raise _cip.FatalParseError(context, "Unsupported condition operator STRLESS")
                    case _cip.BinaryConditionOperator.STRGREATER:
                        raise _cip.FatalParseError(context, "Unsupported condition operator STRGREATER")
                    case _cip.BinaryConditionOperator.STRLESS_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator STRLESS_EQUAL")
                    case _cip.BinaryConditionOperator.STRGREATER_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator STRGREATER_EQUAL")
                    case _cip.BinaryConditionOperator.EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator EQUAL")
                    case _cip.BinaryConditionOperator.LESS:
                        raise _cip.FatalParseError(context, "Unsupported condition operator LESS")
                    case _cip.BinaryConditionOperator.GREATER:
                        raise _cip.FatalParseError(context, "Unsupported condition operator GREATER")
                    case _cip.BinaryConditionOperator.LESS_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator LESS_EQUAL")
                    case _cip.BinaryConditionOperator.GREATER_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator GREATER_EQUAL")
                    case _cip.BinaryConditionOperator.VERSION_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator VERSION_EQUAL")
                    case _cip.BinaryConditionOperator.VERSION_LESS:
                        raise _cip.FatalParseError(context, "Unsupported condition operator VERSION_LESS")
                    case _cip.BinaryConditionOperator.VERSION_GREATER:
                        raise _cip.FatalParseError(context, "Unsupported condition operator VERSION_GREATER")
                    case _cip.BinaryConditionOperator.VERSION_LESS_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator VERSION_LESS_EQUAL")
                    case _cip.BinaryConditionOperator.VERSION_GREATER_EQUAL:
                        raise _cip.FatalParseError(context, "Unsupported condition operator VERSION_GREATER_EQUAL")
                    case _cip.BinaryConditionOperator.MATCHES:
                        return eval_matches(cond.left, cond.right)
                    case _cip.BinaryConditionOperator.IN_LIST:
                        return eval_in_list(cond.left, cond.right)
                    case _cip.BinaryConditionOperator.IS_NEWER_THAN:
                        raise _cip.FatalParseError(context, "Unsupported condition operator IS_NEWER_THAN")
                    case _cip.BinaryConditionOperator.AND:
                        return eval_and(cond.left, cond.right)
                    case _cip.BinaryConditionOperator.OR:
                        return eval_or(cond.left, cond.right)
                assert_never(cond.operator)
            assert_never(cond)

        def eval_defined(operand: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            string, uncertainty_reason = eval_as_str(operand)
            assert (string is None) != (uncertainty_reason is None)
            if string is None:
                return base.Tristate.UNCERTAIN, uncertainty_reason
            var_name = _cu.parse_var_name(string)
            match var_name:
                case _cu.GeneralVarName(name):
                    value = directory.resolve_variable(name)
                    if isinstance(value, _cip.CertainValue):
                        if value.string is None:
                            return base.Tristate.FALSE, None
                        return base.Tristate.TRUE, None
                    if isinstance(value, _cip.UncertainValue):
                        return base.Tristate.UNCERTAIN, value.reason
                    assert_never(value)
                case _cu.CacheVarName() | _cu.EnvVarName():
                    # FIXME: Support `CACHE{<variable>}` and `ENV{<variable>}` syntaxes
                    fatal(cmake_path, operand.line_no, "Unsupported variable type for DEFINED operator")
            assert_never(var_name)

        def eval_strequal(left: _cip.Condition,
                         right: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            string_1, uncertainty_reason = eval_as_str_from_var_or_str(left)
            assert (string_1 is None) != (uncertainty_reason is None)
            if string_1 is None:
                return base.Tristate.UNCERTAIN, uncertainty_reason
            string_2, uncertainty_reason = eval_as_str_from_var_or_str(right)
            assert (string_2 is None) != (uncertainty_reason is None)
            if string_2 is None:
                return base.Tristate.UNCERTAIN, uncertainty_reason
            if string_1 == string_2:
                return base.Tristate.TRUE, None
            return base.Tristate.FALSE, None

        def eval_matches(left: _cip.Condition,
                         right: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            string_1, uncertainty_reason_1 = eval_as_str_from_var_or_str(left)
            assert (string_1 is None) != (uncertainty_reason_1 is None)
            string_2, uncertainty_reason_2 = eval_as_str(right)
            assert (string_2 is None) != (uncertainty_reason_2 is None)
            # FIXME: Must also taint capture variables if the match is certain but the
            # matches operation is part of a block of commands whose execution is predicated
            # on an uncertain condition.                           
            if string_1 is None or string_2 is None:
                # FIXME: Taint `CMAKE_MATCH_COUNT`                 
                #
                # FIXME: If `string_2` is unknown, taint all capture variables, otherwise
                # taint only the cature variables that correspond to capure groups in the
                # regular expression.                              
                #
                return base.Tristate.UNCERTAIN, (uncertainty_reason_1 or uncertainty_reason_2)
            try:
                regex = _cr.compile_(string_2)
                m = regex.matches(string_1)
                if m:
                    # CMake exposes up to 10 capture groups including the full match
                    max_groups = 10
                    groups = [m.group(0)] + list(m.groups(""))[:max_groups-1]
                    if groups[0]:
                        # CMAKE_MATCH_COUNT is the highest N with a nonempty capture
                        n = max(i for i in range(len(groups)) if groups[i])
                        directory.set_variable("CMAKE_MATCH_COUNT", str(n))
                        # Replicating CMake quirk / bug by only setting the capture variable
                        # if the captured string is nonempty.
                        for i, group in enumerate(groups):
                            if group:
                                directory.set_variable("CMAKE_MATCH_%s" % i, group)
                    else:
                        directory.set_variable("CMAKE_MATCH_COUNT", "")
                    return base.Tristate.TRUE, None
                return base.Tristate.FALSE, None
            except _cr.SyntaxError as e:
                # FIXME: Find a way to expose the position of the problem as specified by
                # e.pos
                fatal(cmake_path, right.line_no, "Regular expression syntax error: %s", e)

        def eval_in_list(left: _cip.Condition,
                         right: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            string_1, uncertainty_reason = eval_as_str_from_var_or_str(left)
            assert (string_1 is None) != (uncertainty_reason is None)
            if string_1 is None:
                return base.Tristate.UNCERTAIN, uncertainty_reason
            string_2, uncertainty_reason = eval_as_str_from_var(right)
            assert (string_2 is None) != (uncertainty_reason is None)
            if string_2 is None:
                return base.Tristate.UNCERTAIN, uncertainty_reason
            if string_1 in _cu.list_split(string_2):
                return base.Tristate.TRUE, None
            return base.Tristate.FALSE, None

        def eval_not(operand: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            state, uncertainty_reason = eval_as_bool(operand)
            assert (state == base.Tristate.UNCERTAIN) != (uncertainty_reason is None)
            return ~state, uncertainty_reason

        def eval_and(left: _cip.Condition,
                     right: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            # In CMake, AND is not short-circuiting
            state_1, uncertainty_reason_1 = eval_as_bool(left)
            assert (state_1 == base.Tristate.UNCERTAIN) != (uncertainty_reason_1 is None)
            state_2, uncertainty_reason_2 = eval_as_bool(right)
            assert (state_2 == base.Tristate.UNCERTAIN) != (uncertainty_reason_2 is None)
            state = state_1 & state_2
            if state is not base.Tristate.UNCERTAIN:
                return state, None
            return state, (uncertainty_reason_1 or uncertainty_reason_2)

        def eval_or(left: _cip.Condition,
                    right: _cip.Condition) -> tuple[base.Tristate, _cip.VariableUncertaintyReason | None]:
            # In CMake, OR is not short-circuiting
            state_1, uncertainty_reason_1 = eval_as_bool(left)
            assert (state_1 == base.Tristate.UNCERTAIN) != (uncertainty_reason_1 is None)
            state_2, uncertainty_reason_2 = eval_as_bool(right)
            assert (state_2 == base.Tristate.UNCERTAIN) != (uncertainty_reason_2 is None)
            state = state_1 | state_2
            if state is not base.Tristate.UNCERTAIN:
                return state, None
            return state, (uncertainty_reason_1 or uncertainty_reason_2)

        def eval_as_str_from_var(cond: _cip.Condition) -> tuple[str | None, _cip.VariableUncertaintyReason | None]:
            var_name, uncertainty_reason = eval_as_str(cond)
            assert (var_name is None) != (uncertainty_reason is None)
            if var_name is None:
                return None, uncertainty_reason
            value = directory.resolve_variable(var_name)
            if isinstance(value, _cip.CertainValue):
                return (value.string or ""), None
            if isinstance(value, _cip.UncertainValue):
                return None, value.reason
            assert_never(value)

        def eval_as_str_from_var_or_str(cond: _cip.Condition) -> tuple[str | None,
                                                                       _cip.VariableUncertaintyReason | None]:
            if isinstance(cond, _cip.ArgumentCondition) and not cond.was_quoted_or_bracketed:
                value = directory.resolve_variable(cond.string)
                if isinstance(value, _cip.CertainValue):
                    if value.string is None:
                        return cond.string, None
                    return value.string, None
                if isinstance(value, _cip.UncertainValue):
                    return None, value.reason
                assert_never(value)
            return eval_as_str(cond)

        def eval_as_str(cond: _cip.Condition) -> tuple[str | None, _cip.VariableUncertaintyReason | None]:
            if isinstance(cond, _cip.ArgumentCondition):
                return cond.string, None
            state, uncertainty_reason = eval_as_bool(cond)
            assert (state == base.Tristate.UNCERTAIN) != (uncertainty_reason is None)
            match state:
                case base.Tristate.FALSE:
                    return "0", None
                case base.Tristate.UNCERTAIN:
                    return None, uncertainty_reason
                case base.Tristate.TRUE:
                    return "1", None
            assert_never(state)

        return eval_as_bool(cond)

    def is_false_constant(string: str, context: log.FileContext) -> bool:
        string_cf = string.casefold()
        if string_cf in {"off", "no", "false", "n", "ignore", "notfound", ""} or string_cf.endswith("-notfound"):
            return True
        value = base.Wrap(0)
        if as_number(string, value, context):
            return value == 0
        return False

    def is_true_constant(string: str, context: log.FileContext) -> bool:
        string_cf = string.casefold()
        if string_cf in {"on", "yes", "true", "y"}:
            return True
        value = base.Wrap(0)
        if as_number(string, value, context):
            return value != 0
        return False

    def as_number(string: str, value: base.Wrap[int], context: log.FileContext) -> bool:
        m = _FLOAT_REGEX.fullmatch(string)
        if not m:
            return False
        if _INT_REGEX.fullmatch(string):
            value.value = int(string)
            return True
        fatal(context.path, context.line_no, "Unsupported floating-point syntax (%s)", base.quote(string))

    def fatal(cmake_path: pathlib.Path, line_no: int, message: str, *args: Any) -> Never:
        context = log.FileContext(cmake_path, line_no)
        log.FileContextLogger(logger, context).fatal(message, *args)
        raise _ParseError from None

    def update_conditional_state(state: base.Tristate,
                                 variable_uncertainty_reason: _cip.VariableUncertaintyReason | None,
                                 cmake_path: pathlib.Path, invoc_line_no: int) -> None:
        assert conditional_stack
        parent_is_certain = conditional_state.effective == base.Tristate.TRUE
        effective = ~conditional_state.accumulated & state
        conditional_state.accumulated |= state
        conditional_state.effective = conditional_stack[-1].effective & effective
        if parent_is_certain and state == base.Tristate.UNCERTAIN:
            assert variable_uncertainty_reason
            conditional_state.conditional_uncertainty_reason = _cip.ConditionalUncertaintyReason(
                log.FileContext(cmake_path, invoc_line_no),
                variable_uncertainty_reason,
            )

    try:
        root_path = root_dir_path / "CMakeLists.txt"
        directory = _Directory(root_path)
        root_group = Group(root_path)
        subgroup_path: list[int] = []
        process_group(root_path, directory, root_group, subgroup_path)
        if not errors_seen:
            return root_group
        return None
    except _cip.FatalParseError as e:
        log.FileContextLogger(logger, e.context).fatal(e.message, *e.args)
        return None
    except _ParseError:
        return None


class _ParseError(Exception):
    pass


# FIXME: Consier allowing for taint patterns where only part of a variable name is known  
class _Directory:
    def __init__(self, path: pathlib.Path, parent: _Directory | None = None) -> None:
        self._path   = path
        self._parent = parent
        self._variables:         dict[str, _cip.CertainValue]              = {}
        self._tainted_variables: dict[str, _cip.VariableUncertaintyReason] = {}
        self._link_item_sets:    list[LinkItemSet]                         = []

    @property
    def path(self) -> pathlib.Path:
        return self._path

    @property
    def parent(self) -> _Directory | None:
        return self._parent

    def resolve_variable(self, name: str) -> _cip.Value:
        reason = self._tainted_variables.get(name)
        if reason:
            return _cip.UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            return value
        if self._parent:
            return self._parent.resolve_variable(name)
        reason = _cip.UnspecifiedVariableValueReason(name)
        return _cip.UncertainValue(reason)

    def set_variable(self, name: str, value: str | None) -> None:
        self._variables[name] = _cip.CertainValue(value)
        self._tainted_variables.pop(name, None)

    def taint_variable(self, reason: _cip.VariableUncertaintyReason) -> None:
        name = reason.variable_name
        self._variables.pop(name, None)
        self._tainted_variables[name] = reason

    def get_link_item_sets(self) -> list[LinkItemSet]:
        return self._link_item_sets + (self._parent.get_link_item_sets() if self._parent else [])

    def add_link_item_set(self, item_set: LinkItemSet) -> None:
        self._link_item_sets.append(item_set)


class _ConditionalState:
    def __init__(self) -> None:
        self.line_no      = 0
        self.effective    = base.Tristate.TRUE  # Conjunction along current nesting path
        self.accumulated  = base.Tristate.FALSE # Disjunction of preceding alternative branches
        self.else_seen    = False
        self.conditional_uncertainty_reason: _cip.ConditionalUncertaintyReason | None = None


# Attuned to C function `strtod()`
_FLOAT_REGEX = re.compile(r"""
    \s*   # Allow leading whitespace
    [+-]? # Optional sign
    (?:
        # 1. Hexadecimal Floats
        0x
        (?:
            [0-9a-f]+\.?[0-9a-f]* |
            \.[0-9a-f]+
        )
        (?:p[+-]?[0-9]+)?

        |

        # 2. Decimal Floats
        (?:
            [0-9]+\.?[0-9]* |
            \.[0-9]+
        )
        (?:e[+-]?[0-9]+)?

        |

        # 3. Infinity and NaN
        INF(?:INITY)?
        |
        NAN(?:\([a-z0-9_]*\))?
    )
""", re.VERBOSE | re.IGNORECASE)


_INT_REGEX = re.compile(r"\s*[+-]?[0-9]+")


def _dump_structure(root_group: Group, logger: log.Logger, log_level: log.LogLevel) -> None:
    def dump_root_group() -> None:
        dump(0, "Root(%s):", root_group.cmake_path)
        dump_targets(root_group, [], 1)
        dump_subgroups(root_group, [], 1)
    def dump_subgroup(subgroup: Subgroup, subgroup_path: list[int], level: int) -> None:
        if subgroup.type_ is Subgroup.Type.INCLUSION:
            label = "Inclusion"
        elif subgroup.type_ is Subgroup.Type.SUBDIR:
            label = "Subdirectory"
        else:
            assert_never(subgroup.type_)
        dump(level, "%s(%s [line %s]):", label, subgroup.cmake_path, subgroup.line_no)
        dump_targets(subgroup, subgroup_path, level + 1)
        dump_subgroups(subgroup, subgroup_path, level + 1)
    def dump_targets(group: Group, subgroup_path: list[int], level: int) -> None:
        for target in group.targets:
            dump_target(target, subgroup_path, level)
    def dump_subgroups(group: Group, subgroup_path: list[int], level: int) -> None:
        for i, subgroup in enumerate(group.subgroups):
            dump_subgroup(subgroup, subgroup_path + [ i ], level)
    def dump_target(target: Target, subgroup_path: list[int], level: int) -> None:
        if isinstance(target, LibraryTarget):
            dump(level, "LibraryTarget(%s %s [line %s] [closing line %s]):", target.cmake_name,
                 target.library_type.name, target.line_no, target.closing_line_no)
            if target.library_type != LibraryType.INTERFACE:
                dump_source_files(target, level + 1)
            dump_output_name(target, level + 1)
            dump_source_file_sets(target, subgroup_path, level + 1)
            dump_header_file_sets(target, subgroup_path, level + 1)
            dump_link_item_sets(target, subgroup_path, level + 1)
            return
        if isinstance(target, ExecutableTarget):
            dump(level, "ExecutableTarget(%s [line %s] [closing line %s]):", target.cmake_name, target.line_no,
                 target.closing_line_no)
            dump_source_files(target, level + 1)
            dump_output_name(target, level + 1)
            dump_source_file_sets(target, subgroup_path, level + 1)
            dump_header_file_sets(target, subgroup_path, level + 1)
            dump_link_item_sets(target, subgroup_path, level + 1)
            return
        assert_never(target)
    def dump_source_files(target: Target, level: int) -> None:
        dump(level, "SourceFiles:")
        for path in target.source_files:
            dump(level + 1, "%s [line %s]", path.path.as_posix(), path.line_no)
    def dump_source_file_sets(target: Target, subgroup_path: list[int], level: int) -> None:
        for file_set in target.source_file_sets:
            location = format_location(file_set.subgroup_path, file_set.line_no, subgroup_path)
            dump(level, "SourceFileSet(%s [closing line %s]):", location, file_set.closing_line_no)
            for path in file_set.files:
                dump(level + 1, "%s %s [line %s]", path.scope.name, path.path.as_posix(), path.line_no)
    def dump_header_file_sets(target: Target, subgroup_path: list[int], level: int) -> None:
        for file_set in target.header_file_sets:
            location = format_location(file_set.subgroup_path, file_set.line_no, subgroup_path)
            dump(level, "HeaderFileSet(%s %s %s [closing line %s]):", file_set.scope.name, base.quote(file_set.name),
                 location, file_set.closing_line_no)
            for path in file_set.files:
                dump(level + 1, "%s [line %s]", path.path.as_posix(), path.line_no)
    def dump_link_item_sets(target: Target, subgroup_path: list[int], level: int) -> None:
        for link_item_set in target.link_item_sets:
            location = format_location(link_item_set.subgroup_path, link_item_set.line_no, subgroup_path)
            dump(level, "LinkItemSet(%s [closing line %s]):", location, link_item_set.closing_line_no)
            for item in link_item_set.items:
                dump(level + 1, "%s %s %s [line %s]", item.scope.name, item.config.name, base.quote(item.value),
                     item.line_no)
    def dump_output_name(target: Target, level: int) -> None:
        if target.output_name is None:
            return
        dump(level, "Output name: %s", base.quote(target.output_name))
    def format_location(subgroup_path: list[int], line_no: int, curr_subgroup_path: list[int]) -> str:
        if subgroup_path == curr_subgroup_path:
            return "[line %s]" % line_no
        entries = []
        group = root_group
        for index in subgroup_path:
            subgroup = group.subgroups[index]
            entries.append("%s:%s" % (group.cmake_path, subgroup.line_no))
            group = subgroup
        entries.append("%s:%s" % (group.cmake_path, line_no))
        return "[%s]" % ", ".join(entries)
    lines = []
    def dump(level: int, message: str, *args: Any) -> None:
        indent = level * "    "
        lines.append("%s%s" % (indent, message % args))
    dump_root_group()
    logger.log(log_level, "%s", "\n".join(lines))
