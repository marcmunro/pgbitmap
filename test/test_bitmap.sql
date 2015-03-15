\set ECHO none
\set QUIET 1
-- Turn off echo and keep things quiet.

-- Format the output for nice TAP.
\pset format unaligned
\pset tuples_only true
\pset pager

-- Revert all changes on failure.
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
\set QUIET 1

begin;
create extension pgtap;
create extension bitmap;

select plan(32);

select is((select count(*) from bits(bitmap()))::integer,
          0, 'Check empty bitmap');

select is((select count(*) from bits(bitmap(532)))::integer, 
	  1, 'Check non-empty bitmap - 1');

select is((select bits from bits(bitmap(532))), 532,
          'Check non-empty bitmap  2');

select is((select * from bits(cast(cast(bitmap(532) as text) as bitmap))),
	  532, 'Check bitmap  serialisation');

select is(is_empty(bitmap()), true, 'Check empty bitmap');

select is(is_empty(bitmap(5)), false, 'Check non-empty bitmap');

select is(bitmin(bitmap()), null, 'Bitmin of empty bitmap');
select is(bitmin(bitmap(14)), 14, 'Bitmin of bitmap - 1');
select is(bitmin(bitmap(23)), 23, 'Bitmin of bitmap - 2');

select is(bitmax(bitmap()), null, 'Bitmax of empty bitmap');
select is(bitmax(bitmap(14)), 14, 'Bitmax of bitmap - 1');
select is(bitmax(bitmap(23)), 23, 'Bitmax of bitmap - 2');

select is(bitmax(bitmap_setbit(bitmap(2099), 23)), 2099, 
         'bitmap_setbit - 1');

select is((select bits 
             from bits(bitmap_setbit(bitmap(2099), 23))
            where bits = 23), 23, 'bitmap_setbit - 2');

select is((select bits 
             from bits(bitmap_setbit(bitmap(2099), 23))
            where bits = 2099), 2099, 'bitmap_setbit - 3');

select is((select count(*)
             from bits(bitmap(2099) + 23))::integer, 
	  2, 'bitmap_setbit - 4');

select is((select count(*)
             from bits(bitmap(2099) + 23 + 217))::integer, 
	  3, 'bitmap_setbit - 5');

select is((select bits 
             from bits(bitmap(2099) + 23 + 217)
            where bits = 217), 217, 'bitmap_setbit - 6');

select is((select bits 
             from bits(bitmap(23) + 217 + 2099)
            where bits = 217), 217, 'bitmap_setbit - 7');

select is((select count(*)
             from bits(bitmap(23) + 217 + 2099))::integer, 
	  3, 'bitmap_setbit - 8');

select is((bitmap(23) + 217 + 2099) ? 23,
	  true, 'bitmap_testbit - 1');

select is((bitmap(23) + 217 + 2099) ? 217,
	  true, 'bitmap_testbit - 2');

select is((bitmap(23) + 217 + 2099) ? 4100,
	  false, 'bitmap_testbit - 3');

select is((bitmap(23) + bitmap(217)) ? 217,
          true, 'bitmap_union - 1');

select is((bitmap(23) + bitmap(217)) ? 23,
          true, 'bitmap_union - 2');

select is((bitmap(23) + bitmap(217)) ? 47,
          false, 'bitmap_union - 3');

select is((select count(*) from bits(bitmap(23) + bitmap(217)))::integer,
          2, 'bitmap_union - 4');

select is(bitmax(bitmap(23) + 217 + 2099 - 2099), 217,
          'bitmap_clearbit - 1');

select is((select bits
             from bits(bitmap(23) + 217 + 2099 - 2099)
	    where bits = 23), 
	  23, 'bitmap_clearbit - 2');

select is((select count(*)
             from bits(bitmap(23) + 217 + 2099 - 2099))::integer,
	  2, 'bitmap_clearbit - 3');

select is(bitmin(bitmap(23) + 217 + 2099 - 23), 217,
          'bitmap_clearbit - 4');

select is((select count(*)
             from bits((bitmap(23) + 217 + 2100 + 2099) *
	               (bitmap(217) + 2099)))::integer,
	  2, 'bitmap_intersection- 1');

-- There should be tests for the aggregate and array operations too.

-- Finish the tests and clean up.
select * from finish();
rollback;

