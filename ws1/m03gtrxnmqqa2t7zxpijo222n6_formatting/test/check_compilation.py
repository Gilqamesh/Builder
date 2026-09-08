#!/usr/bin/env python3
"""Check rejection contracts and link formatter use across translation units."""
import os
from pathlib import Path
import subprocess
import tempfile

module = Path(__file__).resolve().parent.parent
compiler = os.environ.get("CXX", "g++")
flags = [compiler, "-std=c++26", "-freflection", "-I" + str(module.parent)]
header = "#include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>\n"
base = "m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t"
cases = {
    "private_member": ("class test_t { int hidden = 3; };", "requires public subobjects"),
    "private_base": ("struct base_t {}; class test_t : private base_t {};", "requires public subobjects"),
    "union": ("union test_t { int number; float fraction; };", "unions require a custom formatter"),
    "unsupported_member": ("struct unformatted_t {}; struct test_t { unformatted_t member; };", "requires a formatter for each member"),
    "unsupported_base": ("struct unformatted_t {}; struct test_t : unformatted_t {};", "requires a formatter for each base"),
}

with tempfile.TemporaryDirectory(prefix="builder-formatting-compilation-") as temporary:
    root = Path(temporary)
    for name, (declaration, diagnostic) in cases.items():
        source = root / (name + ".cpp")
        source.write_text(header + declaration + "\ntemplate <> struct std::formatter<test_t> : " + base
            + " {};\nint main() { (void)std::format(\"{}\", test_t{}); }\n")
        result = subprocess.run(flags + ["-fsyntax-only", str(source)], capture_output=True, text=True)
        if result.returncode == 0 or diagnostic not in result.stderr:
            raise SystemExit(name + " did not produce its expected rejection:\n" + result.stderr)
        print("PASS", name)

    literal = root / "literal.cpp"
    literal.write_text(header + "int main() { (void)std::format(\"{:x}\", " + base + "{}); }\n")
    result = subprocess.run(flags + ["-fsyntax-only", str(literal)], capture_output=True, text=True)
    if result.returncode == 0 or "a level from 0 to 3" not in result.stderr:
        raise SystemExit("invalid literal specification was not rejected:\n" + result.stderr)
    print("PASS invalid_literal")

    unselected = root / "unselected.cpp"
    unselected.write_text(header + "struct unselected_t {}; static_assert(!std::formattable<unselected_t, char>);\n")
    subprocess.run(flags + ["-fsyntax-only", str(unselected)], check=True)
    print("PASS explicit_selection")

    first = root / "first.cpp"
    second = root / "second.cpp"
    fixture = '#include "' + str(module / "test/fixtures.h") + '"\n'
    first.write_text(fixture + "std::string across_translation_units() { return std::format(\"{}\", m03gtrxnmqqa2t7zxpijo222n6_formatting::extent_t{1, 2}); }\n")
    second.write_text("#include <format>\n" + fixture
        + "std::string across_translation_units();\nint main() { return across_translation_units() != std::format(\"{}\", m03gtrxnmqqa2t7zxpijo222n6_formatting::extent_t{1, 2}); }\n")
    binary = root / "linked"
    subprocess.run(flags + [str(first), str(second), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("PASS translation_units_and_include_order")
