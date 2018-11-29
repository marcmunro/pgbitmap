/**
 * @file   bitmap.c
 * \code
 *     Author:       Marc Munro
 *     Copyright (c) 2015, 2018 Marc Munro
 *     License:      BSD
 * 
 * \endcode
 * @brief  
 * Define a bitmap type for postgres.
 * 
 */

#include "bitmap.h"

PG_MODULE_MAGIC;


/**
 * The length of a 64-bit integer as a base64 string.
 * This should really be 8 but the last char is always '=' so we can
 * optimise it away.
 */
#define INT32SIZE_B64          7 

/**
 * The length of a 32-bit integer as a base64 string.
 */
#define INT64SIZE_B64          12



#ifdef USE_64_BIT
/**
 * Array of bit positions for int64, indexed by bitno.
 */
static
uint64 bitmasks[64] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008, 
	0x00000010, 0x00000020, 0x00000040, 0x00000080, 
	0x00000100, 0x00000200, 0x00000400, 0x00000800, 
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x0000000100000000, 0x0000000200000000, 
	    0x0000000400000000, 0x0000000800000000, 
	0x0000001000000000, 0x0000002000000000, 
	    0x0000004000000000, 0x0000008000000000, 
	0x0000010000000000, 0x0000020000000000, 
	    0x0000040000000000, 0x0000080000000000, 
	0x0000100000000000, 0x0000200000000000, 
	    0x0000400000000000, 0x0000800000000000,
	0x0001000000000000, 0x0002000000000000, 
	    0x0004000000000000, 0x0008000000000000,
	0x0010000000000000, 0x0020000000000000, 
	    0x0040000000000000, 0x0080000000000000,
	0x0100000000000000, 0x0200000000000000, 
	    0x0400000000000000, 0x0800000000000000,
	0x1000000000000000, 0x2000000000000000, 
	    0x4000000000000000, 0x8000000000000000};

#else
/**
 * Array of bit positions for int32, indexed by bitno.
 */
static
uint32 bitmasks[] = {0x00000001, 0x00000002, 0x00000004, 0x00000008, 
				  	 0x00000010, 0x00000020, 0x00000040, 0x00000080, 
				  	 0x00000100, 0x00000200, 0x00000400, 0x00000800, 
				  	 0x00001000, 0x00002000, 0x00004000, 0x00008000,
				  	 0x00010000, 0x00020000, 0x00040000, 0x00080000,
				  	 0x00100000, 0x00200000, 0x00400000, 0x00800000,
				  	 0x01000000, 0x02000000, 0x04000000, 0x08000000,
				  	 0x10000000, 0x20000000, 0x40000000, 0x80000000};

#endif


/* BEGIN SECTION OF CODE COPIED FROM pgcrypto.c (via veil) */

static const char _base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const short b64lookup[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};

static unsigned
b64_encode(const char *src, unsigned len, char *dst)
{
	char	   *p,
			   *lend = dst + 76;
	const char *s,
			   *end = src + len;
	int			pos = 2;
	uint32		buf = 0;

	s = src;
	p = dst;

	while (s < end)
	{
		buf |= (unsigned char) *s << (pos << 3);
		pos--;
		s++;

		/* write it out */
		if (pos < 0)
		{
			*p++ = _base64[(buf >> 18) & 0x3f];
			*p++ = _base64[(buf >> 12) & 0x3f];
			*p++ = _base64[(buf >> 6) & 0x3f];
			*p++ = _base64[buf & 0x3f];

			pos = 2;
			buf = 0;
		}
		if (p >= lend)
		{
			*p++ = '\n';
			lend = p + 76;
		}
	}
	if (pos != 2)
	{
		*p++ = _base64[(buf >> 18) & 0x3f];
		*p++ = _base64[(buf >> 12) & 0x3f];
		*p++ = (pos == 0) ? _base64[(buf >> 6) & 0x3f] : '=';
		*p++ = '=';
	}

	return p - dst;
}

static unsigned
b64_decode(const char *src, unsigned len, char *dst)
{
	const char *srcend = src + len,
			   *s = src;
	char	   *p = dst;
	char		c;
	int			b = 0;
	uint32		buf = 0;
	int			pos = 0,
				end = 0;

	while (s < srcend)
	{
		c = *s++;

		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			continue;

		if (c == '=')
		{
			/* end sequence */
			if (!end)
			{
				if (pos == 2)
					end = 1;
				else if (pos == 3)
					end = 2;
				else
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unexpected \"=\"")));
			}
			b = 0;
		}
		else
		{
			b = -1;
			if (c > 0 && c < 127)
				b = b64lookup[(unsigned char) c];
			if (b < 0)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid symbol")));
		}
		/* add it to buffer */
		buf = (buf << 6) + b;
		pos++;
		if (pos == 4)
		{
			*p++ = (buf >> 16) & 255;
			if (end == 0 || end > 1)
				*p++ = (buf >> 8) & 255;
			if (end == 0 || end > 2)
				*p++ = buf & 255;
			buf = 0;
			pos = 0;
		}
	}

	if (pos != 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid end sequence")));

	return p - dst;
}

/* END SECTION OF CODE COPIED FROM pgcrypto.c */

/*
 * Low-level bit operation functions follow
 **********************************************************************
 */



/** 
 * Predicate identifying whether the bitmap is empty.
 * 
 * @param bitmap The ::Bitmap being scanned.
 * 
 * @return True if the bit at bitmin is zero.
 */

static boolean
bitmapEmpty(Bitmap *bitmap)
{
	return ((bitmap->bitset[0]) == 0);
}

