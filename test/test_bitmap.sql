-- Format the output for quiet tests.
\set ECHO none
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager


-- If there were any failures we need to fail with an error status
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true

begin;

create or replace
function record_test(n integer) returns bool as
$$
begin
  create temporary table if not exists my_tests (
    tests_run    bitmap
  );
  insert
    into my_tests
  select bitmap()
   where not exists (select 1 from my_tests);
  update my_tests set tests_run = tests_run + n;
  return false;
end;
$$
language 'plpgsql' security definer volatile
set client_min_messages = 'error';



-- Create functions for running tests.
create or replace
function expect(cmd text, n integer, msg text)
  returns bool as
$$
declare
  res integer;
  rc integer;
begin
  execute cmd into res;
  get diagnostics rc = row_count;
  if rc = 1 then
    if (res != n) or ((res is null) != (n is null)) then
      raise exception '%  Expecting %, got %', msg, n, res;
    end if;
  else
    raise exception '%  Expecting %, got no rows', msg, n;
  end if;
  return false;
end;
$$
language 'plpgsql' security definer volatile;

create or replace
function expect(cmd text, n bool, msg text)
  returns bool as
$$
declare
  res bool;
  rc integer;
begin
  execute cmd into res;
  get diagnostics rc = row_count;
  if rc = 1 then
    if (res != n) or ((res is null) != (n is null)) then
      raise exception '%  Expecting %, got %', msg, n, res;
    end if;
  else
    raise exception '%  Expecting %, got no rows', msg, n;
  end if;
  return false;
end;
$$
language 'plpgsql' security definer volatile;

create or replace
function expect(val integer, n integer, msg text)
  returns bool as
$$
begin
  if (val != n) or ((val is null) != (n is null)) then
    raise exception '%  Expecting %, got %', msg, n, val;
  end if;
  return false;
end;
$$
language 'plpgsql' security definer volatile;

create or replace
function expect(val bool, n bool, msg text)
  returns bool as
$$
begin
  if (val != n) or ((val is null) != (n is null)) then
    raise exception '%  Expecting %, got %', msg, n, val;
  end if;
  return false;
end;
$$
language 'plpgsql' security definer volatile;

create extension pgbitmap;

-- We put expect() into the where clause so that no rows are returned
-- from each test.  This enables much cleaner output.

-- Test bits() on empty and non-empty bitmaps
select null
 where record_test(1)
    or expect((select count(*) from bits(bitmap()))::integer,
          	0, 'EMPTY BITMAP SHOULD HAVE 0 BITS')
    or expect((select count(*) from bits(bitmap(532)))::integer, 
	        1, 'THIS NON-EMPTY-BITMAP SHOULD HAVE 1 BIT');

select null
 where record_test(2)
    or expect('select bits from bits(bitmap(532))',
       		532, 'NON-EMPTY-BITMAP NOT FOUND(2)');

-- Test that serialisation of bitmap yields sane results
select null
 where record_test(3)
    or expect('select * from bits(cast(cast(bitmap(532) as text) as bitmap))',
	        532, 'BITMAP SERIALISATION FAILS');

-- Test is_empty()
select null
 where record_test(4)
    or expect(is_empty(bitmap()),
		true, 'BITMAP SHOULD BE EMPTY')
    or  expect(is_empty(bitmap(5)),
		false, 'BITMAP SHOULD NOT BE EMPTY');

-- Test bitmin() 
select null
 where record_test(5)
    or expect(bitmin(bitmap()),
                null, 'BITMIN OF EMPTY BITMAP SHOULD BE NULL')
    or expect(bitmin(bitmap(14)),
                14, 'BITMIN SHOULD BE 14')
    or expect(bitmin(bitmap(73)),
                73, 'BITMIN SHOULD BE 73');

-- Test bitmax()
select null
 where record_test(6)
    or expect(bitmax(bitmap()),
                null, 'BITMAX OF EMPTY BITMAP SHOULD BE NULL')
    or expect(bitmax(bitmap(14)),
                14, 'BITMAX SHOULD BE 14')
    or expect(bitmax(bitmap(73)),
                73, 'BITMAX SHOULD BE 73');

