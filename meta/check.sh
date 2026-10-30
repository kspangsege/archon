cd "$(dirname "$0")" || exit 1

mypy *_demo.py test_*.py check_grammar.py generate_cmake_policy_definitions.py || exit 1

pylint archon/ *_demo.py test_*.py check_grammar.py generate_cmake_policy_definitions.py || exit 1