/** 
 * Clear all bits in a ::Bitmap.
 * 
 * @param bitmap The ::Bitmap in which all bits are to be cleared
 */
static void
clearBitmap(Bitmap *bitmap)
{
	int32 elems = ARRAYELEMS(bitmap->bitmin, bitmap->bitmax);
	int32 i;
	
	for (i = 0; i < elems; i++) {
		bitmap->bitset[i] = 0;
	}
}


/** 
 * Return a new, possibly uninitialised, ::Bitmap.
 * 
 * @param min  The lowest value bit to be stored in the bitmap
 * @param max  The highest value bit to be stored in the bitmap
 *
 * @return New appropriately sized bitmap. 
 */
static Bitmap *
newBitmap(int32 min, int32 max)
{
	int32 elems = ARRAYELEMS(min, max);
	int32 size = sizeof(Bitmap) + (sizeof(bm_int) * elems);
	Bitmap *bitmap = bitmap = palloc(size);

	SET_VARSIZE(bitmap, size);
	
	bitmap->bitmin = min;
	bitmap->bitmax = max;

	return bitmap;
}


/** 
 * Test a bit within a ::Bitmap.  If the bit is outside of the acceptable
 * range return false.
 * 
 * @param bitmap The ::Bitmap within which the bit is to be set. 
 * @param bit The bit to be tested.
 * 
 * @return True if the bit is set, false otherwise.
 */
static bool
bitmapTestbit(Bitmap *bitmap,
			   int32 bit)
{
	if ((bit > bitmap->bitmax) ||
		(bit < bitmap->bitmin)) 
	{
		return false;
	}
	else {
		int32 relative_bit = bit - BITZERO(bitmap->bitmin);
		int32 element = BITSET_ELEM(relative_bit);
		
		return (bitmap->bitset[element] &
				bitmasks[BITSET_BIT(relative_bit)]) != 0;
	}
}

/** 
 * Create a copy of a bitmap.
 * 
 * @param bitmap  The original bitmap
 *
 * @return Copy of bitmap.
 */
static Bitmap *
copyBitmap(Bitmap *bitmap)
{
	Bitmap *result;
	int32 i;

	result = newBitmap(bitmap->bitmin, bitmap->bitmax);
	
	for (i = ARRAYELEMS(bitmap->bitmin, bitmap->bitmax); i >= 0; i--) {
		result->bitset[i] = bitmap->bitset[i];
	}
	return result;
}


/** 
 * Take an existing bitmap, and return a larger copy that will be able
 * to store a specified bit.
 * 
 * @param bitmap  The original bitmap
 * @param bit     A new bit that the returned bitmap must be capable of
 *                storing. 
 *
 * @return New bitmap with appropriate bitmin and bitmax values.
 */
static Bitmap *
extendBitmap(Bitmap *bitmap,
			  int32 bit)
{
	Bitmap *result;
	int32 orig_elems = ARRAYELEMS(bitmap->bitmin, bitmap->bitmax);
	int32 new_elems;
	int32 elems_offset;
	int32 extra_elems;
	int32 to_clear;
	int32 i;
	//elog(NOTICE, "Adding %d to %d->%d", bit, bitmap->bitmin, bitmap->bitmax);

	// Allocate new bitmap (no need to clear it)
	result = newBitmap(MIN(bit, bitmap->bitmin),
					   MAX(bit, bitmap->bitmax));
	//elog(NOTICE, "New: %d->%d", result->bitmin, result->bitmax);

	// Copy existing bitset into our new bitset.
	elems_offset = BITSET_ELEM(bitmap->bitmin) - BITSET_ELEM(result->bitmin);
	//elog(NOTICE, "orig_elems %d, offset %d", orig_elems, elems_offset);
	for (i = 0; i < orig_elems; i++) {
		//elog(NOTICE, "Copying %d into %d", i, i + elems_offset);
		result->bitset[i + elems_offset] = bitmap->bitset[i];
	}

	// Clear any preceding elements
	for (i = 0; i < elems_offset; i++) {
		//elog(NOTICE, "Clearing %d", i);
		result->bitset[i] = 0;
	}
	
	// Clear any following elements
	new_elems = ARRAYELEMS(result->bitmin, result->bitmax);
	extra_elems = new_elems - orig_elems;
	to_clear = extra_elems - elems_offset;
	//elog(NOTICE, "new_elems %d, extra_elems %d, to_clear %d",
	//	 new_elems, extra_elems, to_clear);
	for (i = new_elems - to_clear; i < new_elems; i++) {
		//elog(NOTICE, "Clearing %d", i);
		result->bitset[i] = 0;
	}
	return result;
}


/** 
 * Set a bit within a ::Bitmap.
 * 
 * @param bitmap The ::Bitmap within which the bit is to be set. 
 * @param bit The bit to be set.
 *
 * @return Possibly new bitmap with the specified bit set.
 */
static void
doSetBit(Bitmap *bitmap,
		 int32 bit)
{
    int32 relative_bit;
    int32 element;
	
    relative_bit = bit - BITZERO(bitmap->bitmin);
    element = BITSET_ELEM(relative_bit);
    bitmap->bitset[element] |= bitmasks[BITSET_BIT(relative_bit)];

	if (bit < bitmap->bitmin) {
		bitmap->bitmin = bit;
	}
	else if (bit > bitmap->bitmax) {
		bitmap->bitmax = bit;
	}
}

/** 
 * Return a new ::Bitmap with bit set.  
 * 
 * @param bitmap The ::Bitmap within which the bit is to be set. 
 * @param bit The bit to be set.
 *
 * @return New bitmap with the specified bit set.
 */
