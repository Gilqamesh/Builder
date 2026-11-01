# Source include lexer

## Goal

Extract paths from quoted and angle-bracket source include directives so dependency scanners can discover textual includes. This is a focused lexical scanner; resolving paths, interpreting module identities, and enforcing dependency eligibility belong to the source-dependency layer.

See [lexer.h](lexer.h) for the public interface.
