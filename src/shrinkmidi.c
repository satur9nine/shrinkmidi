#include "shrinkmidi.h"

int main( int argc, char** argv) {
	int i, j;
	int options = 0;
	int numfilesneeded = 2;
	int temp;
	smf_midi smf;

	verbose = 0;

	for( i = 1; i < argc ; i++) {
		if( strncmp( argv[i], "-", 1 ) ) {
			break;
		} else if( strncmp( &(argv[i][1]), "t", 1 )==0 || 
		           strncmp( &(argv[i][1]), "-test", 5 )==0 ) {
			options |= OPT_TEST;
			numfilesneeded = 1;
		} else if( strncmp( &(argv[i][1]), "h", 1 )==0 || 
		           strncmp( &(argv[i][1]), "?", 1 )==0 || 
		           strncmp( &(argv[i][1]), "-help", 5 )==0 ) {
			options |= OPT_HELP;
		} else if( strncmp( &(argv[i][1]), "v", 1 )==0 || 
		           strncmp( &(argv[i][1]), "-verbose", 7 )==0 ) {
			temp = atoi( &argv[i][strlen(argv[i])-1] );
			if( temp>0 ) {
				verbose += temp;
			} else {
				verbose++;
			}
			for( j = 2 ; j < strlen(argv[i]) ; j++ ) {
				if( strncmp( &(argv[i][j]), "v", 1 )==0 ) {
					verbose++;
				}
			}
		} else if( strncmp( &(argv[i][1]), "f", 1 )==0 || strncmp( &(argv[i][1]), "-fix", 4 )==0 ) {
			options |= OPT_FIX;
		} else if( strncmp( &(argv[i][1]), "V", 1 )==0 || strncmp( &(argv[i][1]), "-version", 8 )==0 ) {
			options |= OPT_VERS;
		}
	}


	if( options&OPT_VERS ) {
		fprintf( stderr, "shrinkmidi 0.1 by Jacob Whitaker Abrams\n", argv[0] );
	}

	if( (options&OPT_HELP) || ((argc-i) < numfilesneeded) ) {
		if( ((argc-i) < numfilesneeded) ) {
			fprintf( stderr, "Error: Insufficient input files.\n" );
		}
		fprintf( stderr, "Usage: %s [OPTION]... [INPUTFILE] [OUTPUTFILE] \n", argv[0] );
		fprintf( stderr, "\nOptions:\n" );
		fprintf( stderr, "  -t, --test\t\ttests the input file only.\n" );
		fprintf( stderr, "  -f, --fix\t\tattempts to correct errors only.\n" );
		fprintf( stderr, "  -v, --verbose[=level]\texplain what is being done, verbosity level optional.\n" );
		fprintf( stderr, "  -V, --version\t\tversion of the program.\n" );
	} else {
		printf( "\n" );
		read_midi( &smf, argv[i] );
		if( !(options&OPT_TEST) ) {
			if( !(options&OPT_FIX) ) {
				shrink_midi( &smf );
			}
			fix_track_lengths( &smf );
			write_midi( &smf, argv[i+1] );
			printf( "\t%5d bytes\t%5d bytes\t%3d%%\n", smf.old_filesize, smf.new_filesize, (smf.new_filesize*100)/smf.old_filesize );
		}
		free_smf( &smf ); 
	}

	return 0;
}

void free_chunk( track_chunk* chunk ) {
	track_event *track, *temp;

	if( chunk != NULL ) {
		if( chunk->data!=NULL ) {
			free( chunk->data );
		} else {
			track = chunk->first_event;
			while( track!=NULL ) {
				free( track->data );
				temp = track->next_event;
				free( track );
				track = temp;
			}
		}
		free( chunk );
	}
}

void free_smf( smf_midi* smf ) {
	int i;
	if( smf->header == NULL ) {
		return;
	}
	for( i = smf->header->num_tracks-1 ; i >= 0 ; i-- ) {
		free_chunk( smf->chunks[i] );
	}
	free( smf->chunks );
	free( smf->header );
}