static Bitmap *
bitmapSetbit(Bitmap *bitmap,
			 int32 bit)
{
	Bitmap *result;

	if ((bit > bitmap->bitmax) || (bit < bitmap->bitmin)) {
		result = extendBitmap(bitmap, bit);
	}
	else {
		result = copyBitmap(bitmap);
	}
	doSetBit(result, bit);
	return result;
}


/** 
 * Return the next set bit in the ::Bitmap.
 * 
 * @param bitmap The ::Bitmap being scanned.
 * @param inbit The starting bit from which to scan the bitmap
 * @param found Boolean that will be set to true when a set bit has been
 * found.
 * 
 * @return The bit id of the found bit, or the inbit parameter if no set
 *         bits were found.  
 */
static int32
bitmapNextBit(Bitmap *bitmap,
			  int32 inbit,
			  bool *found)
{
	int32 bit = inbit;
	while (bit <= bitmap->bitmax) {
		if (bitmapTestbit(bitmap, bit)) {
			*found = true;
			return bit;
		}
		bit++;
	}
	*found = false;
	return inbit;
}

static char *
serialise_bitmap(Bitmap *bitmap);

/** 
 * Take an existing ::Bitmap, and shrink it to be bounded by the elements
 * containing the highest and lowest bits.
 * 
 * @param bitmap  The original ::Bitmap
 *
 * @return Bitmap with updated bitmin and bitmax values.
 */
static void
reduceBitmap(Bitmap *bitmap)
{
	int32 bit = bitmap->bitmin;
	bool found;
	int32 first_elem;
	int32 last_elem = BITSET_ELEM(bitmap->bitmax) - BITSET_ELEM(bitmap->bitmin);
	int32 i;

	bit = bitmapNextBit(bitmap, bit, &found);
	if (!found) {
		/* Bitmap is empty: do a quick exit. */
		bitmap->bitmax = bitmap->bitmin;
		return;
	}
	first_elem = BITSET_ELEM(bit) - BITSET_ELEM(bitmap->bitmin);
	if (first_elem) {
		/* The first elements have no bits, so we need to re-base
	     * the bitset array. */
		for (i = first_elem; i <= last_elem; i++) {
			bitmap->bitset[i - first_elem] = bitmap->bitset[i];
		}
		last_elem -= first_elem;
	}
	bitmap->bitmin = bit;

	/* Now find last bit (we know that found is true at this point). */
	while (found) {
		bit += 1;
		bit = bitmapNextBit(bitmap, bit, &found);
	}
	bitmap->bitmax = bit - 1; /* -1 undoes the last increment in above loop. */
}

/** 
 * Clear a bit within a ::Bitmap.
 * 
 * @param bitmap The ::Bitmap within which the bit is to be cleared. 
 * @param bit The bit to be set.
 *
 * @return Possibly new bitmap with the specified bit set.
 */
static void
doClearBit(Bitmap *bitmap,
		   int32 bit)
{
    int32 relative_bit = bit - BITZERO(bitmap->bitmin);
    int32 element = BITSET_ELEM(relative_bit);
	if ((bit <= bitmap->bitmax) && (bit >= bitmap->bitmin)) 
	{
		bitmap->bitset[element] &= ~(bitmasks[BITSET_BIT(relative_bit)]);
	}
}

/** 
 * Clear a bit within a ::Bitmap. 
 * 
 * @param bitmap The ::Bitmap within which the bit is to be cleared. 
 * @param bit The bit to be cleared.
 *
 * @return The original bitmap, possibly shrunk, with the specified bit
 * cleared.
 */
static Bitmap *
bitmapClearbit(Bitmap *bitmap,
			   int32 bit)
{
	doClearBit(bitmap, bit);
	if ((bit == bitmap->bitmin) || (bit == bitmap->bitmax)) {
		reduceBitmap(bitmap);
	}

	return bitmap;
}


/** 
 * Ensure bitmin of bitmap is no less than parameter.  This provides the
 * means to quickly truncate a bitmap.
 * 
 * @param bitmap The ::Bitmap to be modified
 * @param bitmin The new minimum value
 * 
 * @return A newly allocated bitmap which has no lower bits than bitmin
 */
static Bitmap *
bitmapSetMin(Bitmap *bitmap, int bitmin)
{
	Bitmap *result = newBitmap(MAX(bitmap->bitmin, bitmin),
							   bitmap->bitmax);
	int32 bitmap_elems = ARRAYELEMS(bitmap->bitmin, bitmap->bitmax);
	int32 res_elems = ARRAYELEMS(result->bitmin, result->bitmax);
	int32 lost_elems = bitmap_elems - res_elems;
	int32 i;
	
	/* Copy the bitset elements that we need. */
	for (i = 0; i < res_elems; i++) {
		result->bitset[i] = bitmap->bitset[i + lost_elems];
	}

	if (result->bitmin != bitmap->bitmin) {
		/* Clear any bits below bitmin */
		if (result->bitmin > BITZERO(result->bitmin)) {
			for (i = BITSET_ELEM(result->bitmin - 1); i >= 0; i--) {
				result->bitset[0] &= ~(bitmasks[BITSET_BIT(i)]);
			}
		}
		/* Update bitmin to match the lowest bit that is actually set. */
		/* If the bitmap is sparse, there may be entire words that are
		 * empty of bits.  Let's deal with that situation first. */
		lost_elems = 0;
		for (i = 0; i < res_elems; i++) {
			if (result->bitset[i] == 0) {
				lost_elems++;
			}
			else {
				break;
			}
		}
		if (lost_elems) {
			for (i = 0; i < (res_elems - lost_elems); i++) {
				result->bitset[i] = result->bitset[i + lost_elems];
			}
			result->bitmin = BITZERO(result->bitmin + (lost_elems * ELEMBITS));
		}
		
		for (i = BITSET_ELEM(result->bitmin); i < ELEMBITS; i++) {
			if (result->bitset[0] & bitmasks[BITSET_BIT(i)]) {
				result->bitmin = BITZERO(result->bitmin) + i;
				break;
			}
		}
	}

	return result;
}