-- Test setbit, testbit
with set1 as (
    select bitmap(2099) as bm1),
set2 as (
    select bitmap_setbit(bm1, 199) as bm2 from set1),
set3 as (
    select bm2 + 23 as bm3 from set2),
bits as (
    select bits from set3 cross join bits(bm3))
select null
  from bits
 where record_test(7)
    or expect((select bits from bits where bits = 23),
              23, '23 MUST BE IN THE SET')
    or expect((select bits from bits where bits = 199),
              199, '199 MUST BE IN THE SET')
    or expect((select bits from bits where bits = 2099),
              2099, '2099 MUST BE IN THE SET')
union all
select null
 where expect((select count(*) from bits)::integer,
              3, 'THERE MUST BE 3 ELEMENTS IN BITS')
union all
select null
  from set3
 cross join set2
 cross join set1
 where expect(bitmin(bm3), 23, 'BITMIN OF SET3 MUST BE 23')
    or expect(bitmin(bm2), 199, 'BITMIN OF SET2 MUST BE 199')
    or expect(bitmin(bm1), 2099, 'BITMIN OF SET1 MUST BE 2099')
    or expect(bitmax(bm1), 2099, 'BITMAX OF SET1 MUST BE 2099')
    or expect(bitmax(bm2), 2099, 'BITMAX OF SET2 MUST BE 2099')
    or expect(bitmax(bm3), 2099, 'BITMAX OF SET3 MUST BE 2099')
    or expect(bitmap_testbit(bm3, 199), true, '199 MUST BE FOUND IN SET3')
    or expect(bm3 ? 199, true, '199 MUST BE FOUND IN SET3(2)')
    or expect(bitmap_testbit(bm1, 199), false, '199 MUST NOT BE FOUND IN SET1')
    or expect(bm1 ? 199, false, '199 MUST NOT BE FOUND IN SET1(2)');


-- Test clearbit operations
with set1 as (
    select bitmap(23) + 217 + 98 + 99 as bm1)
select null
  from set1
 where record_test(8)
    or expect((select count(*) from bits(bm1 - 217))::integer,
              3, 'THERE SHOULD BE 3 BITS AFTER CLEARING ONE')
    or expect((select count(*) from bits(bitmap_clearbit(bm1, 99)))::integer,
              3, 'THERE SHOULD BE 3 BITS AFTER CLEARING ONE(2)')
    or expect((select count(*) from bits(bitmap_clearbit(bm1, 7)))::integer,
              4, 'THERE SHOULD BE 4 BITS AFTER CLEARING NONE');


with set1 as (
    select bitmap(23) + 217 + 98 + 99 as bm1)
select null
  from set1
 where record_test(9)
    or expect((select count(*) from bits(bm1 - 217))::integer,
              3, 'THERE SHOULD BE 3 BITS AFTER CLEARING ONE')
    or expect((select count(*) from bits(bitmap_clearbit(bm1, 99)))::integer,
              3, 'THERE SHOULD BE 3 BITS AFTER CLEARING ONE(2)')
    or expect((select count(*) from bits(bitmap_clearbit(bm1, 7)))::integer,
              4, 'THERE SHOULD BE 4 BITS AFTER CLEARING NONE');

-- Test union operations
with set1 as (
    select bitmap(23) + bitmap(217) as bm1),
set2 as (
    select bitmap(98) + bitmap(99) as bm2)
select null
  from set1 cross join set2
 where record_test(10)
    or expect(bm1 ? 23, true, '23 MUST BE IN UNION')
    or expect(bm1 ? 217, true, '217 MUST BE IN UNION')
    or expect(bm2 ? 98, true, '98 MUST BE IN UNION')
    or expect(bm2 ? 99, true, '99 MUST BE IN UNION')
    or expect((bm1 + bm2) ? 217, true, '217 MUST BE IN SUPER-UNION')
    or expect((bm1 + bm2) ? 99, true, '99 MUST BE IN SUPER-UNION')
    or expect((select count(*) from bits(bm1 + bm2))::integer,
  	      4, 'SUPERUNION MUST HAVE 4 ENTRIES');
	       