void shutdown( smf_midi* smf, FILE* file ) {
	if( file != NULL ) {
		fclose( file );
	}
	if( smf != NULL ) {
		free_smf( smf );
	}
	exit( 1 );
}

void shrink_midi( smf_midi* smf ) {
	track_event* event;
	track_event* event_to_free;
	track_chunk* chunk_to_free;
	int i, j;
	int num_tracks_before;

	num_tracks_before = smf->header->num_tracks;

	i = 0;
	while( i < smf->header->num_tracks ) {
		chunk_to_free = NULL;

		event = smf->chunks[i]->first_event;

		while( event!=NULL ) {
			event_to_free = NULL;

			// Meta Event
			if( (event->status)==0xFF ) {
				switch( event->type ) {
					// sequence number
					case 0x00:
					// text
					case 0x01:
					// copyright
					case 0x02:
					// sequence/track name
					case 0x03:
					// instrument name
					case 0x04:
					// lyric
					case 0x05:
					// marker
					case 0x06:
					// cue point
					case 0x07:
					// program name
					case 0x08:
					// device name
					case 0x09:
					// midi channel prefix
					case 0x20:
					// midi port
					case 0x21:
					// sequencer specific event
					case 0x7F:
						if( event->next_event != NULL ) {
							event->delta_time = event->next_event->delta_time;
							event->length = event->next_event->length;
							event->status = event->next_event->status;
							event->type = event->next_event->type;
							if( event->data != NULL ) {
								free( event->data );
							}
							event->data = event->next_event->data;
							event_to_free = event->next_event;
							event->next_event = event->next_event->next_event;
						}
						break;
					// smpte offset
					case 0x54:
					// tempo
					case 0x51:
					// time signature
					case 0x58:
					// key signature
					case 0x59:
					// end of track
					case 0x2F:
						break;
				}
			}
			// Sysex Event
			else if( ((event->status)&0xF0)==0xF0 || ((event->status)&0xF7)==0xF7 ) {
				if( event->next_event != NULL ) {
					event->delta_time = event->next_event->delta_time;
					event->length = event->next_event->length;
					event->status = event->next_event->status;
					event->type = event->next_event->type;
					if( event->data != NULL ) {
						free( event->data );
					}
					event->data = event->next_event->data;
					event_to_free = event->next_event;
					event->next_event = event->next_event->next_event;
				}
			}

			if( event_to_free != NULL ) {
				free( event_to_free );
			} else {
				event = event->next_event;
			}
		}

		if( (smf->chunks[i]->first_event->status & 0xFF)==0xFF && 
		    (smf->chunks[i]->first_event->type & 0x2F)==0x2F ) {
			free_chunk( smf->chunks[i] );
			smf->chunks[i] = NULL;
			for( j = i+1 ; j < smf->header->num_tracks ; j++) {
				smf->chunks[j-1] = smf->chunks[j];
			}
			smf->header->num_tracks--;
		} else {
			i++;
		}
	}
	if( verbose > 0 ) {
		printf( "\t%5d %s\t%5d %s\t%3d%%\n", num_tracks_before, num_tracks_before>1?"tracks":"track", smf->header->num_tracks, smf->header->num_tracks>1?"tracks":"track", (smf->header->num_tracks*100)/num_tracks_before );
	}
}

