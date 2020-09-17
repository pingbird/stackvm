## HBF

HBF, short for high-level brainfuck, is an IR that employs an elegant set of optimization techniques.

#### Range

These represent well, a range between two offsets. Ranges can be unbounded in which case the upper is INT32_MAX and/or
the lower is INT32_MIN.

```
-1 <= x < 3 // A range from -1 (inclusive) to 3 (exclusive).
x < 5       // A range for any offset less than 5.
4 <= x      // A range for any offset greater or equal to 4.
x = 9       // A range for a single value.
all         // A range that includes everything.
none        // A range that includes nothing.
```

#### Mask

These represent an arbitrary set of non-overlapping ranges, forming a mask that can you can efficiently test offsets
against. Note that this is not implemented as an actual bitmask but as a sorted list of offsets that you can do binary
search on.

```
x < 5, x = 9 // A mask that includes everything below 5 and 9.
4 <= x > 3   // A mask for a single range.
```

#### Seek

These represent any pure seeks, including balanced and unbalanced loops:

```
<           // A simple seek with an offset of -1.
>[<<<]>     // A more complex seek with an unbalanced loop.
[>>[>>>]<]> // An even more complex seek with nested unbalanced loops.
```

Note that pure balanced loops cannot halt and invoke UB as a result (brainfuck UB, fun!).

#### Def instruction

Local address definitions are defined through the `I_DEF` instruction, these allow you to hoist any invariant
addresses.

```
{x}     // Declares x as ptr.
{>x}    // Declares x as ptr + 1.
{x[>]y} // Declares x as ptr and y as the next zero cell.
```

Defs can also be nested, expressing a complex set of related addresses.

```
{>>>[<<]>[>]x}{>>>[<<]>[<]y}
// Is equivalent to:
{>>>[<<]>{[>]x}[<]y}
```