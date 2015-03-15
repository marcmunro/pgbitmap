/**
 * @file   bitmap.h
 * \code
 *     Author:       Marc Munro
 *     Copyright (c) 2015 Marc Munro
 *     License:      BSD
 * 
 * \endcode
 * @brief  
 * Define a bitmap type for postgres.  Note that this is derived from
 * veil in an attempt to deprecate veil now that PostgreSQL has built-in
 * features that render much of it redundant.
 * 
 */

#include "bitmap.h"

PG_MODULE_MAGIC;


#define INT32SIZE_B64          7   /* Actually 8 but the last char is
									* always '=' so we forget it. */
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
 * Clear all bits in a ::Bitmap.
 * 
 * @param bitmap The ::Bitmap in which all bits are to be cleared
 */
static void
clearBitmap(Bitmap *bitmap)
{
	int32 elems = ARRAYELEMS(bitmap->bitzero, bitmap->bitmax);
	int32 i;
	
	for (i = 0; i < elems; i++) {
		bitmap->bitset[i] = 0;
	}
}


/** 
 * Return a newly initialised (empty) ::Bitmap.
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
	
	bitmap->bitzero = min;
	bitmap->bitmax = max;
	clearBitmap(bitmap);

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
    int32 relative_bit = bit - BITZERO(bitmap->bitzero);
    int32 element = BITSET_ELEM(relative_bit);

	if ((bit > bitmap->bitmax) ||
		(bit < bitmap->bitzero)) 
	{
		return false;
	}

    return (bitmap->bitset[element] & bitmasks[BITSET_BIT(relative_bit)]) != 0;
}


/** 
 * Take an existing bitmap, and return a larger copy that will be able
 * to store a specified bit.
 * 
 * @param bitmap  The original bitmap
 * @param bit     A new bit that the returned bitmap must be capable of
 *                storing. 
 *
 * @return New bitmap with appropriate bitzero and bitmax values.
 */