void fix_track_lengths( smf_midi* smf ) {
	track_event* event;
	int i;
	uint32 chunk_length;
	uint8 savedstatus = 0xFF;

	for( i = 0 ; i < smf->header->num_tracks ; i++ ) {
		chunk_length = 0;
		event = smf->chunks[i]->first_event;

		while( event!=NULL ) {

			chunk_length += vlq_length( event->delta_time );

			// MIDI Event
			if( ((event->status)&0xF0)!=0xF0 ) {
				// status
				if( savedstatus!=event->status ) {
					chunk_length++;
				}
				// data
				chunk_length += event->length;
				savedstatus = event->status;
			}
			// Meta Event
			else if( (event->status)==0xFF ) {
				// status
				chunk_length++;
				// type
				chunk_length++;
				// length
				chunk_length += vlq_length( event->length );
				// data
				chunk_length += event->length;
				savedstatus = 0xFF;
			}
			// Sysex Event
			else if( ((event->status)&0xF0)==0xF0 || ((event->status)&0xF7)==0xF7 ) {
				// status
				chunk_length++;
				// length
				chunk_length += vlq_length( event->length );
				// data
				chunk_length += event->length;
				savedstatus = 0xFF;
			}
			event = event->next_event;
		}

		smf->chunks[i]->length = chunk_length;
	}
}

int read_midi( smf_midi* smf, char* filename ) {
	int tracks_read;
	FILE* infile = NULL;
	struct stat infile_stat;

	if( verbose>1 ) {
		printf( "Processing MIDI\n" );
	}

	infile = fopen(filename,"rb");
	if( infile==NULL ) {
		fprintf( stderr, "Error: unable to open file %s.\n", filename );
		exit( 1 );
	}

	stat( filename, &infile_stat );
	smf->old_filesize = infile_stat.st_size;

	smf->header = (midi_header*)malloc( sizeof( midi_header ) );
	if( smf->header==NULL ) {
		fprintf( stderr, "\nError: insufficient memory.\n" );
		exit( 1 );
	}

	fread( smf->header->signature, 1, 4, infile );

	if( memcmp( smf->header->signature, sig_header, 4 )!=0 ) {
		fprintf( stderr, "\nError: %s has an invalid header signature.\n", filename );
		shutdown( NULL, infile );
	}

	fread( &(smf->header->length), 4, 1, infile );
	smf->header->length = reverse32( smf->header->length );

	if( smf->header->length!=6 ) {
		fprintf( stderr, "\nError: %s contains a header of unexpected length.\n", filename );
		shutdown( smf, infile );
	}

	fread( &(smf->header->format), 2, 1, infile );
	smf->header->format = reverse16( smf->header->format );
	fread( &(smf->header->num_tracks), 2, 1, infile );
	smf->header->num_tracks = reverse16( smf->header->num_tracks );

	if( smf->header->num_tracks==0 ) {
		fprintf( stderr, "\nError: %s does not contain a legal number of tracks.\n", filename );
		shutdown( smf, infile );
	}

	smf->chunks = (track_chunk**)malloc( sizeof( track_chunk* )*smf->header->num_tracks );
	if( smf->chunks==NULL ) {
		fprintf( stderr, "\nError: insufficient memory.\n" );
		shutdown( smf, infile );
	}

	fread( &(smf->header->division), 2, 1, infile );
	smf->header->division = reverse16( smf->header->division );

	if( verbose>1 ) {
		printf( "Header Chunk\n  type     = \"%c%c%c%c\"\n", smf->header->signature[0], smf->header->signature[1], smf->header->signature[2], smf->header->signature[3] );
		printf( "  length   = %lu\n", smf->header->length );
		printf( "  format   = %hu\n", smf->header->format );
		printf( "  tracks   = %hu\n", smf->header->num_tracks );
		printf( "  division = %hi\n", smf->header->division );
	}

	tracks_read = 0;
	while( tracks_read<smf->header->num_tracks ) {
		if( feof( infile ) ) {
			fprintf( stderr, "\nError: %s end of file reached unexpectedly.\n", filename );
			shutdown( smf, infile );
		}
		smf->chunks[tracks_read] = (track_chunk*)malloc( sizeof(track_chunk) );
		if( smf->chunks[tracks_read]==NULL ) {
			fprintf( stderr, "\nError: insufficient memory.\n" );
			shutdown( smf, infile );
		}
		smf->chunks[tracks_read]->first_event = NULL;
		smf->chunks[tracks_read]->data = NULL;
		read_track_chunk( smf, smf->chunks[tracks_read], infile, filename );
		tracks_read++;
	}

	fclose( infile );

	return 0;
}

