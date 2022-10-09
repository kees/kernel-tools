# Coccinelle "How Do I ...?"

This is an ever-growing dump of tips, tricks, discoveries, and wisdom for
writing [Coccinelle](https://coccinelle.gitlabpages.inria.fr/website/)
rules for the Linux kernel. When I can't figure it out
myself, I will ask for help on the [Coccinelle mailing
list](https://coccinelle.gitlabpages.inria.fr/website/contact.html).
I have started to collect those answers and my own discoveries here,
since I frequently end up needing them again in the future.


## Run in parallel

The Linux kernel's `coccicheck` target already knows how to run Coccinelle
in parallel, so instead of trying to figure this out each time, we can
just ask nicely for the command line:

```
# The arguments are Linux Makefile-specific, so this attempts to extract
# the desired arguments from the execution error message.
ARGS=$(make coccicheck V=1 MODE=patch COCCI=/dev/null 2>&1 | \
        grep ^Running | \
        awk -F"/usr/bin/spatch " '{print $2}' | \
        sed     \
                -e 's/-D patch //' \
                -e 's/--dir \. //' \
                -e 's/--cocci-file \/dev\/null //' \
        )
```

See the
[cocci](https://github.com/kees/kernel-tools/blob/trunk/coccinelle/cocci)
wrapper for more details.

## Include headers in matches

To examine complex patterns in structure layouts, fully recursive
analysis of included headers may be needed. For the deep structure rule
[example](#limit-runtimes-with-complex-matches) below, the .cocci file
must include:

```
// Options: --recursive-includes
```

Using recursive includes can make the runtime much slower, though. See
[Limit runtimes with complex matches](#limit-runtimes-with-complex-matches)
and [Use regular expressions to quickly match
identifiers](#use-regular-expressions-to-quickly-match-identifiers)
for possible ways to improve runtimes.

For doing general matches across the entire code base where all headers
should be considered in addition to all .c files, use:

```
// Options: --all-headers
```

## Use regular expressions to quickly match identifiers

If a regex is used to match an identifier, like this:

```
@rule@
identifier func =~ "get_random_int|prandom_u32|get_random_u32";
@@

*	func(...)
```

it will greatly increase run-times. This can be fixed by adding a
literal match list first, and having the rule depend on it:

```
@match@
@@

(
	get_random_int
|
	prandom_u32
|
	get_random_u32
)
	()

@rule depends on match@
identifier func =~ "get_random_int|prandom_u32|get_random_u32";
@@

*	func(...)
```

## Limit runtimes with complex matches

Similar to the above tip, listing the most restrictive rule first can
help narrow the run-time. This can be done without knowing much about
more complex later matches, too. For example, metavariables can be
referenced by later metavariable definitions, without having them
exist in the rule body itself:

```
@compflex@
identifier OUTER, COMPFLEX;
struct OUTER *PTR;
expression SRC, SIZE;
@@

  memcpy(&PTR->COMPFLEX, SRC, SIZE)
```

Here, `OUTER` is not used by _this_ rules, but its relationship to
`PTR` can be used in later rules. We have created the constraint that
"`PTR` will match a pointer to any struct whose name will be matched
by `OUTER`", which can be used by the next rule:

```
@struct_match@
identifier INNER;
identifier compflex.OUTER, compflex.COMPFLEX;
@@
        struct OUTER {
                ...
                struct INNER COMPFLEX;
        };
```

In this case, Coccinelle will limit matching to only the `memcpy`
instances, and then further limit to the structs that match the described
relationship between `PTR` and `OUTER`. If this were done in the other
direction, all possible structs would be matched first, which is huge
compared to the number of `memcpy` matches.

Note that complex structure layout analysis will likely require that the
.cocci file [Include headers in matches](#include-headers-in-matches)
recursively.

In a pinch, the list of files can be manually collected and fed to the
"cocci" wrapper directly instead of doing a full recursive search:

```
git grep 'something_to_match' | cut -d: -f1 | sort -u | xargs cocci RULES.cocci
```

## Rewrite a matched element

If you need to rewrite a matched string (or do any kind of variable
processing), you can use Python to do the work by matching what you want,
converting it, and then using the conversion in a second match:

```
@proto@
identifier FUNC;
type TYPE;
identifier NAME;
position p;
@@

 FUNC@p(...,
        TYPE NAME
 ) {
        ...
   }

@script:python get_member@
type_name << proto.TYPE;
MEMBER;
@@

// Extract everything following the first "_" from the "TYPE" match.
// e.g. "blah_cow_foo *" becomes "cow_foo"
coccinelle.MEMBER = cocci.make_ident(type_name.split("_", 1)[1].split(' ',1)[0])

@convert@
identifier proto.FUNC;
type proto.TYPE;
identifier proto.NAME;
position proto.p;
identifier get_member.MEMBER;
@@

 FUNC@p(...,
-       TYPE NAME
+       union nfsd4_op_u *u
 ) {
+       TYPE NAME = &u->MEMBER;
        ...
   }
```

In a `script:python`, the `coccinelle` object is used for output, and the
`cocci` object is used for access to the Coccinelle helper functions.

To have a rule not match, use `cocci.include_match(False)`. To construct
metavariables for use in further rules, use the `cocci` object's `make_ident`, `make_expr`,
`make type`, `make_stmt`, etc helpers. For additional details, see
[Coccinelle's Python API documentation](https://github.com/coccinelle/coccinelle/blob/master/python/python_documentation.md).

## Limit matches when variable contents aren't used again

To change or remove a variable's contents safely, we must make sure its
contents aren't goint be used again after the rule. For example:

```
foo = 5;
something(foo);
```

is only safe to be rewritten to:

```
something(5);
```

... if there are either no more uses of `foo`, or the next use is an assignment. For example,

This would be a safe result (`foo` is assigned a new value):
```
something(5);
foo = next_value;
```

But this would not be (`foo` is used, but we've removed its earlier assignment):

```
something(5);
val += foo;
```

This condition can be matched by checking for "no more use of `foo`
until an optionally present assignment":

```
@remove@
identifier VAR, ANYTHING;
expression E;
@@

-	VAR = E;
	... when != VAR
	something(
-		VAR
+		E
	);
	... when != VAR
?	VAR = ANYTHING;
```
