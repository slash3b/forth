# Forth Words to Implement

## Stack Manipulation (most critical)
- `DUP` - duplicate top (`a -- a a`)
- `DROP` - discard top (`a --`)
- `SWAP` - swap top two (`a b -- b a`)
- `OVER` - copy second to top (`a b -- a b a`)
- `ROT` - rotate top three (`a b c -- b c a`)

## Comparison (return -1 for true, 0 for false)
- `=` - equal
- `<` `>` - less than, greater than
- `0=` - is zero?

## I/O
- `.` (dot) - pop and print top of stack
- `EMIT` - print character (ASCII value)
- `CR` - newline
- `.S` - print stack (non-destructive, great for debugging)

## Logical
- `AND` `OR` `XOR` `NOT`

## Control Flow
- `IF...ELSE...THEN`
- `DO...LOOP` (or `BEGIN...UNTIL`)

## Defining Words
- `:` and `;` - define new words
- `VARIABLE` / `CONSTANT`

## Other Useful
- `NEGATE` - negate number
- `ABS` - absolute value
- `MIN` `MAX`
- `2DUP` `2DROP` `2SWAP` - operate on pairs

## Priority
Start with: `DUP`, `DROP`, `SWAP`, `.` (print), and `.S` - they make the interpreter usable and debuggable.