-- Intersect and basic aggregate operations
with set1 as (
    select bitmap(23) + 24 + 27 + 28 as bm1),
set2 as (
    select bitmap_of(x) as bm2
      from (select generate_series as x
             from generate_series(26, 35)) s)
select null
  from set1 cross join set2
 where record_test(11)
    or expect((select count(*) from bits(bm1 * bm2))::integer,
              2, 'INTERSECTION SHOULD HAVE 2 ENTRIES')
    or expect((bm1 * bm2) ? 27, true, '27 SHOULD BE IN INTERSECTION')
    or expect((bm1 * bm2) ? 28, true, '28 SHOULD BE IN INTERSECTION')
    or expect((bm1 * bm2) ? 26, false, '26 SHOULD NOT BE IN INTERSECTION');
 

-- Bitmap union as an aggregate
with set1 as (
    select bitmap(23) + 24 + 27 + 28 as bm1),
set2 as (
    select bitmap_of(x) as bm2
      from (select generate_series as x
             from generate_series(26, 35)) s),
all_sets as (
    select bm1 as bma from set1 union all select bm2 from set2),
set3 as (
    select union_of(bma) as bm3 from all_sets)
select null
  from set3
 where record_test(12)
    or expect((select count(*) from bits(bm3))::integer,
              12, 'BITMAP UNION SHOULD HAVE 12 ELEMENTS');


-- Array to bitmap
with array1 as (
  select array(select generate_series(99, 130)) as a1),
set1 as (
  select to_bitmap(a1) as bm1 -- bitmap from array
    from array1),
set2 as (
  select bitmap(123) + 124 as bm2),
set3 as (
  select bm1 * bm2 as bm3 -- Intersect 
    from set1 cross join set2)
select null
  from set3
 where record_test(13)
    or expect((select count(*) from bits(bm3))::integer, 2,
              'INTERSECT FROM ARRAY SHOULD HAVE 2 ELEMENTS')
    or expect(bm3 ? 123, true, 'INTERSECT FROM ARRAY SHOULD CONTAIN 123');

-- bitmap to array
with array1 as (
  select array(select generate_series(99, 130)) as a1),
set1 as (
  select to_bitmap(a1) as bm1 -- bitmap from array
    from array1),
array2 as (
  select to_array(bm1) as a2
    from set1)
select null
  from array1 cross join array2
 where record_test(14)
    or expect(a1 = a2, true,
              'ARRAYS SHOULD BE EQUAL');


-- bitmap equality
with array1 as (
  select array(select generate_series(99, 130)) as a1),
set1 as (
  select to_bitmap(a1) as bm1 -- bitmap from array
    from array1),
set2 as (
  select to_bitmap(a1) as bm2 -- bitmap from array
    from array1)
select null
  from set1 cross join set2
 where record_test(15)
    or expect(bm1 = bm2, true, 'BITMAPS SHOULD BE EQUAL')
    or expect((bm1 - 122) = bm2, false, 'BITMAPS SHOULD NOT BE EQUAL')
    or expect((bm1 - 122) != bm2, true, 'BITMAPS SHOULD NOT BE EQUAL(2)');


-- setmin
with set1 as (
  select to_bitmap('{13, 171, 222, 279, 322, 700}') as bm1),
set2 as (
  select bitmap_setmin(bm1, 171) as bm2 from set1)
select null
  from set2
 where record_test(16)
    or expect((select count(*) from bits(bm2))::integer,
              5, 'SHOULD BE 5 ELEMENTS AFTER SETMIN')
    or expect((select min(bits) from bits(bm2)),
              171, 'SETMIN SMALLEST SHOULD BE 171')
    or expect(bitmin(bm2), 171, 'SETMIN SMALLEST SHOULD BE 171(2)');