static Bitmap *
extendBitmap(Bitmap *bitmap,
			 int32 bit)
{
	Bitmap *result = bitmap;
	int32 from;
	int32 to;
    int32 relative_frombit;

	if (BITZERO(bit) < BITZERO(bitmap->bitzero)) {
		result = newBitmap(bit, bitmap->bitmax);
		relative_frombit = bitmap->bitzero - BITZERO(result->bitzero);

		for (from = 0, to = BITSET_ELEM(relative_frombit);
			 to <= BITSET_ELEM(result->bitmax);
			 from++, to++) 
		{
			result->bitset[to] = bitmap->bitset[from];
		}
	}
	if (BITZERO(bit) > BITZERO(bitmap->bitmax)) {
		result = newBitmap(bitmap->bitzero, bit);
		for (to = 0; to < ARRAYELEMS(bitmap->bitzero, bitmap->bitmax); to++) {
			result->bitset[to] = bitmap->bitset[to];
		}
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
static Bitmap *
bitmapSetbit(Bitmap *bitmap,
			 int32 bit)
{
    int32 relative_bit;
    int32 element;
	Bitmap *result = bitmap;

	if ((result->bitzero == result->bitmax) &&
		!bitmapTestbit(result, result->bitzero))
	{
		result->bitzero = result->bitmax = bit;
	}

	if ((bit > bitmap->bitmax) ||
		(bit < bitmap->bitzero)) 
	{
		result = extendBitmap(bitmap, bit);
	}

    relative_bit = bit - BITZERO(result->bitzero);
    element = BITSET_ELEM(relative_bit);
    result->bitset[element] |= bitmasks[BITSET_BIT(relative_bit)];

	if (bit < result->bitzero) {
		result->bitzero = bit;
	}
	else if (bit > result->bitmax) {
		result->bitmax = bit;
	}
	return result;
}


/** 
 * Return the bit number of the lowest bit set in elem.
 * 
 * @param elem  A bitset element
 *
 * @return The bit number of the lowest bit, or -1 if no bits were set.
 */
static int32
lowBitno(bm_int elem)
{
	int32 i;
	for (i = 0; i < ELEMBITS; i++) {
		if (elem & bitmasks[i]) {
			return i;
		}
	}
	return -1;
}


/** 
 * Return the bit number of the highest bit set in elem.
 * 
 * @param elem  A bitset element
 *
 * @return The bit number of the highest bit, or -1 if no bits were set.
 */
static int32
highBitno(bm_int elem)
{
	int32 i;
	for (i = ELEMBITS - 1; i >= 0; i--) {
		if (elem & bitmasks[i]) {
			return i;
		}
	}
	return -1;
}

/** 
 * Take an existing bitmap, and shrink it to be bounded by the elements
 * containing the highest and lowest bits.
 * 
 * @param bitmap  The original bitmap
 *
 * @return Bitmap with updated bitzero and bitmax values.
 */
static void
reduceBitmap(Bitmap *bitmap)
{
	int32 elems = ARRAYELEMS(bitmap->bitzero, bitmap->bitmax);
	int32 first_elem = 0;
	int32 last_elem = elems - 1;
	int32 to;
	int32 i;

	for (i = 0; i < elems; i++) {
		if (bitmap->bitset[i]) {
			first_elem = i;
			break;
		}
	}
	for (i = last_elem; i >= first_elem; i--) {
		if (bitmap->bitset[i]) {
			last_elem = i;
			break;
		}
	}
	if (first_elem > 0) {
		to = 0;
		for (i = first_elem; i <= last_elem; i++, to++) {
			bitmap->bitset[to] = bitmap->bitset[i];
		}
	}
	bitmap->bitzero = BITZERO(bitmap->bitzero) + 
		lowBitno(bitmap->bitset[0]) +
		(ELEMBITS * first_elem);
	bitmap->bitmax =  BITZERO(bitmap->bitzero) + 
		highBitno(bitmap->bitset[last_elem]) +
		(ELEMBITS * last_elem);
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
    int32 relative_bit = bit - BITZERO(bitmap->bitzero);
    int32 element = BITSET_ELEM(relative_bit);

	if ((bit <= bitmap->bitmax) && (bit >= bitmap->bitzero)) 
	{
		bitmap->bitset[element] &= ~(bitmasks[BITSET_BIT(relative_bit)]);
		if ((bit == bitmap->bitzero) || (bit == bitmap->bitmax)) {
			reduceBitmap(bitmap);
		}
	}

	return bitmap;
}


/** 
 * Return the next set bit in the ::Bitmap.
 * 
 * @param bitmap The ::Bitmap being scanned.
 * @param bit The starting bit from which to scan the bitmap
 * @param found Boolean that will be set to true when a set bit has been
 * found.
 * 
 * @return The bit id of the found bit, or zero if no set bits were found. 
 */
static int32
bitmapNextBit(Bitmap *bitmap,
			  int32 bit,
			  bool *found)
{
	while (bit <= bitmap->bitmax) {
		if (bitmapTestbit(bitmap, bit)) {
			*found = true;
			return bit;
		}
		bit++;
	}
	*found = false;
	return 0;
}


/** 
 * Predicate identifying whether the bitmap is empty.
 * 
 * @param bitmap The ::Bitmap being scanned.
 * 
 * @return True if there are no bits in the bitmap.
 */

static boolean
bitmapEmpty(Bitmap *bitmap)
{
	int32 elems = ARRAYELEMS(bitmap->bitzero, bitmap->bitmax);
	int32 i;
	for (i = 0; i < elems; i++) {
		if (bitmap->bitset[i] != 0) {
			/* Normally bitzero will be set, so this should return on
			 * the first iteration unless the bitmap is empty, in which
			 * case there *should* only be one iteration of the loop.
			 */
			return FALSE;
		}
	}
	return TRUE;
}


/** 
 * Create the union of two bitmaps.
 * 
 * @param target The ::Bitmap into which the result will be placed.
 * @param source The ::Bitmap to be unioned into target.
 *
 * @return A newly allocated bitmap which is the union
 */
static Bitmap *
bitmapUnion(Bitmap *bitmap1,
			 Bitmap *bitmap2)
{
	Bitmap *result = newBitmap(MIN(bitmap1->bitzero, bitmap2->bitzero),
							   MAX(bitmap1->bitmax, bitmap2->bitmax));
	int32 res_elems = ARRAYELEMS(result->bitzero, result->bitmax);
	int32 bit_offset1 = BITZERO(result->bitzero) - BITZERO(bitmap1->bitzero);
	int32 bit_offset2 = BITZERO(result->bitzero) - BITZERO(bitmap2->bitzero);
	int32 elem_offset1 = BITSET_ELEM(bit_offset1);
	int32 elem_offset2 = BITSET_ELEM(bit_offset2);
	int32 elem_max1 = BITSET_ELEM((bitmap1->bitmax - 
								   BITZERO(bitmap1->bitzero)));
	int32 elem_max2 = BITSET_ELEM((bitmap2->bitmax - 
								   BITZERO(bitmap2->bitzero)));
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
 * @param target The ::Bitmap into which the result will be placed.
 * @param source The ::Bitmap to be unioned into target.
 *
 * @return A newly allocated bitmap which is the union
 */
static Bitmap *
bitmapIntersect(Bitmap *bitmap1,
				Bitmap *bitmap2)
{
	Bitmap *result = newBitmap(MAX(bitmap1->bitzero, bitmap2->bitzero),
							   MIN(bitmap1->bitmax, bitmap2->bitmax));
	int32 res_elems = ARRAYELEMS(result->bitzero, result->bitmax);
	int32 bit_offset1 = BITZERO(result->bitzero) - BITZERO(bitmap1->bitzero);
	int32 bit_offset2 = BITZERO(result->bitzero) - BITZERO(bitmap2->bitzero);
	int32 elem_offset1 = BITSET_ELEM(bit_offset1);
	int32 elem_offset2 = BITSET_ELEM(bit_offset2);
	int32 elem_max1 = BITSET_ELEM((bitmap1->bitmax - 
								   BITZERO(bitmap1->bitzero)));
	int32 elem_max2 = BITSET_ELEM((bitmap2->bitmax - 
								   BITZERO(bitmap2->bitzero)));
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
    int32 elems = ARRAYELEMS(bitmap->bitzero, bitmap->bitmax);
    int32 stream_len = (INT32SIZE_B64 * 2) + 
		streamlen(sizeof(bm_int) * elems) + 1;
	char *stream = palloc(stream_len * sizeof(char));
	char *streamstart = stream;

	serialise_int32(&stream, bitmap->bitzero);
	serialise_int32(&stream, bitmap->bitmax);
	serialise_stream(&stream, elems * sizeof(bm_int), 
					 (char *) &(bitmap->bitset));
	*stream = '\0';

	/* Ensure bitmap is valid. */
	if (bitmap->bitzero != bitmap->bitmax) {
		if (! (bitmapTestbit(bitmap, bitmap->bitzero) &&
			   bitmapTestbit(bitmap, bitmap->bitmax)))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("Corrupted bitmap"),
					 errdetail("one of bitzero or bitmax is not set.")));
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
    int32 bitzero;
	int32 bitmax;

	if (strcmp(charstream, "[]") == 0) {
		return newBitmap(0, 0);
	}
	bitzero = deserialise_int32(p_stream);
	bitmax = deserialise_int32(p_stream);
	bitmap = newBitmap(bitzero, bitmax);
	elems = ARRAYELEMS(bitzero, bitmax);

	deserialise_stream(p_stream, elems * sizeof(bitmap->bitset[0]), 
					   (char *) &(bitmap->bitset[0]));
	return bitmap;
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
 * This relies on bitmap->bitzero always identifying the lowest numbered
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
    int32 relative_bit = bitmap->bitzero - BITZERO(bitmap->bitzero);

	if (bitmap->bitset[0] & bitmasks[relative_bit]) {
		PG_RETURN_INT32(bitmap->bitzero);
	}
	if (bitmap->bitzero != bitmap->bitmax) {
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Corrupted bitmap"),
				 errdetail("bitzeo is incorrect for bitmap (range %d..%d).", 
						   bitmap->bitzero, bitmap->bitmax)));
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
    int32 relative_bit = bitmap->bitmax - BITZERO(bitmap->bitzero);
	int32 elem = BITSET_ELEM(relative_bit);

	if (bitmap->bitset[elem] & bitmasks[BITSET_BIT(relative_bit)]) {
		PG_RETURN_INT32(bitmap->bitmax);
	}
	if (bitmap->bitzero != bitmap->bitmax) {
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Corrupted bitmap"),
				 errdetail("bitzeo is incorrect for bitmap (range %d..%d).", 
						   bitmap->bitzero, bitmap->bitmax)));
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
        state->bit = state->bitmap->bitzero;
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
	bitmap = bitmapSetbit(bitmap, bit);

    PG_RETURN_BITMAP(bitmap);
}


