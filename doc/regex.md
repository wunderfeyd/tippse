Tippse regular expressions
==========================

## Escaping

Regular expressions command literals have to be different to usual literals. To allow both in the same text, the escape character `\` switches to the other form (literal or command). For example `\|` results in a `|` in the output instead of the alternative command `|`. To enter an arbitrary character/codepoint it's possible to use `\xXX`, `\uXXXX` and `\UXXXXXXXX` where X is the hexadecimal representation of the codepoint.

## Alternatives

Alternatives are used in the form `abc|def` and matches `abc` or `def`.

## Grouping

Literals and/or other groups can be grouped together either to repeat with quantization modifiers or as backreferences in replacements. `(abc)+`

## Sets

If one or more literals are allowed at the same position it possible to use a literal set in form of `[abc]`. It is possible to add ranges like `[A-Z]`, to invert the final set with `[^A-Z]` and/or use a character class `[^\w_]`. Please note that the `-` (minus sign) has to be escaped and is not allowed as stand alone at the end of the set unlike other implementations.

## Character classes

Predefined sets are:

1. `\d` digits (all languages)
2. `\D` everything else than digits
3. `\w` letters (all languages)
4. `\W` everything else than letters
5. `\s` whitespace (all languages)
6. `\S` everything else than whitespace
7. `.` any character (including newline)

## Quantization

After a group or literal the number of repetitions are added if more or less than a single match is needed. Possible commands:

1. `*` matches from 0 to infinite times
2. `+` matches from 1 to infinite times
3. `?` matches from 0 to 1 times
4. `{n}` matches exactly n times
5. `{n,}` matches at least n times
6. `{,m}` matches from 0 to m times
7. `{n,m}` matches from n to m times

Example: `(abc){3}` would match `abcabcabc` and `(abc|def){2}` would match `abcabc`, `abcdef`, `defabc` or `defdef`.

## Matching type

After using a quantization modifier software search for the longest match longest match (greedy). To modify this behaviour use one of the following options:

* Lazy matching

  When using `?` the shortest match that fulfills the criterias is used.

* Possessive matching

  When using `+` the longest match of the group/literal is used. Unlike the greedy match type which also checks the rest of the expression.

## Anchors

With `^` the search start with the next line start or file start. Whereas `$` checks for a line end or file end.

## Backreferences

Backreferences are allowed in replacements and during matching outside the referenced group. Use \1 to \9 for the group number. `(\S)123(\S)` with replacement `\1\2` on `abc123def` results in `abcdef`.

## Notes

When using the "ignore case" option the literal sets are expanded with their uppercase/lowercase variants. If the transformation results in more than one codepoint/character everything is added. For example the german `ß` is expanded to `ss`.
