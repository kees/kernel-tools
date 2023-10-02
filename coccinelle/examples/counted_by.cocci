// Options: --all-includes

// When finding the element count assignment, it must be reordered
// to before any accesses of the PTR->ARRAY itself, otherwise runtime
// checking will trigger (i.e. the index of ARRAY will be checked against
// COUNTER before COUNTER has been assigned the correct COUNT value).
@allocated@
identifier STRUCT, ARRAY, COUNTER, CALC;
expression COUNT, REPLACED;
struct STRUCT *PTR;
identifier ALLOC;
type ELEMENT_TYPE;
@@

(
	CALC = struct_size(PTR, ARRAY, COUNT);
	... when != CALC = REPLACED
 	PTR = ALLOC(..., CALC, ...);
|
	CALC = \(sizeof(*PTR)\|sizeof(struct STRUCT)\) +
		(COUNT * \(sizeof(*PTR->ARRAY)\|sizeof(PTR->ARRAY[0])\|sizeof(ELEMENT_TYPE)\));
	... when != CALC = REPLACED
	PTR = ALLOC(..., CALC, ...);
|
	PTR = ALLOC(..., struct_size(PTR, ARRAY, COUNT), ...);
|
	PTR = ALLOC(..., \(sizeof(*PTR)\|sizeof(struct STRUCT)\) +
			 (COUNT * \(sizeof(*PTR->ARRAY)\|sizeof(PTR->ARRAY[0])\|sizeof(ELEMENT_TYPE)\)), ...);
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
type allocated.ELEMENT_TYPE;
identifier OTHER_ARRAY;		// Without struct_size(), ARRAY be be misidentified.
attribute name __counted_by;
@@

 struct STRUCT {
 	...
 	COUNTER_TYPE COUNTER;
 	...
(
	\(ARRAY_TYPE\|ELEMENT_TYPE\) \(ARRAY\|OTHER_ARRAY\)[] __counted_by(COUNTER);
|
	ELEMENT_TYPE { ... } \(ARRAY\|OTHER_ARRAY\)[] __counted_by(COUNTER);
|
	\(ARRAY_TYPE\|ELEMENT_TYPE\) \(ARRAY\|OTHER_ARRAY\)[]
+	__counted_by(COUNTER)
	;
|
	ELEMENT_TYPE { ... } \(ARRAY\|OTHER_ARRAY\)[]
+	__counted_by(COUNTER)
	;
)
	...
 };
