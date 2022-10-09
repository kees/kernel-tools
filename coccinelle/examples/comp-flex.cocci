// match memcpy() which has a composite flexible array struct as the destination
@memcpy_compflex_dest@
identifier outer, compflex;
struct outer *PTR;
expression SRC, SIZE;
@@

  memcpy(
       &PTR->compflex
  , SRC, SIZE)

// "level2" matches a composite flexible array struct (struct ending with "level1")
@level2@
identifier inner;
identifier memcpy_compflex_dest.outer, memcpy_compflex_dest.compflex;
@@
        struct outer {
                ...
                struct inner compflex;
        };

// "level1" matches a struct ending in a flexible array.
@level1@
identifier level2.inner, flex;
type T;
@@
        struct inner {
                ...
(
                T flex[1];
|
                T flex[0];
|
                T flex[];
|
                T flex;
)
        };

// match memcpy() which has a composite flexible array struct as the destination
@depends on level1@
identifier memcpy_compflex_dest.outer, memcpy_compflex_dest.compflex;
struct outer *memcpy_compflex_dest.PTR;
expression SRC, SIZE;
@@

  memcpy(
*       &PTR->compflex
  , SRC, SIZE)