int read_track_chunk( smf_midi* smf, track_chunk* chunk, FILE* infile, char* filename ) {
	int i;
	int bytes_read = 0;
	int total_length;
	int runningstatus = FALSE;
	uint8 savedstatus = 0xFF;
	track_event* event = NULL;

	bytes_read += fread( chunk->signature, 1, 4, infile );

	if( bytes_read<4 || memcmp( chunk->signature, sig_track, 4 ) ) {
		fprintf( stderr, "Error: %s does not appear to be a MIDI, or is corrupt.\n", filename );
		shutdown( smf, infile );
	}
	if( verbose>1 ) {
		printf( "Track Chunk\n  type     = \"%c%c%c%c\"\n", chunk->signature[0], chunk->signature[1], chunk->signature[2], chunk->signature[3] );
	}

	//read track length
	bytes_read += fread( &(chunk->length), 4, 1, infile );
	chunk->length = reverse32( chunk->length );
	if( verbose>1 ) {
		printf( "  length   = %lu\n", chunk->length );
	}

	total_length = bytes_read + chunk->length;

	while( bytes_read < total_length ) {

		if( chunk->first_event==NULL ) {
			chunk->first_event = (track_event*)malloc( sizeof( track_event ) );
			if( chunk->first_event==NULL ) {
				fprintf( stderr, "\nError: insufficient memory.\n" );
				shutdown( smf, infile );
			}
			event = chunk->first_event;
		} else {
			event->next_event = (track_event*)malloc( sizeof( track_event ) );
			if( event->next_event==NULL ) {
				fprintf( stderr, "\nError: insufficient memory.\n" );
				shutdown( smf, infile );
			}
			event = event->next_event;
		}
		event->next_event = NULL;
		event->data = NULL;

		if( verbose>1 ) {
			printf( "Track event\n" );
		}

		//read delta-time
		bytes_read += read_vlq( &(event->delta_time), infile );
		if( verbose>1 ) {
			printf( "  deltatime= %li\n", event->delta_time );
		}

		//read status byte or first midi data byte if running status is enabled
		bytes_read += fread( &(event->status), 1, 1, infile );

		// MIDI Event
		if( ((event->status)&0xF0)!=0xF0 && ( runningstatus || ((event->status)&0x80)==0x80 ) ) {
			if( verbose>1 ) {
				printf( "  -- midi event --\n" );
			}
			if( !runningstatus || ((event->status)&0x80)!=0x00 ) {
				// new midi status
				savedstatus = event->status;
			}
			if( verbose>1 ) {
				printf( "  status   = 0x%X\n", savedstatus );
			}
			switch( (savedstatus)&0xF0 ) {
				// note off
				case 0x80:
					event->length = 2;
					break;
				// note on
				case 0x90:
					event->length = 2;
					break;
				// polyphonic pressue
				case 0xA0:
					event->length = 2;
					break;
				// controller
				case 0xB0:
					event->length = 2;
					break;
				// program change
				case 0xC0:
					event->length = 1;
					break;
				// channel pressure
				case 0xD0:
					event->length = 1;
					break;
				// pitch bend
				case 0xE0:
					event->length = 2;
					break;
				default:
					fprintf( stderr, "Error: encountered unknown midi event type.\n" );
					shutdown( smf, infile );
					break;
			}
			event->data = (uint8*)malloc( event->length );
			if( event->data==NULL ) {
				fprintf( stderr, "\nError: insufficient memory.\n" );
				shutdown( smf, infile );
			}
			if( savedstatus != event->status ) {
				// running status
				event->data[0] = event->status;
				event->status = savedstatus;
				if( (event->length-1)>0 ) {
					bytes_read += fread( &(event->data[1]), 1, event->length-1, infile );
				}
			} else {
				bytes_read += fread( event->data, 1, event->length, infile );
			}
			if( verbose>1 ) {
				printf( "  data     =" );
				for( i = 0 ; i < event->length ; i++ ) {
					printf( " %02X", event->data[i] );
				}
				printf( "\n" );
			}
			runningstatus = TRUE;
		}
		// Meta Event
		else if( (event->status)==0xFF ) {
			bytes_read += fread( &(event->type), 1, 1, infile );
			if( feof(infile) || ferror(infile ) ) {
			}
			if( verbose>1 ) {
				printf( "  status   = 0x%X\n", event->status );
				printf( "  -- meta event --\n" );
				printf( "  type     = 0x%X\n", event->type );
			}
			switch( event->type ) {
				// sequence number
				case 0x00:
					break;
				// text
				case 0x01:
					break;
				// copyright
				case 0x02:
					break;
				// sequence/track name
				case 0x03:
					break;
				// instrument name
				case 0x04:
					break;
				// lyric
				case 0x05:
					break;
				// marker
				case 0x06:
					break;
				// cue point
				case 0x07:
					break;
				// program name
				case 0x08:
					break;
				// device name
				case 0x09:
					break;
				// midi channel prefix
				case 0x20:
					break;
				// midi port
				case 0x21:
					break;
				// end of track
				case 0x2F:
					break;
				// tempo
				case 0x51:
					break;
				// smpte offset
				case 0x54:
					break;
				// time signature
				case 0x58:
					break;
				// key signature
				case 0x59:
					break;
				// sequencer specific event
				case 0x7F:
					break;
				default:
					fprintf( stderr, "Error: encountered unknown meta event type.\n" );
					shutdown( smf, infile );
					break;
			}
			bytes_read += read_vlq( &(event->length), infile );
			if( verbose>1 ) {
				printf( "  length   = %i\n", event->length );
			}
			if( event->length>0 ) {
				event->data = (uint8*)malloc( event->length );
				if( event->data==NULL ) {
					fprintf( stderr, "\nError: insufficient memory.\n" );
					shutdown( smf, infile );
				}
				bytes_read += fread( event->data, 1, event->length, infile );
				if( verbose>1 ) {
					printf( "  data     =" );
					for( i = 0 ; i < event->length ; i++ ) {
						printf( " %02X", event->data[i] );
					}
					printf( "\n" );
				}
			} else {
				event->data = NULL;
			}
			runningstatus = FALSE;
		}
		// Sysex Event
		else if( ((event->status)&0xF0)==0xF0 || ((event->status)&0xF7)==0xF7 ) {
			if( verbose>1 ) {
				printf( "  status   = 0x%X\n", event->status );
				printf( "  -- sysex event --\n" );
			}
			bytes_read += read_vlq( &(event->length), infile );
			if( event->length>0 ) {
				event->data = (uint8*)malloc( event->length );
				if( event->data==NULL ) {
					fprintf( stderr, "\nError: insufficient memory.\n" );
					shutdown( smf, infile );
				}
				bytes_read += fread( event->data, 1, event->length, infile );
				if( verbose>1 ) {
					printf( "  data     =" );
					for( i = 0 ; i < event->length ; i++ ) {
						printf( " %02X", event->data[i] );
					}
					printf( "\n" );
				}
				if( (event->data[event->length-1])!=0xF7 ) {
					fprintf( stderr, "Warning: Encountered special casio-style sysex event." );
				}
			} else {
				event->data = NULL;
			}
			runningstatus = FALSE;
		}
		// Error
		else {
			fprintf( stderr, "Error: encountered unknown track event type.\n" );
			shutdown( smf, infile );
		}
	}

	return bytes_read;
}