PG_FUNCTION_INFO_V1(bitmap_setbit);
/** 
 * <code>bitmap_setbit(bitmap bitmap, bit int4) returns bool</code>
 * Set the given bit in the bitmap, returning TRUE.
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
	int32    bitno;
	Bitmap *result;

    bitmap = PG_GETARG_BITMAP(0);
    bitno = PG_GETARG_INT32(1);
	result = bitmapSetbit(bitmap, bitno);

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
	int32    bitno;
	bool     result;

    bitmap = PG_GETARG_BITMAP(0);
    bitno = PG_GETARG_INT32(1);
	result = bitmapTestbit(bitmap, bitno);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(bitmap_union);
/** 
 * <code>bitmap_union(bitmap1 bitmap, bitmap2 bitmap) returns bitmap</code>
 * Return the union of 2 bitmaps.
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

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapUnion(bitmap1, bitmap2);

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
	result = bitmapClearbit(bitmap, bitno);

	PG_RETURN_BITMAP(result);
}


PG_FUNCTION_INFO_V1(bitmap_intersection);
/** 
 * <code>bitmap_intersection(bitmap1 bitmap, bitmap2 bitmap) 
 *     returns bitmap</code>
 * Return the intersection of 2 bitmaps.
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

    bitmap1 = PG_GETARG_BITMAP(0);
    bitmap2 = PG_GETARG_BITMAP(1);
	result = bitmapIntersect(bitmap1, bitmap2);

	PG_RETURN_BITMAP(result);
}
