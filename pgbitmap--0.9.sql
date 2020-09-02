/* ----------
 * pgbitmap_interface.sqs (or a file derived from it)
 *
 *      Source file from which pgbitmap--<version>.sql is generated using
 *	sed. 
 *
 *      Copyright (c) 2020 Marc Munro
 *      Author:  Marc Munro
 *	License: BSD
 *
 * ----------
 */


create 
function bitmap_in(textin cstring) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_in'
     language C immutable strict;

comment on function bitmap_in(cstring) is
'Read the serialised string representation of a bitmap, TEXTIN, into a bitmap.';


create 
function bitmap_out(bitmap bitmap) returns cstring
     as '$libdir/pgbitmap', 'bitmap_out'
     language C immutable strict;

comment on function bitmap_out(bitmap) is
'create a serialised string representation of BITMAP.';


create type bitmap (
    input = bitmap_in,
    output = bitmap_out,
    internallength = variable,
    alignment = double,
    storage = main
);

comment on type bitmap is
'A set-of-bits type, intended for use in managing sets of privileges for
security purposes.';


create
function bits(bitmap bitmap) returns setof int4
     as '$libdir/pgbitmap', 'bitmap_bits'
     language C immutable strict;

comment on function bits(bitmap) is
'Return a set of integers showing the contents of BITMAP';


create
function bitmap() returns bitmap
     as '$libdir/pgbitmap', 'bitmap_new_empty'
     language C immutable strict;

comment on function bitmap() is
'Return an empty bitmap';