-- setmax
with set1 as (
  select to_bitmap('{13, 171, 222, 279, 322, 701, 900}') as bm1),
set2 as (
  select bitmap_setmax(bm1, 700) as bm2 from set1)
select null
  from set2
 where record_test(17)
    or expect((select count(*) from bits(bm2))::integer,
              5, 'SHOULD BE 5 ELEMENTS AFTER SETMAX')
    or expect((select max(bits) from bits(bm2)),
              322, 'SETMAX LARGEST SHOULD BE 322')
    or expect(bitmax(bm2), 322, 'SETMAX LARGEST SHOULD BE 322(2)');

-- bitmap_minus
with set1 as (
  select to_bitmap('{13, 171, 222, 279, 322, 701, 900}') as bm1),
set2 as (
  select to_bitmap('{13, 172, 223, 279, 322, 701, 900}') as bm2),
set3 as (
  select bm1 - bm2 as bm3 from set1 cross join set2),
set4 as (
  select bitmap_minus(bm2, bm1) as bm4 from set1 cross join set2),
set5 as (
  select to_bitmap('{171, 222}') as bm5),  -- bm1 - bm2
set6 as (
  select to_bitmap('{172, 223}') as bm6)   -- bm2 - bm1
select null
  from set3 cross join set4 cross join set5 cross join set6
 where record_test(18)
    or expect(bm3 = bm5, true, 'BM3 SHOULD MATCH BM5')
    or expect(bm4 = bm6, true, 'BM4 SHOULD MATCH BM6');

-- intersect aggregate
with set1 as (
  select to_bitmap('{13, 171, 222, 279, 322, 701, 900}') as bm1),
set2 as (
  select to_bitmap('{13, 172, 223, 279, 322, 701, 900}') as bm2),
set3 as (
  select to_bitmap('{172, 223, 279, 322, 701}') as bm3),
setall as (
  select bm1 as bma from set1
  union 
  select bm2 from set2
  union 
  select bm3 from set3),
isect as (
  select intersect_of(bma) bi from setall)
select null
  from isect
 where record_test(19)
    or expect((select count(*) from bits(bi))::integer,
              3, 'INTERSECTION AGGREGATE SHOULD HAVE 3 ENTRIES')
    or expect(bi ? 279, true, '279 SHOULD BE IN INTERSECTION AGG')
    or expect(bi ? 322, true, '322 SHOULD BE IN INTERSECTION AGG')
    or expect(bi ? 701, true, '701 SHOULD BE IN INTERSECTION AGG');

-- Publish the set of tests that we have run
select 'Tests run: ' || to_array(tests_run)::text as "Passed tests"
  from my_tests;

-- Misceleaneous
-- Bitmap corruption bug in version 0.2
with x as (select (bitmap() + 1)::text)
select null from x
 where record_test(19);

-- Server crash bug in version 0.2
select union_of(x) from (select null::bitmap as x) x;

-- Null handling bug in version 0.3alpha
create or replace
function test3a(
    param in integer,
    privs out bitmap,
    x out integer)
  returns setof record as
$$
with base(ord, priv) as
(
  values (1, 1),
         (2, null),
	 (3, 3)
),
base2(ord, priv) as
(
  values (4, 4),
         (5, null),
	 (6, 6)
),
set1 as
(
  select bitmap_of(priv)
  from base
),
set2 as
(
  select bitmap_of(priv)
  from base2
),
set3(ord, privs, x) as
(
  select param, bitmap_of(priv), 1
  from base2
  union
  select 2, null, 1
  union
  select 3, bitmap_of(priv), 1
  from base
),
set4(privs, x) as
(
  select union_of(privs), x
    from set3
   group by x
)
select privs, x
  from set4;

$$
language 'sql';

select null
  from test3a(1) x
 cross join test3a(4) y
 where record_test(20)
    or expect(x.privs = y.privs, true,
              'test3a bitmaps are not equal');

rollback;
\echo ALL TESTS PASSED