/** 
 * Ensure bitmax of bitmap is no more than parameter
 * 
 * @param bitmap The ::Bitmap to be modified
 * @param bitmax The new minimum value
 * 
 * @return A newly allocated bitmap which has no higher bits than bitmax
 */
static Bitmap *
bitmapSetMax(Bitmap *bitmap, int bitmax)
{
	Bitmap *result = newBitmap(bitmap->bitmin,
							   MIN(bitmap->bitmax, bitmax));
	int32 res_elems = ARRAYELEMS(result->bitmin, result->bitmax);
	int32 last_elem;
	int32 lost_elems;
	int32 i;

	/* Copy the bitset elements that we need. */
	for (i = 0; i < res_elems; i++) {
		result->bitset[i] = bitmap->bitset[i];
	}

	if (result->bitmax != bitmap->bitmax) {
		last_elem = res_elems - 1;
		/* Clear any bits above bitmax */
		for (i = BITSET_BIT(result->bitmax) + 1; i < ELEMBITS; i++) {
			result->bitset[last_elem] &= ~(bitmasks[BITSET_BIT(i)]);
		}
		
		/* Update bitmax to match the highest bit that is actually set. */
		lost_elems = 0;
		for (i = last_elem; i > 0; i--) {
			if (result->bitset[i] == 0) {
				// We found an empty word
				lost_elems++;
			}
			else {
				break;
			}
		}
		if (lost_elems) {
			result->bitmax = BITZERO(result->bitmax) -
				(lost_elems * ELEMBITS) + ELEMBITS - 1;
		}
		last_elem -= lost_elems;

		for (i = ELEMBITS - 1; i > 0; i--) {
			if (result->bitset[last_elem] & bitmasks[BITSET_BIT(i)]) {
				result->bitmax = BITZERO(result->bitmax) + i;
				break;
			}
		}
	}

	return result;
}


/** 
 * Test for equality of 2 bitmaps
 * 
 * @param bitmap1 The first ::Bitmap to be compared
 * @param bitmap2 The second ::Bitmap to be compared
 *
 * @return True if the bitmaps are the same.
 */
static bool
bitmapEqual(Bitmap *bitmap1,
			Bitmap *bitmap2)
{
	int32 elems;
	int32 i;
	if ((bitmap1->bitmin == bitmap2->bitmin) &&
		(bitmap1->bitmax == bitmap2->bitmax)) {
		elems = ARRAYELEMS(bitmap1->bitmin, bitmap1->bitmax);
		for (i = 0; i < elems; i++) {
			if (bitmap1->bitset[i] != bitmap2->bitset[i]) {
				return false;
			}
		}
		return true;
	}
	else {
		/* bitmin and bitmax do not agree but they could both be empty
	     * bitmaps, so check for that.
		 */
		return bitmapEmpty(bitmap1) && bitmapEmpty(bitmap2);
	}
}


/** 
 * Create the union of two bitmaps.
 * 
 * @param bitmap1 The first ::Bitmap to be unioned.
 * @param bitmap2 The second ::Bitmap, to be unioned with bitmap1.
 *
 * @return A newly allocated bitmap which is the union
 */
static Bitmap *
bitmapUnion(Bitmap *bitmap1,
			Bitmap *bitmap2)
{
	Bitmap *result = newBitmap(MIN(bitmap1->bitmin, bitmap2->bitmin),
							   MAX(bitmap1->bitmax, bitmap2->bitmax));
	int32 res_elems = ARRAYELEMS(result->bitmin, result->bitmax);
	int32 bit_offset1 = BITZERO(result->bitmin) - BITZERO(bitmap1->bitmin);
	int32 bit_offset2 = BITZERO(result->bitmin) - BITZERO(bitmap2->bitmin);
	int32 elem_offset1 = BITSET_ELEM(bit_offset1);
	int32 elem_offset2 = BITSET_ELEM(bit_offset2);
	int32 elem_max1 = BITSET_ELEM((bitmap1->bitmax - 
								   BITZERO(bitmap1->bitmin)));
	int32 elem_max2 = BITSET_ELEM((bitmap2->bitmax - 
								   BITZERO(bitmap2->bitmin)));
	bm_int elem;
	int32 to;
	int32 from;
	
	for (to = 0; to < res_elems; to++) {
		elem = 0;
		from = to + elem_offset1;
		if ((from >= 0) && (from <= elem_max1)) {
			elem = bitmap1->bitset[from];
		}
		from = to + elem_offset2;
		if ((from >= 0) && (from <= elem_max2)) {
			elem |= bitmap2->bitset[from];
		}
		result->bitset[to] = elem;
	}

	return result;
}


/** 
 * Create the intersection of two bitmaps.
 * 
 * @param bitmap1 The first ::Bitmap to be intersected.
 * @param bitmap2 The second ::Bitmap, to be intersected with bitmap1.
 *
 * @return A newly allocated bitmap which is the intersection
 */
