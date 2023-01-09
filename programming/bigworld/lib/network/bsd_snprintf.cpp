#include "pch.hpp"

/* BigWorld Note:
 * This format string parsing code was taken from Openssh v4.3p2.
 * The original file that was used is located in the source tree at
 * 'openbsd-compat/bsd-snprintf.c'. This copy has been modified to
 * remove unused code for the purposes of simple format string parsing.
 */

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 *
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats.
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 * tridge@samba.org, idra@samba.org, April 2001
 *    got rid of fcvt code (twas buggy and made testing harder)
 *    added C99 semantics
 *
 * date: 2002/12/19 19:56:31;  author: herb;  state: Exp;  lines: +2 -0
 * actually print args for %g and %e
 *
 * date: 2002/06/03 13:37:52;  author: jmcd;  state: Exp;  lines: +8 -0
 * Since includes.h isn't included here, VA_COPY has to be defined here.  I don't
 * see any include file that is guaranteed to be here, so I'm defining it
 * locally.  Fixes AIX and Solaris builds.
 *
 * date: 2002/06/03 03:07:24;  author: tridge;  state: Exp;  lines: +5 -13
 * put the ifdef for HAVE_VA_COPY in one place rather than in lots of
 * functions
 *
 * date: 2002/05/17 14:51:22;  author: jmcd;  state: Exp;  lines: +21 -4
 * Fix usage of va_list passed as an arg.  Use __va_copy before using it
 * when it exists.
 *
 * date: 2002/04/16 22:38:04;  author: idra;  state: Exp;  lines: +20 -14
 * Fix incorrect zpadlen handling in fmtfp.
 * Thanks to Ollie Oldham <ollie.oldham@metro-optix.com> for spotting it.
 * few mods to make it easier to compile the tests.
 * addedd the "Ollie" test to the floating point ones.
 *
 * Martin Pool (mbp@samba.org) April 2003
 *    Remove NO_CONFIG_H so that the test case can be built within a source
 *    tree with less trouble.
 *    Remove unnecessary SAFE_FREE() definition.
 *
 * Martin Pool (mbp@samba.org) May 2003
 *    Put in a prototype for dummy_snprintf() to quiet compiler warnings.
 *
 *    Move #endif to make sure VA_COPY, LDOUBLE, etc are defined even
 *    if the C library has some snprintf functions already.
 **************************************************************/

#include <stdio.h>	//printf
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "format_string_handler.hpp"
#include "bsd_snprintf.h"
#include "cstdmf/debug.hpp"

#  define VA_COPY(dest, src) va_copy(dest, src)

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)

BW_BEGIN_NAMESPACE

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

#define char_to_int(p) ((p)- '0')
#ifndef MAX
# define MAX(p,q) (((p) >= (q)) ? (p) : (q))
#endif

