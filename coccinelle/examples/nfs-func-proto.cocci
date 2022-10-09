@find@
identifier func;
type T, opsT;
identifier ops, N;
@@

 opsT ops[] = {
	[N] =
 	(T)
	func,
 };

@already_void@
identifier find.func;
identifier name;
@@

 func(...,
-void
+union nfsd4_op_u
 *name)
 {
	...
 }

@proto depends on !already_void@
identifier find.func;
type T;
identifier name;
position p;
@@

 func@p(...,
 	T name
 ) {
	...
   }

@script:python get_member@
type_name << proto.T;
member;
@@

coccinelle.member = cocci.make_ident(type_name.split("_", 1)[1].split(' ',1)[0])

@convert@
identifier find.func;
type proto.T;
identifier proto.name;
position proto.p;
identifier get_member.member;
@@

 func@p(...,
-	T name
+	union nfsd4_op_u *u
 ) {
+	T name = &u->member;
	...
   }

@cast@
identifier find.func;
type T, opsT;
identifier ops, N;
@@

 opsT ops[] = {
	[N] =
-	(T)
	func,
 };

