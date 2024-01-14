#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include "smfio.h"

/* MIDI Structure
 *
 * SMF = <header_chunk> + <track_chunk> [+ <track_chunk> ...]
 *
 * header_chunk = (char[4])"MThd" + (unsigned long int)header_length + 
 *                (unsigned short int)format + (unsigned short int)num_tracks +
 *                (unsigned short int)division
 *
 * track_chunk  = (char[4])"MTrk" + (unsigned long int)length + <track_event> 
 *                [+ <track_event> ...]
 *
 * track_event  = (vlq)delta_time + ( <midi_event> | <meta_event> | <sysex_event> )
 *
 * midi_event   = [(char)status_byte +] (char)midi_data_byte [+ (char)midi_data_byte]
 *
 * meta_event   = (char)status_byte + (char)type + (vlq)length + <meta_event_data>
 *
 * sysex_event  = (char)status_byte + (vlq)length + <messege>
 *
 *
 * Notes
 * 1. The status byte is of a midi_event is optional if and only if the previous event was a midi_event.
 */

const unsigned int OPT_TEST = 1 << 0;
const unsigned int OPT_HELP = 1 << 1;
const unsigned int OPT_FIX  = 1 << 2;
const unsigned int OPT_VERS = 1 << 3;

const unsigned char sig_header[] = {'M','T','h','d'};
const unsigned char sig_track[]  = {'M','T','r','k'};

int verbose;

typedef struct _track_event {
	uint32 delta_time;
	uint32 length; // used for meta and sysex events only
	uint8* data;
	struct _track_event* next_event; // for internal purposes only

	uint8  status; // optional for midi events when running status is in effect
	uint8  type;   // used for meta events only
} track_event;

typedef struct _track_chunk {
	uint8 signature[4];
	uint32 length;

	// the members below are for internal purposes only
	track_event* first_event;
	void* data; // this is for storing the data of uknown chunks
} track_chunk;

typedef struct _midi_header {
	uint8 signature[4];
	uint32 length;
	uint16 format;
	uint16 num_tracks;
	uint16 division;
} midi_header;

typedef struct _smf_midi {
	off_t old_filesize;
	off_t new_filesize;
	midi_header* header;
	track_chunk** chunks;
} smf_midi;

void free_smf( smf_midi* smf );
void free_chunk( track_chunk* chunk );
void shutdown( smf_midi* smf, FILE* file );
int read_midi( smf_midi* smf, char* filename );
int read_track_chunk( smf_midi* smf, track_chunk* chunk, FILE* infile, char* filename );
int write_midi( smf_midi* smf, char* filename );
int write_track_chunk( smf_midi* smf, track_chunk* chunk, FILE* outfile, char* filename );
void shrink_midi( smf_midi* smf );
void fix_track_lengths( smf_midi* smf );