bool handleFormatString( const char *format, FormatStringHandler &handler )
{
	char ch;
	int min;
	int max;
	int state;
	int flags;
	int cflags;
	int vflags;

	int pos;
	size_t strstart, strend;
	const char *format_start = format;

	state = DP_S_DEFAULT;
	flags = cflags = vflags = min = 0;
	max = -1;
	ch = *format++;

	pos = 0;
	strstart = strend = 0;

	while (state != DP_S_DONE) {
		if (ch == '\0')
			state = DP_S_DONE;

		switch(state) {

		case DP_S_DEFAULT:
			if (ch == '%') {
				strend = (format - format_start) - 1;
				if (strstart != strend)
					handler.onString(strstart, strend);
				state = DP_S_FLAGS;
			}
			ch = *format++;
			break;
		case DP_S_FLAGS:
			switch (ch) {
			case '-':
				flags |= DP_F_MINUS;
				ch = *format++;
				break;
			case '+':
				flags |= DP_F_PLUS;
				ch = *format++;
				break;
			case ' ':
				flags |= DP_F_SPACE;
				ch = *format++;
				break;
			case '#':
				flags |= DP_F_NUM;
				ch = *format++;
				break;
			case '0':
				flags |= DP_F_ZERO;
				ch = *format++;
				break;
			default:
				state = DP_S_MIN;
				break;
			}
			break;
		case DP_S_MIN:
			if (isdigit((unsigned char)ch)) {
				min = 10*min + char_to_int (ch);
				ch = *format++;
			} else if (ch == '*') {
				min = 0;
				vflags |= VARIABLE_MIN_WIDTH;
				ch = *format++;
				state = DP_S_DOT;
			} else {
				state = DP_S_DOT;
			}
			break;
		case DP_S_DOT:
			if (ch == '.') {
				state = DP_S_MAX;
				ch = *format++;
			} else {
				state = DP_S_MOD;
			}
			break;
		case DP_S_MAX:
			if (isdigit((unsigned char)ch)) {
				if (max < 0)
					max = 0;
				max = 10*max + char_to_int (ch);
				ch = *format++;
			} else if (ch == '*') {
				max = -1;
				vflags |= VARIABLE_MAX_WIDTH;
				ch = *format++;
				state = DP_S_MOD;
			} else {
				state = DP_S_MOD;
			}
			break;
		case DP_S_MOD:
			switch (ch) {
			case 'h':
				cflags = DP_C_SHORT;
				ch = *format++;
				if (ch == 'h')
				{
					// It's a short short - or char size. Use DP_C_SHORT
					// anyway, it will be promoted safely upwards.
					ch = *format++;
				}
				break;
			case 'l':
				cflags = DP_C_LONG;
				ch = *format++;
				if (ch == 'l') {	/* It's a long long */
					cflags = DP_C_LLONG;
					ch = *format++;
				}
				break;
			case 'z': // size_t
				cflags = DP_C_LONG;
				ch = *format++;
				break;
			case 't': // ptrdiff_t
				cflags = DP_C_LONG;
				ch = *format++;
				break;
			case 'L':
				cflags = DP_C_LDOUBLE;
				ch = *format++;
				break;
			default:
				break;
			}
			state = DP_S_CONV;
			break;
		case DP_S_CONV:
			switch (ch) {
			case 'd':
			case 'i':
				handler.onToken('d', cflags, min, max, flags, 10, vflags);
				break;
			case 'o':
				flags |= DP_F_UNSIGNED;
				handler.onToken('o', cflags, min, max, flags, 8, vflags);
				break;
			case 'u':
				flags |= DP_F_UNSIGNED;
				handler.onToken('u', cflags, min, max, flags, 10, vflags);
				break;
			case 'X':
				flags |= DP_F_UP;
			case 'x':
				flags |= DP_F_UNSIGNED;
				handler.onToken('x', cflags, min, max, flags, 16, vflags);
				break;
			case 'f':
				/* um, floating point? */
				handler.onToken('f', cflags, min, max, flags, 0, vflags);
				break;
			case 'E':
				flags |= DP_F_UP;
			case 'e':
				handler.onToken('e', cflags, min, max, flags, 0, vflags);
				break;
			case 'G':
				flags |= DP_F_UP;
			case 'g':
				handler.onToken('g', cflags, min, max, flags, 0, vflags);
				break;
			case 'c':
				handler.onToken('c', cflags, min, max, flags, 0, vflags);
				break;
			case 's':
				handler.onToken('s', cflags, min, max, flags, 0, vflags);
				break;
			case 'p':
				handler.onToken('p', cflags, min, max, flags, 16, vflags);
				break;
				/* BigWorld: Determined that we wouldn't support this
				   case 'n':
				   if (cflags == DP_C_SHORT) {
				   short int *num;
				   num = va_arg (args, short int *);
				   *num = currlen;
				   } else if (cflags == DP_C_LONG) {
				   long int *num;
				   num = va_arg (args, long int *);
				   *num = (long int)currlen;
				   } else if (cflags == DP_C_LLONG) {
				   LLONG *num;
				   num = va_arg (args, LLONG *);
				   *num = (LLONG)currlen;
				   } else {
				   int *num;
				   num = va_arg (args, int *);
				   *num = currlen;
				   }
				   break;
				*/
			case '%':
				handler.onString( format - format_start - 1,
					format - format_start );
				break;
			case 'w':
				/* not supported yet, treat as next char */
				ch = *format++;
				break;
			default:
				ERROR_MSG( "handleFormatString( \"%s\" ) illegal format "
							  "char: '%c'\n", format_start, ch );
				/* Unknown, skip */
				return false;
			}
			ch = *format++;
			state = DP_S_DEFAULT;
			flags = cflags = vflags = min = 0;
			max = -1;
			strstart = strend = (format - format_start) - 1;
			break;
		case DP_S_DONE:
			break;
		default:
			/* hmm? */
			break; /* some picky compilers need this */
		}

		pos++;
	}

	strend = (format - format_start) - 1;
	if (strstart != strend)
		handler.onString(strstart, strend);

	return true;
}


