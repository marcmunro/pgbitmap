pgbitmap - Bitmap Extension for Postgres  
========================================

This extension creates a space-optimised, non-sparse, bitmap type for
postgres.

A bitmap is an array of bits, indexed by an integer.  Bitmaps provide an
efficient means to implement sets and `pgbitmap` provides operations
for: 

- creating new bitmaps;
- adding an element to a bitmap;
- removing an element from a bitmap;
- testing for inclusion of an element in a bitmap;
- finding the minimum and maximum bits in the bitmap;
- unioning bitmaps together (set union/logical or);
- intersecting bitmaps (set intersection/logical and);
- subtracting one bitmap from another;
- converting bitmaps to and from textual representations;
- converting bitmaps to and from arrays;
- aggregating bits, and bitmaps, into bitmaps.

Status
======

This is a beta release.  The plan is to give it real-life usage before
releasing a production version if all looks well.

There are no known bugs or deficiencies.  If you find any problems or
want enhancements, contact me and I will do what I can to respond
quickly. 

Change History
==============

0.2 (alpha) Initial release

0.3 (alpha) Fix for bitmap corruption when adding bit to empty bitmap.

0.5 (alpha) Change name of extension to pgbitmap from bitmap.

0.6 (alpha) Minor fixes to Makefiles find_pg_config and docs

0.9 (beta) Minor updates to documentation and to allow distribution
through pgxn.  Updated to Beta status as it all seems stable enough.  

0.9.1 (beta) Updated to convince pgxn to fully index the extension.

Doxygen Docs
============