int write_midi( smf_midi* smf, char* filename ) {
	FILE* outfile;
	int tracks_written;
	int32 tmp32;
	int16 tmp16;
	struct stat outfile_stat;

	outfile = fopen(filename,"wb");
	if( outfile==NULL ) {
		fprintf( stderr, "Error: unable to create file %s.\n", filename );
		shutdown( smf, outfile );
	}

	if( fwrite( smf->header->signature, 1, 4, outfile )!=4 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tmp32 = reverse32( smf->header->length );
	if( fwrite( &tmp32, 4, 1, outfile )!=1 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tmp16 = reverse16( smf->header->format );
	if( fwrite( &tmp16, 2, 1, outfile )!=1 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tmp16 = reverse16( smf->header->num_tracks );
	if( fwrite( &tmp16, 2, 1, outfile )!=1 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tmp16 = reverse16( smf->header->division );
	if( fwrite( &tmp16, 2, 1, outfile )!=1 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tracks_written = 0;
	while( tracks_written<smf->header->num_tracks ) {
		write_track_chunk( smf, smf->chunks[tracks_written], outfile, filename );
		tracks_written++;
	}

	fclose( outfile );

	stat( filename, &outfile_stat );
	smf->new_filesize = outfile_stat.st_size;

	return 0;
}

int write_track_chunk( smf_midi* smf, track_chunk* chunk, FILE* outfile, char* filename ) {
	uint8 savedstatus = 0xFF;
	int32 tmp32;
	track_event* event;

	if( fwrite( chunk->signature, 1, 4, outfile )!=4 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	tmp32 = reverse32( chunk->length );
	if( fwrite( &tmp32, 4, 1, outfile )!=1 ) {
		fprintf( stderr, "Error: disk write error.\n" );
		shutdown( smf, outfile );
	}

	event = chunk->first_event;

	while( event!=NULL ) {

		// write delta-time
		if( write_vlq( event->delta_time, outfile )<=0 ) {
			fprintf( stderr, "Error: disk write error.\n" );
			shutdown( smf, outfile );
		}

		// MIDI Event
		if( ((event->status)&0xF0)!=0xF0 ) {
			// only write status when running status is not enabled
			if( savedstatus!=event->status ) {
				if( fwrite( &(event->status), 1, 1, outfile )!=1 ) {
					fprintf( stderr, "Error: disk write error.\n" );
					shutdown( smf, outfile );
				}
			}
			if( fwrite( event->data, 1, event->length, outfile )!=event->length ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			savedstatus = event->status;
		}
		// Meta Event
		else if( (event->status)==0xFF ) {
			if( fwrite( &(event->status), 1, 1, outfile )!=1 ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			if( fwrite( &(event->type), 1, 1, outfile )!=1 ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			if( write_vlq( event->length, outfile )<=0 ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			if( event->length>0 ) {
				if( fwrite( event->data, 1, event->length, outfile )!=event->length ) {
					fprintf( stderr, "Error: disk write error.\n" );
					shutdown( smf, outfile );
				}
			}
			savedstatus = 0xFF;
		}
		// Sysex Event
		else if( ((event->status)&0xF0)==0xF0 || ((event->status)&0xF7)==0xF7 ) {
			if( fwrite( &(event->status), 1, 1, outfile )!=1 ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			if( write_vlq( event->length, outfile )<=0 ) {
				fprintf( stderr, "Error: disk write error.\n" );
				shutdown( smf, outfile );
			}
			if( event->length>0 ) {
				if( fwrite( event->data, 1, event->length, outfile )!=event->length ) {
					fprintf( stderr, "Error: disk write error.\n" );
					shutdown( smf, outfile );
				}
			}
			savedstatus = 0xFF;
		}
		// Error
		//else {
		//	fprintf( stderr, "Error: encountered unknown track event type.\n" );
		//	shutdown( smf, outfile );
		//}

		event = event->next_event;
	}

	return 0;
}
