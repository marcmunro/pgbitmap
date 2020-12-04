/**
 * @file   pgbitmap.h
 * \code
 *     Author:       Marc Munro
 *     Copyright (c) 2020 Marc Munro
 *     License:      BSD
 * 
 * \endcode
 * @brief  
 * Define bitmap datatypes 
 * 
 */

#include "postgres.h"
#include "funcapi.h"

#ifndef BITMAP_DATATYPES
/** 
 * Prevent this header from being included multiple times.
 */
#define BITMAP_DATATYPES 1

/* Bitmaps will be based on 64-bit integers if pointer types are 64-bit,
 * unless FORCE_32_BIT is defined.
 */
#ifdef FORCE_32_BIT
#undef USE_64_BIT
#else
#if (SIZEOF_VOID_P == 8)
/**
 * Use 64-bit word definitions throughout.
 */
#define USE_64_BIT 1
#else
#undef USE_64_BIT
#endif
#endif

#ifdef USE_64_BIT
/**
 * Use 64-bit words.
 */
#define ELEMBITS 64
#else
/**
 * Use 32-bit words.
 */
#define ELEMBITS 32
#endif

//#define BITMAP_DEBUG 1
#ifdef BITMAP_DEBUG
#define DBG_ELEMS 1
#define SETCANARY(b) do {									\
		b->bitset[ARRAYELEMS(b->bitmin, b->bitmax)] = 0;	\
		b->canary = 0;										\
	} while (false)

#define CHKCANARY(b)										\
	((b->bitset[ARRAYELEMS(b->bitmin, b->bitmax)] == 0) &&	\
	 (b->canary == 0))

#ifdef USE_64_BIT
#define CANARYELEM uint64 canary;
#else
#define CANARYELEM uint32 canary;
#endif
#else
#define DBG_ELEMS 0
#define SETCANARY(b) 
#define CHKCANARY(b) true
#define CANARYELEM
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
#define BITZERO(x) ((x) & 0xffffffffffffffc0)
#else
#define BITZERO(x) ((x) & 0xffffffe0)
#endif

/**
 * Gives the index of the word for a given bit, assuming bitmin is zero.
 *
 * @param x The bit in question
 *
 * @return The array index of the bit.
 */
#ifdef USE_64_BIT
#define BITSET_ELEM(x) ((x) >> 6)
#else
#define BITSET_ELEM(x) ((x) >> 5)
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
/**
 * bm_int is the bitmap integer type (a 64-bit value).
 */
typedef uint64 bm_int;
#else
/**
 * bm_int is the bitmap integer type (a 32-bit value).
 */
typedef uint32 bm_int;
#endif

/**
 * A bitmap is stored as an array of integer values.  Note that the size
 * of a Bitmap structure is determined dynamically at run time as the
 * size of the array is only known then.
 */
typedef struct Bitmap {
	char    vl_len[4];  /**< Standard postgres length header */
    int32   bitmin;	    /**< The index of the lowest bit the bitmap has
						 * stored */
    int32   bitmax;		/**< The index of the highest bit the bitmap has
						 * stored */
	CANARYELEM
	bm_int  bitset[0];  /**< Element zero of the array of int4 values
						 * comprising the bitmap. */
} Bitmap;

/**
 * Defines a boolean type to make our code more readable.
 */
typedef unsigned char boolean;

/**
 * Provide a macro for getting a bitmap datum.
 */
#define DatumGetBitmap(x)	((Bitmap *) PG_DETOAST_DATUM(DatumGetPointer(x)))

/**
 * Provide a macro for dealing with bitmap arguments.
 */
#define PG_GETARG_BITMAP(x)	DatumGetBitmap(		\
		PG_DETOAST_DATUM(PG_GETARG_DATUM(x)))
/**
 * Provide a macro for returning bitmap results.
 */
#define PG_RETURN_BITMAP(x)	PG_RETURN_POINTER(x)

extern bool bitmapTestbit(Bitmap *bitmap, int32 bit);
extern Bitmap *bitmapCopy(Bitmap *bitmap);

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
extern Datum bitmap_setmin(PG_FUNCTION_ARGS);
extern Datum bitmap_setmax(PG_FUNCTION_ARGS);
extern Datum bitmap_equal(PG_FUNCTION_ARGS);
extern Datum bitmap_nequal(PG_FUNCTION_ARGS);
extern Datum bitmap_union(PG_FUNCTION_ARGS);
extern Datum bitmap_clearbit(PG_FUNCTION_ARGS);
extern Datum bitmap_union(PG_FUNCTION_ARGS);
extern Datum bitmap_intersection(PG_FUNCTION_ARGS);
extern Datum bitmap_minus(PG_FUNCTION_ARGS);
extern Datum bitmap_lt(PG_FUNCTION_ARGS);
extern Datum bitmap_le(PG_FUNCTION_ARGS);
extern Datum bitmap_gt(PG_FUNCTION_ARGS);
extern Datum bitmap_ge(PG_FUNCTION_ARGS);
extern Datum bitmap_cmp(PG_FUNCTION_ARGS);


#endif