create
function bitmap(bitno int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_new'
     language C immutable strict;

comment on function bitmap(int4) is
'Return a bitmap containing the single bit provided in BITNO.';


create 
function is_empty(bitmap bitmap) returns boolean
     as '$libdir/pgbitmap', 'bitmap_is_empty'
     language C immutable strict;

comment on function is_empty(bitmap) is
'Predicate identifying whether BITMAP is empty';


create 
function bitmin(bitmap bitmap) returns int4
     as '$libdir/pgbitmap', 'bitmap_bitmin'
     language C immutable strict;

comment on function bitmin(bitmap) is
'Return the number of the minimum bit stored in BITMAP.  NULL, if no
bits';


create 
function bitmax(bitmap bitmap) returns int4
     as '$libdir/pgbitmap', 'bitmap_bitmax'
     language C immutable strict;

comment on function bitmax(bitmap) is
'Return the number of the maximum bit stored in BITMAP.  NULL, if no
bits';


create 
function bitmap_setbit(bitmap bitmap, bitno int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_setbit'
     language C immutable strict;

comment on function bitmap_setbit(bitmap, int4) is
'In BITMAP, set the bit given by BITNO';

create operator + (
    procedure = bitmap_setbit,
    leftarg = bitmap,
    rightarg = int4
);


create 
function bitmap_testbit(bitmap bitmap, bitno int4) returns bool
     as '$libdir/pgbitmap', 'bitmap_testbit'
     language C immutable strict;

comment on function bitmap_testbit(bitmap, int4) is
'In BITMAP, test the bit, BITNO, returning true if set, otherwise false.';

create operator ? (
    procedure = bitmap_testbit,
    leftarg = bitmap,
    rightarg = int4
);


create 
function bitmap_setmin(bitmap bitmap, bitmin int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_setmin'
     language C immutable strict;

comment on function bitmap_setmin(bitmap, int4) is
'In BITMAP, clear any bits that are less than BITMIN.';

create 
function bitmap_setmax(bitmap bitmap, bitmax int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_setmax'
     language C immutable strict;

comment on function bitmap_setmin(bitmap, int4) is
'In BITMAP, clear any bits that are greater than BITMAX.';

create 
function bitmap_equal(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_equal'
     language C immutable strict;

comment on function bitmap_equal(bitmap, bitmap) is
'Predicate returning true if bitmap1 and bitmap2 have the same bits set.';

create operator = (
    procedure = bitmap_equal,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = =,
    negator = <>
);

create 
function bitmap_nequal(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_nequal'
     language C immutable strict;

comment on function bitmap_nequal(bitmap, bitmap) is
'Predicate returning false if bitmap1 and bitmap2 have the same bits set.';

create operator <> (
    procedure = bitmap_nequal,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = <>,
    negator = =
);

create 
function bitmap_lt(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_lt'
     language C immutable strict;

create operator < (
    procedure = bitmap_lt,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = >,
    negator = >=
);

create 
function bitmap_le(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_le'
     language C immutable strict;

create operator <= (
    procedure = bitmap_le,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = >=,
    negator = >
);

create 
function bitmap_gt(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_gt'
     language C immutable strict;

create operator > (
    procedure = bitmap_gt,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = <,
    negator = <=
);

create 
function bitmap_ge(bitmap1 bitmap, bitmap2 bitmap) returns bool
     as '$libdir/pgbitmap', 'bitmap_ge'
     language C immutable strict;

create operator >= (
    procedure = bitmap_ge,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = <=,
    negator = <
);

create
function bitmap_cmp(bitmap, bitmap) returns int4
    as '$libdir/pgbitmap', 'bitmap_ge'
    language C immutable strict;

create operator class bitmap_ops
    default for type bitmap  using btree as
        operator        1       < ,
        operator        2       <= ,
        operator        3       = ,
        operator        4       >= ,
        operator        5       >,
        function        1       bitmap_cmp(bitmap, bitmap);



create 
function bitmap_union(bitmap1 bitmap, bitmap2 bitmap) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_union'
     language C immutable strict;

comment on function bitmap_union(bitmap, bitmap) is
'Return the union of BITMAP and BITMAP2';

create operator + (
    procedure = bitmap_union,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = +
);


create 
function bitmap_clearbit(bitmap bitmap, bitno int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_clearbit'
     language C immutable strict;

comment on function bitmap_clearbit(bitmap, int4) is
'In BITMAP, rest the bit, BITNO, to zero.';

create operator - (
    procedure = bitmap_clearbit,
    leftarg = bitmap,
    rightarg = int4
);


create 
function bitmap_intersection(bitmap1 bitmap, vitmap2 bitmap) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_intersection'
     language C immutable strict;

comment on function bitmap_intersection(bitmap, bitmap) is
'Return the intersection of BITMAP and BITMAP2';

create operator * (
    procedure = bitmap_intersection,
    leftarg = bitmap,
    rightarg = bitmap,
    commutator = *
);


create function bitmap_int_agg(bitmap bitmap, bitno int4) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_setbit'
     language C immutable;

create aggregate bitmap_of(integer) (
    sfunc = bitmap_int_agg,
    stype = bitmap);

comment on aggregate bitmap_of(integer) is
'Aggregate a set of integers into a bitmap';


create function bitmap_union_agg(bitmap1 bitmap,
                                 bitmap2 bitmap) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_union'
     language C immutable;

create aggregate union_of(bitmap) (
    sfunc = bitmap_union_agg,
    stype = bitmap);

comment on aggregate union_of(bitmap) is
'Union an aggregate of bitmaps into a single bitmap';


create function bitmap_intersect_agg(bitmap1 bitmap,
                                     bitmap2 bitmap) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_intersection'
     language C immutable;

create aggregate intersect_of(bitmap) (
    sfunc = bitmap_intersect_agg,
    stype = bitmap);

comment on aggregate intersect_of(bitmap) is
'Intersect an aggregate of bitmaps into a single bitmap';


create 
function to_array(bitmap)
  returns int[] as
$$
select array_agg(bits) from bits($1);
$$
language sql;

comment on function to_array(bitmap) is
'Convert a bitmap into an array - this may be a good way of getting a
user-readable version of a bitmap.';


create 
function to_bitmap(int[])
  returns bitmap as
$$
select bitmap_of(unnest) from unnest($1)
$$
language sql;

comment on function to_bitmap(int[]) is
'Convert an array of integers into a bitmap.';

create function bitmap_minus(bitmap1 bitmap, bitmap2 bitmap) returns bitmap
     as '$libdir/pgbitmap', 'bitmap_minus'
     language C immutable strict;

comment on function bitmap_minus(bitmap, bitmap) is
'Return a bitmap containing the bits from BITMAP1 with all matching bits
from BITMAP2 cleared.';

create operator - (
    procedure = bitmap_minus,
    leftarg = bitmap,
    rightarg = bitmap
);
