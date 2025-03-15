// Options: --no-includes --include-headers

@initialize:python@
@@
import sys

def alloc_array(name):
	prefix = "km"
	if name.startswith('kma'):
		prefix = "km"
	elif name.startswith('kvm'):
		prefix = "kvm"
	elif name.startswith('kva'):
		prefix = "kv"
	elif name.startswith('kca'):
		prefix = "kz"
	elif name.startswith('kvz'):
		prefix = "kvz"
	elif name.startswith('kvca'):
		prefix = "kvz"
	else:
		print(f"Unknown transform for {name}", file=sys.stderr)
	return f"{prefix}alloc_objs"

@noprof@
identifier ALLOC =~ "kv?[mz]alloc_noprof";
@@

	ALLOC(...)

@direct depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
identifier PTR;
expression DEREF;
expression GFP;
expression COUNT;
expression FLEX;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_OBJ = ALLOC ## "_obj";
fresh identifier ALLOC_FLEX = ALLOC ## "_flex";
identifier ALLOC_ARRAY =~ "kv?([mz]alloc_array|calloc)";
fresh identifier ALLOC_OBJS = script:python(ALLOC_ARRAY) { alloc_array(ALLOC_ARRAY) };
@@

(
-	TYPE *PTR = ALLOC((\(sizeof(*PTR)\|sizeof(TYPE)\)), GFP);
+	TYPE *PTR = ALLOC_OBJ(*PTR, GFP);
|
	P =
-	ALLOC((\(sizeof(*P)\|sizeof(TYPE)\)), GFP)
+	ALLOC_OBJ(*P, GFP)
|
	DEREF =
-	ALLOC((sizeof(*DEREF)), GFP)
+	ALLOC_OBJ(*DEREF, GFP)
|
-	return ALLOC((sizeof(TYPE)), GFP);
+	return ALLOC_OBJ(TYPE, GFP);
|
	DEREF =
-	ALLOC(struct_size(DEREF, FLEX, COUNT), GFP)
+	ALLOC_FLEX(*DEREF, FLEX, COUNT, GFP)
|
-	return ALLOC(struct_size_t(TYPE, FLEX, COUNT), GFP);
+	return ALLOC_FLEX(TYPE, FLEX, COUNT, GFP);
|
	P =
-	ALLOC_ARRAY(COUNT, \(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	ALLOC_OBJS(*P, COUNT, GFP)
|
	DEREF =
-	ALLOC_ARRAY(COUNT, sizeof(*DEREF), GFP)
+	ALLOC_OBJS(*DEREF, COUNT, GFP)
|
-	return ALLOC_ARRAY(COUNT, sizeof(TYPE), GFP);
+	return ALLOC_OBJS(TYPE, COUNT, GFP);
|
	P =
-	ALLOC_ARRAY(\(sizeof(*P)\|sizeof(TYPE)\), COUNT, GFP)
+	ALLOC_OBJS(*P, COUNT, GFP)
|
	DEREF =
-	ALLOC_ARRAY(sizeof(*DEREF), COUNT, GFP)
+	ALLOC_OBJS(*DEREF, COUNT, GFP)
|
-	return ALLOC_ARRAY(sizeof(TYPE), COUNT, GFP);
+	return ALLOC_OBJS(TYPE, COUNT, GFP);
)

@assign_sizeof depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
expression DEREF;
expression GFP;
expression SIZE;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_OBJ_SZ = ALLOC ## "_obj_sz";
@@

(
-	SIZE = sizeof(*DEREF);
	DEREF =
-	ALLOC(SIZE, GFP)
+	ALLOC_OBJ_SZ(*DEREF, GFP, &SIZE)
	;
|
-	SIZE = sizeof(TYPE);
	P =
-	ALLOC(SIZE, GFP)
+	ALLOC_OBJ_SZ(*P, GFP, &SIZE)
	;
|
-	SIZE = sizeof(*DEREF);
	... when != SIZE
-	DEREF = ALLOC(SIZE, GFP)
+	ALLOC_OBJ_SZ(*DEREF, GFP, &SIZE)
	;
|
-	SIZE = sizeof(TYPE);
	... when != SIZE
	P =
-	ALLOC(SIZE, GFP)
+	ALLOC_OBJ_SZ(*P, GFP, &SIZE)
	;
)

@assign_struct_size depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
expression DEREF;
expression GFP;
expression SIZE;
expression FLEX;
expression COUNT;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_FLEX_SZ = ALLOC ## "_flex_sz";
@@

(
-	SIZE = struct_size(*DEREF, FLEX, COUNT);
	DEREF =
-	ALLOC(SIZE, GFP)
+	ALLOC_FLEX_SZ(*DEREF, FLEX, COUNT, GFP, &SIZE)
	;
|
-	SIZE = struct_size_t(TYPE, FLEX, COUNT);
	P =
-	ALLOC(SIZE, GFP)
+	ALLOC_FLEX_SZ(*P, FLEX, COUNT, GFP, &SIZE)
	;
|
-	SIZE = struct_size(*DEREF, FLEX, COUNT);
	... when != SIZE
	DEREF =
-	ALLOC(SIZE, GFP)
+	ALLOC_FLEX_SZ(*DEREF, FLEX, COUNT, GFP, &SIZE)
	;
|
-	SIZE = struct_size_t(TYPE, FLEX, COUNT);
	... when != SIZE
	P =
-	ALLOC(SIZE, GFP)
+	ALLOC_FLEX_SZ(*P, FLEX, COUNT, GFP, &SIZE)
	;
)