/* BigWorld:
 * The following function have been left as part of the bsd-snprintf file as
 * only minor modifications were made from the original code. This way
 * copyright and license notices are much more manageable
 */
void bsdFormatString( const char *value, int flags,
	int min, int max, BW::string &out )
{
	int padlen, strln;     /* amount to pad */
	int cnt = 0;

#ifdef DEBUG_SNPRINTF
	printf("fmtstr min=%d max=%d s=[%s]\n", min, max, value);
#endif
	if (value == 0) {
		value = "(null)";
	}

	// strlen()
	for (strln = 0; (max == -1 || strln < max) && value[strln]; ++strln);

	padlen = min - strln;
	if (padlen < 0)
		padlen = 0;
	if (flags & DP_F_MINUS)
		padlen = -padlen; /* Left Justify */

	while ((padlen > 0) && (cnt < max || max == -1)) {
		out.push_back( ' ' );
		--padlen;
		++cnt;
	}
	while (*value && (cnt < max || max == -1)) {
		out.push_back( *value++ );
		++cnt;
	}
	while ((padlen < 0) && (cnt < max || max == -1)) {
		out.push_back( ' ' );
		++padlen;
		++cnt;
	}
}

void bsdFormatInt( LLONG value, uint8 base, int min, int max, int flags,
	BW::string &out)
{
	char signvalue = 0;
	ULLONG uvalue;
	char convert[20];
	int place = 0;
	int spadlen = 0; /* amount to space pad */
	int zpadlen = 0; /* amount to zero pad */
	int caps = 0;

	if (max < 0)
		max = 0;

	uvalue = value;

	if(!(flags & DP_F_UNSIGNED)) {
		if( value < 0 ) {
			signvalue = '-';
			uvalue = -value;
		} else {
			if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
				signvalue = '+';
			else if (flags & DP_F_SPACE)
				signvalue = ' ';
		}
	}

	if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

	do {
		convert[place++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")
			[uvalue % (unsigned)base  ];
		uvalue = (uvalue / (unsigned)base );
	} while(uvalue && (place < 20));
	if (place == 20) place--;
	convert[place] = 0;

	zpadlen = max - place;
	spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
	if (zpadlen < 0) zpadlen = 0;
	if (spadlen < 0) spadlen = 0;
	if (flags & DP_F_ZERO) {
		zpadlen = MAX(zpadlen, spadlen);
		spadlen = 0;
	}
	if (flags & DP_F_MINUS)
		spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
	printf("zpad: %d, spad: %d, min: %d, max: %d, place: %d, base: %d\n",
		zpadlen, spadlen, min, max, place, base);
#endif

	/* Spaces */
	while (spadlen > 0) {
		out.push_back( ' ' );
		--spadlen;
	}

	/* Sign */
	if (signvalue)
		out.push_back( signvalue );

	/* Zeros */
	if (zpadlen > 0) {
		while (zpadlen > 0) {
			out.push_back( '0' );
			--zpadlen;
		}
	}

	/* Digits */
	while (place > 0)
		out.push_back( convert[--place] );

	/* Left Justified spaces */
	while (spadlen < 0) {
		out.push_back( ' ' );
		++spadlen;
	}
}

static LDOUBLE abs_val(LDOUBLE value)
{
	LDOUBLE result = value;

	if (value < 0)
		result = -value;

	return result;
}

static LDOUBLE POW10(int exp)
{
	LDOUBLE result = 1;

	while (exp) {
		result *= 10;
		exp--;
	}

	return result;
}

static LLONG ROUND(LDOUBLE value)
{
	LLONG intpart;

	intpart = (LLONG)value;
	value = value - intpart;
	if (value >= 0.5) intpart++;

	return intpart;
}

/* a replacement for modf that doesn't need the math library. Should
   be portable, but slow */
static double my_modf(double x0, double *iptr)
{
	int i;
	long l;
	double x = x0;
	double f = 1.0;

	for (i=0;i<100;i++) {
		l = (long)x;
		if (l <= (x+1) && l >= (x-1)) break;
		x *= 0.1;
		f *= 10.0;
	}

	if (i == 100) {
		/* yikes! the number is beyond what we can handle. What do we do? */
		(*iptr) = 0;
		return 0;
	}

	if (i != 0) {
		double i2;
		double ret;

		ret = my_modf(x0-l*f, &i2);
		(*iptr) = l*f + i2;
		return ret;
	}

	(*iptr) = l;
	return x - (*iptr);
}

void bsdFormatFloat( LDOUBLE fvalue, int min, int max, int flags,
	BW::string &out )
{
	if (bw_isnan( fvalue ))
	{
		out.append( "nan" );
		return;
	}

	if (bw_isinf( fvalue ))
	{
		if (fvalue < 0)
			out.append( "-inf" );
		else
			out.append( "inf" );

		return;
	}

	char signvalue = 0;
	double ufvalue;
	char iconvert[311];
	char fconvert[311];
	int iplace = 0;
	int fplace = 0;
	int padlen = 0; /* amount to pad */
	int zpadlen = 0;
	int caps = 0;
	int idx;
	double intpart;
	double fracpart;
	double temp;

	/*
	 * AIX manpage says the default is 0, but Solaris says the default
	 * is 6, and sprintf on AIX defaults to 6
	 */
	if (max < 0)
		max = 6;

	ufvalue = abs_val (fvalue);

	if (fvalue < 0) {
		signvalue = '-';
	} else {
		if (flags & DP_F_PLUS) { /* Do a sign (+/i) */
			signvalue = '+';
		} else {
			if (flags & DP_F_SPACE)
				signvalue = ' ';
		}
	}

#if 0
	if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */
#endif

#if 0
	if (max == 0) ufvalue += 0.5; /* if max = 0 we must round */
#endif

	/*
	 * Sorry, we only support 16 digits past the decimal because of our
	 * conversion method
	 */
	if (max > 16)
		max = 16;

	/* We "cheat" by converting the fractional part to integer by
	 * multiplying by a factor of 10
	 */

	temp = ufvalue;
	my_modf(temp, &intpart);

	fracpart = (double)ROUND((POW10(max)) * (ufvalue - intpart));

	if (fracpart >= POW10(max)) {
		intpart++;
		fracpart -= POW10(max);
	}

	/* Convert integer part */
	do {
		temp = intpart*0.1;
		my_modf(temp, &intpart);
		idx = (int) ((temp -intpart +0.05)* 10.0);
		/* idx = (int) (((double)(temp*0.1) -intpart +0.05) *10.0); */
		/* printf ("%llf, %f, %x\n", temp, intpart, idx); */
		iconvert[iplace++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")[idx];
	} while (intpart && (iplace < 311));
	if (iplace == 311) iplace--;
	iconvert[iplace] = 0;

	/* Convert fractional part */
	if (fracpart)
	{
		do {
			temp = fracpart*0.1;
			my_modf(temp, &fracpart);
			idx = (int) ((temp -fracpart +0.05)* 10.0);
			/* idx = (int) ((((temp/10) -fracpart) +0.05) *10); */
			/* printf ("%lf, %lf, %ld\n", temp, fracpart, idx ); */
			fconvert[fplace++] =
				(caps? "0123456789ABCDEF":"0123456789abcdef")[idx];
		} while(fracpart && (fplace < 311));
		if (fplace == 311) fplace--;
	}
	fconvert[fplace] = 0;

	/* -1 for decimal point, another -1 if we are printing a sign */
	padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0);
	zpadlen = max - fplace;
	if (zpadlen < 0) zpadlen = 0;
	if (padlen < 0)
		padlen = 0;
	if (flags & DP_F_MINUS)
		padlen = -padlen; /* Left Justifty */

	if ((flags & DP_F_ZERO) && (padlen > 0)) {
		if (signvalue) {
			out.push_back( signvalue );
			--padlen;
			signvalue = 0;
		}
		while (padlen > 0) {
			out.push_back( '0' );
			--padlen;
		}
	}
	while (padlen > 0) {
		out.push_back( ' ' );
		--padlen;
	}
	if (signvalue)
		out.push_back( signvalue );

	while (iplace > 0)
		out.push_back( iconvert[--iplace] );

#ifdef DEBUG_SNPRINTF
	printf("fmtfp: fplace=%d zpadlen=%d\n", fplace, zpadlen);
#endif

	/*
	 * Decimal point.  This should probably use locale to find the correct
	 * char to print out.
	 */
	if (max > 0) {
		out.push_back( '.' );

		while (zpadlen > 0) {
			out.push_back( '0' );
			--zpadlen;
		}

		while (fplace > 0)
			out.push_back( fconvert[--fplace] );
	}

	while (padlen < 0) {
		out.push_back( ' ' );
		++padlen;
	}
}

BW_END_NAMESPACE

#endif /* !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) */
