// Options: --no-includes --include-headers

@noprof@
identifier ALLOC =~ "kv?[mz]alloc_noprof";
@@

	ALLOC(...)

@direct depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
expression GFP;
expression COUNT;
expression FLEX;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_OBJ = ALLOC ## "_obj";
fresh identifier ALLOC_FLEX = ALLOC ## "_flex";
@@

(
-	P = ALLOC(\(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	ALLOC_OBJ(P, GFP)
|
-	P = ALLOC(struct_size(P, FLEX, COUNT), GFP)
+	ALLOC_FLEX(P, FLEX, COUNT, GFP)
|
-	P = kmalloc_array(COUNT, \(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	kmalloc_objs(P, COUNT, GFP)
|
-	P = kmalloc_array(\(sizeof(*P)\|sizeof(TYPE)\), COUNT, GFP)
+	kmalloc_objs(P, COUNT, GFP)
|
-	P = kcalloc(COUNT, \(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	kzalloc_objs(P, COUNT, GFP)
|
-	P = kcalloc(\(sizeof(*P)\|sizeof(TYPE)\), COUNT, GFP)
+	kzalloc_objs(P, COUNT, GFP)
|
-	P = kvmalloc_array(COUNT, \(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	kvmalloc_objs(P, COUNT, GFP)
|
-	P = kvmalloc_array(\(sizeof(*P)\|sizeof(TYPE)\), COUNT, GFP)
+	kvmalloc_objs(P, COUNT, GFP)
|
-	P = kvcalloc(COUNT, \(sizeof(*P)\|sizeof(TYPE)\), GFP)
+	kvzalloc_objs(P, COUNT, GFP)
|
-	P = kvcalloc(\(sizeof(*P)\|sizeof(TYPE)\), COUNT, GFP)
+	kvzalloc_objs(P, COUNT, GFP)
)

@assign_sizeof depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
expression GFP;
expression SIZE;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_OBJ_SZ = ALLOC ## "_obj_sz";
@@

(
-	SIZE = sizeof(*P);
-	P = ALLOC(SIZE, GFP);
+	ALLOC_OBJ_SZ(P, GFP, &SIZE);
|
-	SIZE = sizeof(TYPE);
-	P = ALLOC(SIZE, GFP);
+	ALLOC_OBJ_SZ(P, GFP, &SIZE);
|
-	SIZE = sizeof(*P);
	... when != SIZE
-	P = ALLOC(SIZE, GFP);
+	ALLOC_OBJ_SZ(P, GFP, &SIZE);
|
-	SIZE = sizeof(TYPE);
	... when != SIZE
-	P = ALLOC(SIZE, GFP);
+	ALLOC_OBJ_SZ(P, GFP, &SIZE);
)

@assign_struct_size depends on !noprof && !(file in "tools") && !(file in "samples")@
type TYPE;
TYPE *P;
expression GFP;
expression SIZE;
expression FLEX;
expression COUNT;
identifier ALLOC =~ "kv?[mz]alloc";
fresh identifier ALLOC_FLEX_SZ = ALLOC ## "_flex_sz";
@@

(
-	SIZE = struct_size(*P, FLEX, COUNT);
-	P = ALLOC(SIZE, GFP);
+	ALLOC_FLEX_SZ(P, FLEX, COUNT, GFP, &SIZE);
|
-	SIZE = struct_size_t(TYPE, FLEX, COUNT);
-	P = ALLOC(SIZE, GFP);
+	ALLOC_FLEX_SZ(P, FLEX, COUNT, GFP, &SIZE);
|
-	SIZE = struct_size(*P, FLEX, COUNT);
	... when != SIZE
-	P = ALLOC(SIZE, GFP);
+	ALLOC_FLEX_SZ(P, FLEX, COUNT, GFP, &SIZE);
|
-	SIZE = struct_size_t(TYPE, FLEX, COUNT);
	... when != SIZE
-	P = ALLOC(SIZE, GFP);
+	ALLOC_FLEX_SZ(P, FLEX, COUNT, GFP, &SIZE);
)