static Bitmap *
bitmapIntersect(Bitmap *bitmap1,
				Bitmap *bitmap2)
{
	Bitmap *result = newBitmap(MAX(bitmap1->bitmin, bitmap2->bitmin),
							   MIN(bitmap1->bitmax, bitmap2->bitmax));
	int32 res_elems = ARRAYELEMS(result->bitmin, result->bitmax);
	int32 bit_offset1 = BITZERO(result->bitmin) - BITZERO(bitmap1->bitmin);
	int32 bit_offset2 = BITZERO(result->bitmin) - BITZERO(bitmap2->bitmin);
	int32 elem_offset1 = BITSET_ELEM(bit_offset1);
	int32 elem_offset2 = BITSET_ELEM(bit_offset2);
	int32 elem_max1 = BITSET_ELEM((bitmap1->bitmax - 
								   BITZERO(bitmap1->bitmin)));
	int32 elem_max2 = BITSET_ELEM((bitmap2->bitmax - 
								   BITZERO(bitmap2->bitmin)));
	bm_int elem;
	int32 to;
	int32 from;

	for (to = 0; to < res_elems; to++) {
		elem = 0;
		from = to + elem_offset1;
		if ((from >= 0) && (from <= elem_max1)) {
			elem = bitmap1->bitset[from];
	
			from = to + elem_offset2;
			if ((from >= 0) && (from <= elem_max2)) {
				elem &= bitmap2->bitset[from];
			}
			else {
				elem = 0;
			}
		}
		result->bitset[to] = elem;
	}
	reduceBitmap(result);
	return result;
}


/** 
 * Create the subtraction of two bitmaps.
 * 
 * @param bitmap1 The ::Bitmap from which bitmap2 will be subtracted
 * @param bitmap2 The ::Bitmap to be subtracted from from bitmap1.
 *
 * @return A newly allocated bitmap which is the subtraction
 */
static Bitmap *
bitmapMinus(Bitmap *bitmap1,
			Bitmap *bitmap2)
{
	Bitmap *result = copyBitmap(bitmap1);
	bool found = !bitmapEmpty(bitmap2);
	int32 bit = MAX(bitmap1->bitmin, bitmap2->bitmin);
	
	/* Now clear any bits that match from bitmap2 */
	while (found && (bit <= bitmap1->bitmax)) {
		doClearBit(result, bit);
		bit++;
		bit = bitmapNextBit(bitmap2, bit, &found);
	}
	reduceBitmap(result);
	return result;
}


/*
 * Serialisation functions follow
 **********************************************************************
 */


/** 
 * Serialise an int32 value as a base64 stream (truncated to save a
 * byte) into *p_stream.
 *
 * @param p_stream Pointer into stream currently being written.  This
 * must be large enought to take the contents to be written.  This
 * pointer is updated to point to the next free slot in the stream after
 * writing the int32 value, and is null terminated at that position.
 * @param value The value to be written to the stream.
 */
static void
serialise_int32(char **p_stream, int32 value)
{
	int32 len = b64_encode((char *) &value, sizeof(int32), *p_stream);
	(*p_stream) += (len - 1);  /* X: dumb optimisation saves a byte */
	(**p_stream) = '\0';
}


/** 
 * De-serialise an int32 value from a base64 character stream.
 *
 * @param p_stream Pointer into the stream currently being read.
 * must be large enought to take the contents to be written.  This
 * pointer is updated to point to the next free slot in the stream after
 * reading the int32 value.
 * @return the int32 value read from the stream
 */
static int32
deserialise_int32(char **p_stream)
{
	int32 value;
	char *endpos = (*p_stream) + INT32SIZE_B64;
	char endchar = *endpos;
	*endpos = '=';	/* deal with dumb optimisation (X) above */
	b64_decode(*p_stream, INT32SIZE_B64 + 1, (char *) &value);
	*endpos = endchar;
	(*p_stream) += INT32SIZE_B64;
	return value;
}


/** 
 * Serialise a binary stream as a base64 stream into *p_stream.
 *
 * @param p_stream Pointer into stream currently being written.  This
 * pointer is updated to point to the next free slot in the stream after
 * writing the contents of instream and is null terminated at that
 * position.
 *
 * @param bytes The number of bytes to be written.
 * @param instream The binary stream to be written.
 */
static void
serialise_stream(char **p_stream, int32 bytes, char *instream)
{
	int32 len = b64_encode(instream, bytes, *p_stream);
	(*p_stream)[len] = '\0';
	(*p_stream) += len;
}


/** 
 * Return the length of a base64 encoded stream for a binary stream of
 * ::bytes length.
 * 
 * @param bytes The length of the input binary stream in bytes
 * @return The length of the base64 character stream required to
 * represent the input stream.
 */
static int
streamlen(int32 bytes)
{
	return (4 * ((bytes + 2) / 3));
}


/** 
 * De-serialise a binary stream.
 *
 * @param p_stream Pointer into the stream currently being read.
 * pointer is updated to point to the next free slot in the stream after
 * reading the stream.
 * @param bytes The number of bytes to be read
 * @param outstream Pointer into the pre-allocated memory are into which
 * the binary from p_stream is to be written.
 */
static void
deserialise_stream(char **p_stream, int32 bytes, char *outstream)
{
	int32 len = streamlen(bytes);
	b64_decode(*p_stream, len, outstream);
	(*p_stream) += len;
}


/** 
 * Serialise a bitmap.
 *
 * @param bitmap The bitmap to be serialised.
 */