`pgbitmap` is documented internally using Doxygen, with this page
acting as the start page.  The current docs can be found
[here](https://marcmunro.github.io/pgbitmap/docs/html/index.html).

Why not use the Postgres Bitstring Type?
========================================

The standard Postgres bit type is limited in a number of ways.  In
particular, each bit string starts at bit zero so a bitstring for bit
1,000,000 would contain the overhead of 1,000,000 zero bits.  It also,
currently, does not have all of the functionality that this bitmap
type provides.

What is this useful for?
========================

Pgbitmap was developed as a means of implementing sets of integers.  It is
particularly suited for managing sets of privileges for Virtual Private
Database implementations. 

API Summary
===========

Functions:
```.c
    bitmap_in(text) -> bitmap
    
    bitmap_out(bitmap) -> text
    
    bitmap() -> bitmap                          implemented by bitmap_new_empty()

    bitmap(integer) -> bitmap                   implemented by bitmap_new()

    bitmap_setbit(bitmap, integer) -> bitmap

    bitmap_testbit(bitmap, integer) -> boolean

    bitmap_clearbit(bitmap, integer) -> bitmap

    is_empty(bitmap) -> boolean                 implemented by bitmap_is_empty()

    bitmin(bitmap) -> integer                   implemented by bitmap_bitmin()

    bitmax(bitmap) -> integer                   implemented by bitmap_bitmax()

    bitmap_setmin(bitmap, integer) -> bitmap

    bitmap_setmax(bitmap, integer) -> bitmap

    bitmap_equal(bitmap, bitmap) -> boolean

    bitmap_nequal(bitmap, bitmap) -> boolean

    bitmap_lt(bitmap, bitmap) -> boolean

    bitmap_le(bitmap, bitmap) -> boolean

    bitmap_gt(bitmap, bitmap) -> boolean

    bitmap_ge(bitmap, bitmap) -> boolean

    bitmap_cmp(bitmap, bitmap) -> integer

    bitmap_union(bitmap, bitmap) -> bitmap

    bitmap_intersection(bitmap, bitmap) -> bitmap

    bitmap_minus(bitmap, bitmap) -> bitmap

    to_array(bitmap) -> array of integer        

    to_bitmap(array of integer) -> bitmap       implemented using aggregate bitmap_of()
```

Set Returning Functions:
```.c
    bits(bitmap) -> set of integer              implemented by bitmap_bits()
```

Operators:
```.c
    bitmap + integer -> bitmap                  implemented by bitmap_setbit()

    bitmap ? integer -> boolean                 implemented by bitmap_testbit()

    bitmap - integer -> bitmap                  implemented by bitmap_clearbit()

    bitmap = bitmap -> boolean                  implemented by bitmap_equal()

    bitmap <> bitmap -> boolean                 implemented using bitmap_equal()

    bitmap < bitmap -> boolean                  implemented by bitmap_lt()

    bitmap <= bitmap -> boolean                 implemented by bitmap_le()

    bitmap > bitmap -> boolean                  implemented by bitmap_gt()

    bitmap >= bitmap -> boolean                 implemented by bitmap_ge()

    bitmap + bitmap -> bitmap                   implemented by bitmap_union()

    bitmap * bitmap -> bitmap                   implemented by bitmap_intersection()

    bitmap - bitmap -> bitmap                   implemented by bitmap_minus()
```

Aggregates:
```.c
    bitmap_of(integer) -> bitmap                implemented by bitmap_setbit()

    union_of(bitmap) -> bitmap                  implemented by bitmap_union()

    intersect_of(bitmap) -> bitmap              implemented by bitmap_intersection()
```

API Details and Examples
========================

Conversion to and from text
---------------------------
```
    bitmap_in(text) -> bitmap

    bitmap_out(bitmap) -> text
```

The bitmap type has a compact textual representation that is not
intended to be human-readable.  This textual representation enables
bitmaps to be used in hstore, and in text-based backups.

In addition to the functions described above, casts, `::text`, `::bitmap`,
can also be used.

Creating bitmaps
----------------
```
    bitmap() -> bitmap

    bitmap(integer) -> bitmap

    to_bitmap(array of integer) -> bitmap

    bitmap_of(aggregate of integer) -> bitmap
```
    
An empty bitmap can be created using `bitmap()`.  A bitmap with a single
element included (ie a single bit set to 1) can also be created using
`bitmap(n)`.  Bitmaps are more usually created from arrays or queries.
The following queries return identical bitmaps:

```
    select bitmap() + 1 + 2 + 3;

    select bitmap(1) + 2 + 3;

    select bitmap_setbit(bitmap_setbit(bitmap(1), 2), 3);
    
    select to_bitmap('{1, 2, 3}');

    select array[1, 2, 3]::bitmap;

    select bitmap_of(x)
      from generate_series(1, 3) x;
```

Bit Manipulation and Testing
----------------------------
```
    bitmap_setbit(bitmap, integer) -> bitmap

    bitmap + integer -> bitmap

    bitmap_clearbit(bitmap, integer) -> bitmap

    bitmap - integer -> bitmap

    bitmap_testbit(bitmap, integer) -> boolean

    bitmap ? integer -> boolean
```
Elements can be added to a bitmap using the `bitmap_setbit()` function
or the `+` operator.  They can be removed using `bitmap_clearbit()` or
the `-` operator, and can be tested using the `bitmap_testbit()`
function or the `?` operator.  The setbit and clearbit functions are
rarely directly used in SQL, as array or aggregation operations are
usually faster.

This is how you might test for a privilege, in a round-about sort of way:
```
   select bitmap_of(privilege_id) ? 42
     from my_privileges;
```
Bitmap Range Functions
----------------------
```
    is_empty(bitmap) -> boolean

    bitmin(bitmap) -> integer

    bitmin(bitmap) -> integer

    bitmap_setmin(bitmap, integer) -> bitmap

    bitmap_setmax(bitmap, integer) -> bitmap
```
Bitmaps are stored as ranges of bits.  There are a number of functions
for checking and manipulating bitmap ranges.

`is_empty()` returns true if the bitmap contains no elements.

`bitmin()` returns the lowest value element in the bitmap (ie the lowest
bit that is set).  If the bitmap is empty, it returns null.

`bitmax()` returns the highest element in the bitmap, or null if the bitmap is
empty.

`bitmap_setmin()` and `bitmap_setmax()` can be used to efficiently clear
large sections of a bitmap.  The result of:

```
select bitmap_setmin(bitmap_of(x), 200)
      from generate_series(1, 205) x;
```

would be a bitmap with elements 200 to 205.

Bitmap Comparison Functions and Operators
-----------------------------------------
```
    bitmap_equal(bitmap, bitmap) -> boolean

    bitmap_nequal(bitmap, bitmap) -> boolean

    bitmap_lt(bitmap, bitmap) -> boolean

    bitmap_le(bitmap, bitmap) -> boolean

    bitmap_gt(bitmap, bitmap) -> boolean

    bitmap_ge(bitmap, bitmap) -> boolean

    bitmap_cmp(bitmap, bitmap) -> integer

    bitmap = bitmap -> boolean

    bitmap <> bitmap -> boolean

    bitmap < bitmap -> boolean

    bitmap <= bitmap -> boolean

    bitmap > bitmap -> boolean

    bitmap >= bitmap -> boolean
```
Bitmaps may be compared.  This is primarily for the purpose of sorting
and indexing.  Testing for equality or inequality is probably the only
useful comparison from an API perspective.

Set Operations on Bitmaps
-------------------------
```
    bitmap_union(bitmap, bitmap) -> bitmap

    bitmap_intersection(bitmap, bitmap) -> bitmap

    bitmap_minus(bitmap, bitmap) -> bitmap

    bitmap + bitmap -> bitmap

    bitmap * bitmap -> bitmap

    bitmap - bitmap -> bitmap
```
These functions and operators act on a pair of bitmaps to yield a
result.

`bitmap_union()`, the `+` operator, returns a bitmap containing all
elements from both arguments.  The following queries return identical
results:

```
    select bitmap_union(to_bitmap({'1 2 3'}),
                        to_bitmap({'1 3 5'}));

    select to_bitmap({'1 2 3 5}');
```

`bitmap_intersect()`, the `*` operator, yields the set of common elements
from its arguments.  The following queries return identical results:
```
    select to_bitmap({'1 2 3}') * to_bitmap('{3, 4, 5}');

    select bitmap(3);
```

`bitmap_minus()`, the `-` operator, yields a bitmap containing all elements
from the first argument that do not appear in the second.  These queries
return identical results:
```
    select to_bitmap({'1 2 3}') - to_bitmap('{3, 4, 5}');

    select to_bitmap('{1, 2}');
```

Extracting all Elements of a Bitmap
-----------------------------------
```
    to_array(bitmap) -> array of integer

    bits(bitmap) -> set of integer
```

These functions return the elements of a bitmap, either as an array, or
as a set.  Use the set returning function like this:
```
    select bits as privilege_id
      from bits(privileges);
```
The `to_array()` function can also be invoked as a cast, eg:
```
    select privileges::int4[];
```

Bitmap Aggregates
-----------------
```
    union_of(bitmap) -> bitmap

    intersect_of(bitmap) -> bitmap
```

These functions aggregate a collection of bitmaps using the union or
intersect operations.  Eg to identify all privileges of all members of a
group of offices, we could use something like this:
```
    select union_of(privs) as office_privs, office_name
      from user_privs
     where office_name like '%admin%'
     group by office_name;
```

Installing pgbitmap using pgxn
------------------------------

If you're using the pgxn client all you need to do is this: 
```
    $ pgxn install pgbitmap
    $ pgxn load -d mydb pgbitmap
```

Building pgbitmap Manually
--------------------------

Pgbitmap can be built using the standard Postgres PGXS build mechanism
as described here
[https://www.postgresql.org/docs/12/extend-pgxs.html].

The build will need to be able to find the pg_config executable that
matches your Postgres version.  It will attempt to find this using
`find_pg_config ` (in the top-level pgbitmap directory).  If it cannot
find pg_config the build will fail.

You can manually define the location in the `PG_CONFIG` file:
```
  $ echo <path to pg_config> >PG_CONFIG
```

From the pgbitmap directory (the root directory of the extension), use
the following commands:
```
  $ make
```
To build the extension, followed by:

```
  $ sudo make install
```
To install it.  You may then need to stop and restart your database
service to have postgres recognise the extension.

To test the installation use:

```
  $ make test
```

To create html documentation (in docs/html/index.html) use:

```
  $ make docs
```

You will need to have doxygen and dot installed.

