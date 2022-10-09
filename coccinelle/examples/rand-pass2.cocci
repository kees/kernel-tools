@hit@
position p;
@@

(
	get_random_int()@p
|
	prandom_u32()@p
|
	get_random_u32()@p
|
	prandom_bytes
)

@downsize depends on hit@
int LITERAL;
identifier randfunc =~ "get_random_int|prandom_u32|get_random_u32";
type TYPE;
position p;
@@

(
	((TYPE)randfunc()@p & LITERAL)
|
	((TYPE)randfunc()@p % LITERAL)
)

@script:python is_smaller@
literal << downsize.LITERAL;
SMALLER;
@@

value = None
if literal.startswith('0x'):
        value = int(literal, 16)
elif literal[0] in '123456789':
        value = int(literal, 10)
if value is None:
        print("I don't know how to handle %s" % (literal))
        cocci.include_match(False)
elif value <= 2**8:
        coccinelle.SMALLER = cocci.make_ident("get_random_u8")
elif value <= 2**16:
        coccinelle.SMALLER = cocci.make_ident("get_random_u16")
else: # value <= 2**32
        coccinelle.SMALLER = cocci.make_ident("get_random_u32")

@change_size@
int downsize.LITERAL;
position downsize.p;
identifier is_smaller.SMALLER;
identifier FUNC;
type TYPE;
@@

(
-	((TYPE)FUNC()@p & LITERAL)
+	(SMALLER() & LITERAL)
|
-	((TYPE)FUNC()@p % LITERAL)
+	(SMALLER() % LITERAL)
)

@int_to_u32 depends on hit@
@@

-	get_random_int
+	get_random_u32
	()

@bytes depends on hit@
@@

-	prandom_bytes
+	get_random_bytes
	(...)

@rand_u8 depends on hit@
typedef u8;
@@

-	(u8)prandom_u32
+	get_random_u8
	()

@rand_u16 depends on hit@
typedef u16;
@@

-	(u16)prandom_u32
+	get_random_u16
	()

@rand_u32 depends on hit@
@@

-	prandom_u32
+	get_random_u32
	()