static char *
serialise_bitmap(Bitmap *bitmap)
{
    int32 elems = ARRAYELEMS(bitmap->bitmin, bitmap->bitmax);
    int32 stream_len = (INT32SIZE_B64 * 2) + 
		streamlen(sizeof(bm_int) * elems) + 1;
	char *stream = palloc(stream_len * sizeof(char));
	char *streamstart = stream;

	serialise_int32(&stream, bitmap->bitmin);
	serialise_int32(&stream, bitmap->bitmax);
	serialise_stream(&stream, elems * sizeof(bm_int), 
					 (char *) &(bitmap->bitset));
	*stream = '\0';

	/* Ensure bitmap is valid. */
	if (bitmap->bitmin != bitmap->bitmax) {
		if (! (bitmapTestbit(bitmap, bitmap->bitmin) &&
			   bitmapTestbit(bitmap, bitmap->bitmax)))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Corrupted bitmap"),
					 errdetail("one of bitmin or bitmax is not set.")));
		}
	}
	return streamstart;
}


/** 
 * De-serialise a bitmap.
 *
 * @param charstream The serialised character string containing the bitmap.
 */
static Bitmap *
deserialise_bitmap(char *charstream)
{
	Bitmap *bitmap;
	char **p_stream = &charstream;
	int32 elems;
    int32 bitmin;
	int32 bitmax;

	if (strcmp(charstream, "[]") == 0) {
		bitmap = newBitmap(0, 0);
		clearBitmap(bitmap);
	}
	else {
		bitmin = deserialise_int32(p_stream);
		bitmax = deserialise_int32(p_stream);
		bitmap = newBitmap(bitmin, bitmax);
		elems = ARRAYELEMS(bitmin, bitmax);

		deserialise_stream(p_stream, elems * sizeof(bitmap->bitset[0]), 
						   (char *) &(bitmap->bitset[0]));
	}
	return bitmap;
}


/** 
 * Compare 2 bitmaps for indexing/sorting purposes
 * 
 * @param bitmap1 The first ::Bitmap to be compared
 * @param bitmap2 The second ::Bitmap to be compared
 *
 * @return < 0, 0, or > 0 like strcmp
 */
static int
bitmapCmp(Bitmap *bitmap1,
		  Bitmap *bitmap2)
{
	if (bitmapEqual(bitmap1, bitmap2)) {
		return 0;
	}
	else {
		char *s1 = serialise_bitmap(bitmap1);
		char *s2 = serialise_bitmap(bitmap2);
		int result = strcmp(s1, s2);
		pfree(s1);
		pfree(s2);
		return result;
	}
}


/*
 * Interface functions follow
 **********************************************************************
 */


PG_FUNCTION_INFO_V1(bitmap_in);
/** 
 * <code>bitmap_in(serialised_bitmap text) returns bitmap</code>
 * Create a Bitmap initialised from a (base64) text value.
 *
 * @param fcinfo Params as described_below
 * <br><code>serialised_bitmap text</code> A base64 serialiastion of a
 * bitmap value.
 * @return <code>Bitmap</code> the newly created bitmap
 */
Datum
bitmap_in(PG_FUNCTION_ARGS)
{
	char    *stream;
    Bitmap *bitmap;

    stream = PG_GETARG_CSTRING(0);
	bitmap = deserialise_bitmap(stream);

    PG_RETURN_BITMAP(bitmap);
}


PG_FUNCTION_INFO_V1(bitmap_out);
/** 
 * <code>bitmap_out(bitmap bitmap) returns text</code>
 * Create a (base64) text representation of a bitmap.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> A base64 serialiastion of a
 * bitmap value.
 * @return <code>cstring</code> the newly serialised text stream.
 */
Datum
bitmap_out(PG_FUNCTION_ARGS)
{
	char    *result;
    Bitmap *bitmap;

    bitmap = PG_GETARG_BITMAP(0);
	result = serialise_bitmap(bitmap);

	PG_RETURN_CSTRING(result);
}


PG_FUNCTION_INFO_V1(bitmap_is_empty);
/** 
 * <code>bitmap_is_empty(bitmap bitmap) returns boolean</code>
 * Predicate to identify whether a bitmap is empty or not.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap being examined.
 * bitmap value.
 * @return <code>boolean</code> true, if the bitmap has no bits.
 */
Datum
bitmap_is_empty(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;

    bitmap = PG_GETARG_BITMAP(0);
	PG_RETURN_BOOL(bitmapEmpty(bitmap));
}


PG_FUNCTION_INFO_V1(bitmap_bitmin);
/** 
 * <code>bitmap_bitmin(bitmap bitmap) returns boolean</code>
 * Return the lowest bit set in the bitmap, or NULL if none are set.
 * This relies on bitmap->bitmin always identifying the lowest numbered
 * bit in the bitset, unless the bitmap is empty.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap being examined.
 * @return <code>integer</code> The lowest bit set in the bitmap or
 * NULL if there are no bits set.
 */
Datum
bitmap_bitmin(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap = PG_GETARG_BITMAP(0);
    int32 relative_bit = bitmap->bitmin - BITZERO(bitmap->bitmin);

	if (bitmap->bitset[0] & bitmasks[relative_bit]) {
		PG_RETURN_INT32(bitmap->bitmin);
	}
	if (bitmap->bitmin != bitmap->bitmax) {
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Corrupted bitmap"),
				 errdetail("bitzeo is incorrect for bitmap (range %d..%d).", 
						   bitmap->bitmin, bitmap->bitmax)));
	}
	PG_RETURN_NULL();
}


PG_FUNCTION_INFO_V1(bitmap_bitmax);
/** 
 * <code>bitmap_bitmax(bitmap bitmap) returns boolean</code>
 * Return the highest bit set in the bitmap, or NULL if none are set.
 * This relies on bitmap->bitmax always identifying the highest numbered
 * bit in the bitset, unless the bitmap is empty.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap being examined.
 * @return <code>integer</code> The lowest bit set in the bitmap or
 * NULL if there are no bits set.
 */
