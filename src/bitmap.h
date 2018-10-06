/**
 * @file   bitmap.h
 * \code
 *     Author:       Marc Munro
 *     Copyright (c) 2015i, 2018 Marc Munro
 *     License:      BSD
 * 
 * \endcode
 * @brief  
 * Define bitmap datatypes 
 * 
 */

#include "postgres.h"
#include "funcapi.h"

#ifndef VEIL_DATATYPES
/** 
 * Prevent this header from being included multiple times.
 */
#define VEIL_DATATYPES 1

/* Bitmaps will be based on 64-bit integers if pointer types are 64-bit,
 * unless FORCE_32_BIT is defined.
 */
#ifdef FORCE_32_BIT
#undef USE_64_BIT
#else
#if (SIZEOF_VOID_P == 8)
#define USE_64_BIT 1
#else
#undef USE_64_BIT
#endif
#endif

#ifdef USE_64_BIT
#define ELEMBITS 64
#else
#define ELEMBITS 32
#endif


/**
 * Gives the bitmask index for the bitzero value of a Bitmap.  This is
 * part of the "normalisation" process for bitmap ranges.  This process
 * allows unlike bitmaps to be more easily compared by forcing bitmap
 * indexes to be normalised around 32 or 64 bit word boundaries.
 * 
 * @param x The bitzero value of a bitmap
 * 
 * @return The bitmask index representing x.
 */
#ifdef USE_64_BIT
#define BITZERO(x) (x & 0xffffffffffffffc0)
#else
#define BITZERO(x) (x & 0xffffffe0)
#endif

/**
 * Gives the bitmask index for the bitmax value of a bitmap.  See
 * BITZERO() for more information.
 *
 * @param x The bitmax value of a bitmap
 *
 * @return The bitmask index representing x.
 */
#ifdef USE_64_BIT
#define BITMAX(x) (x | 0x3f)
#else
#define BITMAX(x) (x | 0x1f)
#endif

/**
 * Gives the index of a bit within the array of 32-bit words that
 * comprise the bitmap.
 *
 * @param x The bit in question
 *
 * @return The array index of the bit.
 */
#ifdef USE_64_BIT
#define BITSET_ELEM(x) (x >> 6)
#else
#define BITSET_ELEM(x) (x >> 5)
#endif

/**
 * Gives the index into ::bitmasks for the bit specified in x.
 *
 * @param x The bit in question
 *
 * @return The bitmask index
 */
#ifdef USE_64_BIT
#define BITSET_BIT(x) (x & 0x3f)
#else
#define BITSET_BIT(x) (x & 0x1f)
#endif

/**
 * Gives the number of array elements in a ::Bitmap that runs from
 * element min to element max.
 *
 * @param min
 * @param max
 *
 * @return The number of elements in the bitmap.
 */
#ifdef USE_64_BIT
#define ARRAYELEMS(min,max) (((max - BITZERO(min)) >> 6) + 1)
#else
#define ARRAYELEMS(min,max) (((max - BITZERO(min)) >> 5) + 1)
#endif


/**
 * Return the smaller of a or b.  Note that expressions a and b may be
 * evaluated more than once.
 *
 * @param a
 * @param b
 *
 * @return The smaller value of a or b.
 */
#define MIN(a,b) ((a < b)? a: b)

/**
 * Return the larger of a or b.  Note that expressions a and b may be
 * evaluated more than once.
 *
 * @param a
 * @param b
 *
 * @return The smaller value of a or b.
 */
#define MAX(a,b) ((a > b)? a: b)

#ifdef USE_64_BIT
typedef uint64 bm_int;
#else
typedef uint32 bm_int;
#endif

/**
 * A bitmap is stored as an array of integer values.  Note that the size
 * of a Bitmap structure is determined dynamically at run time as the
 * size of the array is only known then.
 */
typedef struct Bitmap {
	char    vl_len[4];  /**< Standard postgres length header */
    int32   bitzero;	/**< The index of the lowest bit the bitmap can
						 * store */
    int32   bitmax;		/**< The index of the highest bit the bitmap can
						 * store */
	bm_int  bitset[0];  /**< Element zero of the array of int4 values
						 * comprising the bitmap. */
} Bitmap;

typedef unsigned char boolean;

#define DatumGetBitmap(x)	((Bitmap *) DatumGetPointer(x))
#define PG_GETARG_BITMAP(x)	DatumGetBitmap( \
		PG_DETOAST_DATUM(PG_GETARG_DATUM(x)))
#define PG_RETURN_BITMAP(x)	PG_RETURN_POINTER(x)


extern Datum bitmap_in(PG_FUNCTION_ARGS);
extern Datum bitmap_out(PG_FUNCTION_ARGS);
extern Datum bitmap_is_empty(PG_FUNCTION_ARGS);
extern Datum bitmap_bits(PG_FUNCTION_ARGS);
extern Datum bitmap_new_empty(PG_FUNCTION_ARGS);
extern Datum bitmap_new(PG_FUNCTION_ARGS);
extern Datum bitmap_bitmin(PG_FUNCTION_ARGS);
extern Datum bitmap_bitmax(PG_FUNCTION_ARGS);
extern Datum bitmap_setbit(PG_FUNCTION_ARGS);
extern Datum bitmap_testbit(PG_FUNCTION_ARGS);
extern Datum bitmap_union(PG_FUNCTION_ARGS);
extern Datum bitmap_clearbit(PG_FUNCTION_ARGS);
extern Datum bitmap_union(PG_FUNCTION_ARGS);
extern Datum bitmap_intersection(PG_FUNCTION_ARGS);


#endif
