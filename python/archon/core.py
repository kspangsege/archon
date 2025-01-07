def quote(string):
    string_2 = ""
    for ch in string:
        string_2 += _quote_repl(ch)
    return "\"%s\"" % string_2


def clamped_quote(string, max_size):
    ellipsis = "..."
    min_max_size = 2 + len(ellipsis)
    if max_size < min_max_size:
        max_size = min_max_size
    limit_2 = max_size - 2
    limit_1 = limit_2 - len(ellipsis)
    string_2 = ""
    breach_pos = None
    for ch in string:
        repl = _quote_repl(ch)
        if breach_pos is None:
            if len(repl) <= limit_1 - len(string_2):
                string_2 += repl
                continue
            breach_pos = len(string_2)
        if len(repl) <= limit_2 - len(string_2):
            string_2 += repl
            continue
        return "\"%s...\"" % string_2[:breach_pos]
    return "\"%s\"" % string_2


def _quote_repl(ch):
    repl = _simple_escapes.get(ch)
    if repl is not None:
        return repl
    if ch.isprintable():
        return ch
    val = ord(ch)
    if val <= 0xFFFF:
        return "\\u%04X" % val
    assert val <= 0xFFFFFFFF
    return "\\U%08X" % val


_simple_escapes = {
    "\a": "\\a",
    "\b": "\\b",
    "\t": "\\t",
    "\n": "\\n",
    "\v": "\\v",
    "\f": "\\f",
    "\r": "\\r",
    "\"": "\\\"",
    "\\": "\\\\",
}
