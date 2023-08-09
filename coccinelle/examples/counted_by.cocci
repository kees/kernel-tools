// Options: --all-includes

// When finding the element count assignment, it must be reordered
// to before any accesses of the PTR->ARRAY itself, otherwise runtime
// checking will trigger (i.e. the index of ARRAY will be checked against
// COUNTER before COUNTER has been assigned the correct COUNT value).
@allocated@
identifier STRUCT, ARRAY, COUNTER, CALC;
expression COUNT, REPLACED;
struct STRUCT *PTR;
identifier ALLOC =~ "[kv][cvzm]alloc";
@@

(
	CALC = struct_size(PTR, ARRAY, COUNT);
	... when != CALC = REPLACED
 	PTR = ALLOC(..., CALC, ...);
|
 	PTR = ALLOC(..., struct_size(PTR, ARRAY, COUNT), ...);
)
(
?	if (!PTR) { ... }
 	... when != PTR->ARRAY
	PTR->COUNTER = COUNT;
|
	if (!PTR) { ... }
+	PTR->COUNTER = COUNT;
	...
-	PTR->COUNTER = COUNT;
|
	if (unlikely(!PTR)) { ... }
+	PTR->COUNTER = COUNT;
	...
-	PTR->COUNTER = COUNT;
|
?	if (!PTR) { ... }
+	PTR->COUNTER = COUNT;
	...
-	PTR->COUNTER = COUNT;
)

// Since trailing attributes are ignored when doing a match, we must
// explicitly exclude existing __counted_by annotations and only add
// it if it wasn't already found.
@annotate@
type COUNTER_TYPE, ARRAY_TYPE;
identifier allocated.STRUCT;
identifier allocated.ARRAY;
identifier allocated.COUNTER;
attribute name __counted_by;
@@

 struct STRUCT {
 	...
 	COUNTER_TYPE COUNTER;
 	...
(
	ARRAY_TYPE ARRAY[] __counted_by(COUNTER);
|
	ARRAY_TYPE ARRAY[]
+	__counted_by(COUNTER)
	;
)
	...
 };