Datum
bitmap_bitmax(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap = PG_GETARG_BITMAP(0);
    int32 relative_bit = bitmap->bitmax - BITZERO(bitmap->bitmin);
	int32 elem = BITSET_ELEM(relative_bit);

	if (bitmap->bitset[elem] & bitmasks[BITSET_BIT(relative_bit)]) {
		PG_RETURN_INT32(bitmap->bitmax);
	}
	if (bitmap->bitmin != bitmap->bitmax) {
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Corrupted bitmap"),
				 errdetail("bitmax is incorrect for bitmap (range %d..%d).", 
						   bitmap->bitmin, bitmap->bitmax)));
	}
	PG_RETURN_NULL();
}


PG_FUNCTION_INFO_V1(bitmap_bits);
/** 
 * <code>bitmap_bits(name text)</code> returns setof int4
 * Return the set of all bits set in the specified Bitmap.
 *
 * @param fcinfo <code>name text</code> The name of the bitmap.
 * @return <code>setof int4</code>The set of bits that are set in the
 * bitmap.
 */
Datum
bitmap_bits(PG_FUNCTION_ARGS)
{
	struct bitmap_bits_state {
		Bitmap *bitmap;
		int32   bit;
	} *state;
    FuncCallContext *funcctx;
	MemoryContext    oldcontext;
    bool   found;
    Datum  datum;
    
    if (SRF_IS_FIRSTCALL())
    {
        funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		state = palloc(sizeof(struct bitmap_bits_state));
        MemoryContextSwitchTo(oldcontext);

        state->bitmap = PG_GETARG_BITMAP(0);
        state->bit = state->bitmap->bitmin;
		funcctx->user_fctx = state;
    }
    
    funcctx = SRF_PERCALL_SETUP();
	state = funcctx->user_fctx;
    
    state->bit = bitmapNextBit(state->bitmap, state->bit, &found);
    
    if (found) {
        datum = Int32GetDatum(state->bit);
        state->bit++;
        SRF_RETURN_NEXT(funcctx, datum);
    }
    else {
        SRF_RETURN_DONE(funcctx);
    }
}


PG_FUNCTION_INFO_V1(bitmap_new_empty);
/** 
 * <code>bitmap_new_empty() returns bitmap;</code>
 * Create an empty Bitmap, for dealing with a named range of
 * values.
 *
 * @param fcinfo Params as described_below
 * @return <code>Bitmap</code> the newly allocated bitmap
 */
Datum
bitmap_new_empty(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	bitmap = newBitmap(0, 0);
	clearBitmap(bitmap);
	
    PG_RETURN_BITMAP(bitmap);
}


PG_FUNCTION_INFO_V1(bitmap_new);
/** 
 * <code>bitmap_new() returns bitmap;</code>
 * Create or re-initialise a Bitmap, for dealing with a named range of
 * values.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitno int32</code> The first integer value to be recorded in
 * the bitmap
 * @return <code>Bitmap</code> the newly allocated bitmap
 */
Datum
bitmap_new(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32   bit;

    bit = PG_GETARG_INT32(0);
	bitmap = newBitmap(bit, bit);
	clearBitmap(bitmap);
	doSetBit(bitmap, bit);

    PG_RETURN_BITMAP(bitmap);
}


PG_FUNCTION_INFO_V1(bitmap_setbit);
/** 
 * <code>bitmap_setbit(bitmap bitmap, bit int4) returns bool</code>
 * Set the given bit in the bitmap, returning TRUE.  This can be used as
 * an aggregate function in which case the bitmap parameter will be null
 * for the first call.  In this case simply create a bitmap from the
 * second argument.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap to be manipulated.
 * <br><code>bitno int4</code> The bitnumber to be set.
 * @return <code>bitmap</code> The result.
 */
Datum
bitmap_setbit(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32   bitno;
	Bitmap *result;

    bitno = PG_GETARG_INT32(1);
	if (PG_ARGISNULL(0)) {
		result = newBitmap(bitno, bitno);
		clearBitmap(result);
		doSetBit(result, bitno);
	}
	else {
		bitmap = PG_GETARG_BITMAP(0);
		result = bitmapSetbit(bitmap, bitno);
	}

	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_testbit);
/** 
 * <code>bitmap_testbit(bitmap bitmap, bit int4) returns bool</code>
 * Test the given bit in the bitmap, returning TRUE if it is 1.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap to be manipulated.
 * <br><code>bitno int4</code> The bitnumber to be tested.
 * @return <code>bool</code> the truth value of the bit.
 */
Datum
bitmap_testbit(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32   bitno;
	bool    result;

    bitmap = PG_GETARG_BITMAP(0);
    bitno = PG_GETARG_INT32(1);
	result = bitmapTestbit(bitmap, bitno);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_setmin);
/** 
 * <code>bitmap_setmin(bitmap bitmap, bitmin int4) returns bitmap</code>
 * Return a new bitmap having no bits less than bitmin
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap to be manipulated.
 * <br><code>bitmin int4</code> The new minimum bit.
 * @return <code>bitmap</code> The new bitmap.
 */
Datum
bitmap_setmin(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32   bitmin;
	Bitmap *result;
	
    bitmap = PG_GETARG_BITMAP(0);
    bitmin = PG_GETARG_INT32(1);
	result = copyBitmap(bitmap);
	result = bitmapSetMin(result, bitmin);

	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_setmax);
