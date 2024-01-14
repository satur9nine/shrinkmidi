#include "smfio.h"

uint32 reverse32( uint32 n ) {
	#ifdef BIG_ENDIAN
	return n;
	#else
	uint8 b0, b1, b2, b3;
	b0 = (uint8)((n & 0x000000FF) >> 0);
	b1 = (uint8)((n & 0x0000FF00) >> 8);
	b2 = (uint8)((n & 0x00FF0000) >> 16);
	b3 = (uint8)((n & 0xFF000000) >> 24);
	return (uint32)( (b0 << 24) | (b1 << 16) | (b2 << 8) | (b3 << 0) );
	#endif
}

uint16 reverse16( uint16 n ) {
	#ifdef BIG_ENDIAN
	return n;
	#else
	uint8 b0, b1;
	b0 = (uint8)((n & 0x00FF) >> 0);
	b1 = (uint8)((n & 0xFF00) >> 8);
	return (uint16)( (b0 << 8) | (b1 << 0) );
	#endif
}

int skip_vlq( FILE *infile ) {
	int i = 0;
	int8 byte;

	do {
		byte = (int8)getc( infile );
		if( feof( infile ) || ferror( infile ) ) {
			return 0;
		}
		i++;
	} while( byte & 0x80 );

	return i;
}

int vlq_length( uint32 value ) {
	int i = 0;
	uint32 buffer;
	uint32 val = value;

	if( value & 0xF0000000 ) {
		return 0;
	}

	buffer = value & 0x7F;

	while( (value >>= 7) ) {
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7F);
	}

	while( buffer & 0x80 ) {
		i++;
		buffer >>= 8;
	}
	i++;

	return i;
}

int read_vlq( uint32* value, FILE* infile ) {
	int i = 0;
	uint32 val = 0;
	uint8 byte;

	do {
		val <<= 7;
		byte = (int8)getc( infile );
		if( feof( infile ) || ferror( infile ) ) {
			return 0;
		}
		i++;
		val |= (byte & 0x7F);
	} while( byte & 0x80 );

	(*value) = val;

	return i;
}

int write_vlq( uint32 value, FILE* outfile ) {
	int i = 0;
	uint32 buffer;

	if( value & 0xF0000000 ) {
		return 0;
	}

	buffer = value & 0x7F;

	while( (value >>= 7) ) {
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7F);
	}

	while( buffer & 0x80 ) {
		putc( buffer & 0xFF, outfile );
		if( ferror( outfile ) ) {
			return 0;
		}
		i++;
		buffer >>= 8;
	}
	putc( buffer & 0xFF, outfile );
	if( ferror( outfile ) ) {
		return 0;
	}
	i++;

	return i;
}
