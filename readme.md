# Grammar Validator
A simple BNF grammar validator that checks for common grammar issues.

## Features
- Check for productiveness (all non-terminals can derive terminals)
- Check for useless terminals and non-terminals
- Check for reachability (all symbols reachable from start symbol)

## BNF Grammar Format
The validator accepts a BNF-like syntax (similar to but not identical to Yacc):

**Rules:**
- Non-terminals are written as-is (e.g., `S`, `expr`)
- Terminals must be declared with `%token` before use
- Productions use `->` to separate left and right sides
- Alternative productions are separated by `|`
- Productions are separated by double newlines
- No newline required at end of file
- The `%start` token specifies the start symbol. If not specified, the first production's left-hand side is used as the start symbol.

**Example:**
```
S -> a | b

%token NUM
%token STRING
a -> NUM | STRING

%token CHAR
b -> CHAR
```

## Building
```bash
make
```

## Usage
```bash
./bnf_validator grammar_file
```

**Example output:**
```
✓ Grammar is productive
✓ No useless symbols
✓ All symbols reachable from start symbol
Grammar is valid!
```

## Limitations

- No support for empty productions (ε)
- No support for quoted terminal strings