/** 
 * <code>bitmap_setmax(bitmap bitmap, bitmax int4) returns bitmap</code>
 * Return a new bitmap having no bits greater than bitmax
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap to be manipulated.
 * <br><code>bitmax int4</code> The new maximum bit.
 * @return <code>bitmap</code> The new bitmap.
 */
Datum
bitmap_setmax(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32   bitmax;
	Bitmap *result;
	
    bitmap = PG_GETARG_BITMAP(0);
    bitmax = PG_GETARG_INT32(1);
	result = copyBitmap(bitmap);
	result = bitmapSetMax(result, bitmax);

	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_equal);
/** 
 * <code>bitmap_equal(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if the bitmaps are equivalent,
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true if the bitmaps contain the same bits.
 */
Datum
bitmap_equal(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapEqual(bitmap1, bitmap2);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_nequal);
/** 
 * <code>bitmap_nequal(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if the bitmaps are not equivalent,
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true unless the bitmaps contain the same bits.
 */
Datum
bitmap_nequal(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = !bitmapEqual(bitmap1, bitmap2);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_cmp);
/** 
 * <code>bitmap_cmp(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return result of comparison of bitmap1's string representation with
 *        bitmap2's.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>int4</code> result of comparison
 */
Datum
bitmap_cmp(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    int32   result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapCmp(bitmap1, bitmap2);

	PG_RETURN_INT32(result);
}


PG_FUNCTION_INFO_V1(bitmap_lt);
/** 
 * <code>bitmap_lt(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if bitmap1's string representation should be sorted
 * 		       earlier than bitmap2's.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true unless the bitmaps contain the same bits.
 */
Datum
bitmap_lt(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapCmp(bitmap1, bitmap2) < 0;

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_le);
/** 
 * <code>bitmap_le(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if bitmap1's string representation should be sorted
 * 		       earlier than, or the same as, bitmap2's.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true unless the bitmaps contain the same bits.
 */
Datum
bitmap_le(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapCmp(bitmap1, bitmap2) <= 0;

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_gt);
/** 
 * <code>bitmap_gt(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if bitmap1's string representation should be sorted
 * 		       later than bitmap2's.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true unless the bitmaps contain the same bits.
 */
Datum
bitmap_gt(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapCmp(bitmap1, bitmap2) > 0;

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_ge);
/** 
 * <code>bitmap_ge(bitmap1 bitmap, bitmap2 bitmap) returns bool</code>
 * Return true if bitmap1's string representation should be sorted
 * 		       later than, or the same as, bitmap2's.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bool</code> true unless the bitmaps contain the same bits.
 */
Datum
bitmap_ge(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    bool    result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapCmp(bitmap1, bitmap2) >= 0;

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_union);
/** 
 * <code>bitmap_union(bitmap1 bitmap, bitmap2 bitmap) returns bitmap</code>
 * Return the union of 2 bitmaps.  This can be used as an aggregate
 * function in which case the bitmap1 parameter will be null for the
 * first call.  In this case simply return the second argument.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bitmap</code> the union of the two bitmaps.
 */
Datum
bitmap_union(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    Bitmap *result;

	bitmap2 = PG_GETARG_BITMAP(1);
	if (PG_ARGISNULL(0)) {
		result = copyBitmap(bitmap2);
	}
	else {
		bitmap1 = PG_GETARG_BITMAP(0);
		result = bitmapUnion(bitmap1, bitmap2);
	}
	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_clearbit);
/** 
 * <code>bitmap_clearbit(bitmap bitmap, bit int4) returns bool</code>
 * Clear the given bit in the bitmap, returning FALSE.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap bitmap</code> The bitmap to be manipulated.
 * <br><code>bitno int4</code> The bitnumber to be set.
 * @return <code>bool</code> TRUE
 */
Datum
bitmap_clearbit(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap;
	int32    bitno;
	Bitmap *result;

    bitmap = PG_GETARG_BITMAP(0);
    bitno = PG_GETARG_INT32(1);
	result = copyBitmap(bitmap);
	result = bitmapClearbit(result, bitno);

	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_intersection);
/** 
 * <code>bitmap_intersection(bitmap1 bitmap, bitmap2 bitmap) 
 *     returns bitmap</code>
 * Return the intersection of 2 bitmaps.  This can be used as an
 * aggregate function in which case the bitmap1 parameter will be null
 * for the first call.  In this case simply return the second argument.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bitmap</code> the intersection of the two bitmaps.
 */
Datum
bitmap_intersection(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    Bitmap *result;

	bitmap2 = PG_GETARG_BITMAP(1);
	if (PG_ARGISNULL(0)) {
		result = copyBitmap(bitmap2);

	}
	else {
		bitmap1 = PG_GETARG_BITMAP(0);
		result = bitmapIntersect(bitmap1, bitmap2);
	}
	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_minus);
/** 
 * <code>bitmap_minus(bitmap1 bitmap, bitmap2 bitmap) 
 *     returns bitmap</code>
 * Return the bitmap1 with all bits from bitmap2 subtracted (cleared)
 *     from it.
 *
 * @param fcinfo Params as described_below
 * <br><code>bitmap1 bitmap</code> The first bitmap
 * <br><code>bitmap2 bitmap</code> The second bitmap
 * @return <code>bitmap</code> the subtraction of the two bitmaps.
 */
Datum
bitmap_minus(PG_FUNCTION_ARGS)
{
    Bitmap *bitmap1;
    Bitmap *bitmap2;
    Bitmap *result;

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapMinus(bitmap1, bitmap2);

	PG_RETURN_BITMAP(result);
}
