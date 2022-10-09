@hit@
position p;
@@

(
	get_random_int()@p
|
	prandom_u32()@p
|
	get_random_u32()@p
)

@easy depends on hit@
identifier randfunc =~ "get_random_int|prandom_u32|get_random_u32";
expression E;
type TYPE;
@@

(
-	((TYPE)randfunc() % (E))
+	prandom_u32_max(E)
|
-	((TYPE)randfunc() & ~(PAGE_MASK))
+	prandom_u32_max(PAGE_SIZE)
|
-	((TYPE)randfunc() & ~(E))
+	prandom_u32_max(E * XXX_CHECK_ME)
|
-	((TYPE)randfunc() & ((PAGE_SIZE) - 1))
+	prandom_u32_max(PAGE_SIZE)
|
-	((TYPE)randfunc() & ((E) - 1))
+	prandom_u32_max(E * XXX_CHECK_ME)
)

@multi_line depends on hit@
identifier randfunc =~ "get_random_int|prandom_u32|get_random_u32";
identifier RAND;
expression E, ANYTHING;
type TYPE;
int LITERAL;
@@

(
-	RAND = (TYPE)randfunc();
	... when != RAND
-	RAND %= (E);
+	RAND = prandom_u32_max(E);
|
-	RAND = (TYPE)randfunc();
	... when != RAND
-	RAND = (RAND % (E)) + LITERAL;
+	RAND = prandom_u32_max(E) + LITERAL;
|
-	RAND = (TYPE)randfunc();
	... when != RAND
-	(RAND % (E))
+	prandom_u32_max(E)
	... when != RAND
?	RAND = ANYTHING;
)

@literal_mask depends on hit@
int LITERAL;
identifier randfunc =~ "get_random_int|prandom_u32|get_random_u32";
type TYPE;
position p;
@@

	((TYPE)randfunc()@p & (LITERAL))

@script:python add_one@
literal << literal_mask.LITERAL;
RESULT;
@@

value = None
if literal.startswith('0x'):
        value = int(literal, 16)
elif literal[0] in '123456789':
        value = int(literal, 10)
if value is None:
        print("I don't know how to handle %s" % (literal))
        cocci.include_match(False)
elif value & (value + 1) != 0:
        print("Skipping 0x%x because it's not a power of two minus one" % (value))
        cocci.include_match(False)
elif literal.startswith('0x'):
        coccinelle.RESULT = cocci.make_expr("0x%x" % (value + 1))
else:
        coccinelle.RESULT = cocci.make_expr("%d" % (value + 1))

@plus_one@
int literal_mask.LITERAL;
position literal_mask.p;
expression add_one.RESULT;
identifier FUNC;
type TYPE;
@@

-	((TYPE)FUNC()@p & (LITERAL))
+	prandom_u32_max(RESULT)

@collapse_ret depends on hit@
type TYPE;
identifier VAR;
expression E;
@@

 {
-	TYPE VAR;
-	VAR = (E);
-	return VAR;
+	return E;
 }

@drop_var depends on hit@
type TYPE;
identifier VAR;
@@

 {
-	TYPE VAR;
	... when != VAR
 }
