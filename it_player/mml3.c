#include <stdlib.h>
#include <string.h>
#include "eztypes.h"
#include "mml3.h"
#include <math.h>

static const char start_marker[] = "[**SUPER-NSF**]";
static const char delimiters[] = " \t";

static const char *text;

enum {
	CMD_MAP,
	CMD_MACRO,
	CMD_MIXING,
	CMD_UNKNOWN
};

const char *command_list[] = {
	"map",
	"macro",
	"mixing",
	0
};

const char *channel_list[] = {
	"sqr", "tri", "nse", "vrc", "vrs", "pcm", 0
};

char lowercase( char c ) {
	if( c >= 'A' && c <= 'Z' ) {
		c += 'a' - 'A';
	}
	return c;
}

int find_start_marker() {

	int matches=0;

	while( *text ) {
		if( (*text++) == start_marker[matches] ) {
			matches++;
			if( start_marker[matches] == 0 ) {
				return 0;
			}
		} else {
			matches = 0;
		}
	}
	return 1;
}

void next_line() {
	while( (*text) ) {
		if( (*text) != '\r' && (*text) != '\n' ) {
			text++;
		} else {
			break;
		}
	}
	if( *text == '\r' ) text++;
	if( *text == '\n' ) text++;
}

int whitespace( char c ) {
	int i;
	for( i = 0; delimiters[i]; i++ ) {
		if( c == delimiters[i] ) {
			return 1;
		}
	}
	return 0;
}

int end_of_term( char c ) {
	return whitespace(c) || (c=='\r') || (c=='\n') || (c==0) || (c=='#');
}

void find_term() {
	while( whitespace( *text ) ) {
		*text++;
	}
}

void pass_term() {
	while( !end_of_term(*text) ) {
		*text++;
	}
}

void find_next_term() {
	pass_term();
	find_term();
}

int term_search( const char *list[] ) {
	int cmdtest;
	for( cmdtest = 0; list[cmdtest]; cmdtest++ ) {
		const char *test = list[cmdtest];
		int n;
		int found = 0;
		for( n = 0; !end_of_term(text[n]); n++ ) {
			found = 1;
			if( test[n] != lowercase(text[n]) ) {
				found = 0;
				break;
			}
			if( test[n] == 0 ) {
				found = 0;
				break;
			}
		}
		if( found )
			return cmdtest;
	}
	return -1;
}

int get_command() {
	return term_search( command_list );
}

int get_channel() {
	return term_search( channel_list );
}

int end_of_command() {
	if( (*text) == '#' || (*text) == '\r' || (*text) == '\n' || (*text) == 0 )
		return 1;
	return 0;
}

int term2number( int decimal ) {
	int result=0;
	while( !end_of_term( *text) ) {
		int c = lowercase(*text++);
		if( decimal )
			result *= 10;
		else
			result *= 16;
		if( c >= '0' && c <= '9' ) 
			result += c-'0';
		if( !decimal )
			if( c >= 'a' && c <= 'f' )
				result += c-'a'+10;
	}
	return result;
}

void parse_mml_data( mml_data *dest, const char *input_mml ) {
	memset( dest, 0, sizeof( mml_data ) );
	text = input_mml;

	{
		int i;
		for( i = 0; i < 11; i++ )
			dest->mixing[i] = 100;
		dest->pcm_mixing = 1.0;
	}

	if( find_start_marker() )
		return;
	
	find_term();
	
	while( *text ) {
		int cmd = get_command();
		find_next_term();
		switch( cmd ) {
		case CMD_MAP: {
			int chwrite;
			for( chwrite = 0; chwrite < 11; chwrite++ ) {
				int ch;
				if( end_of_command() ) {
					break;
				}
				ch = get_channel();
				find_next_term();
				
				switch( ch ) {
				case 0: // 2a03pulse
					dest->chmap[chwrite] = CH_2A03_PULSE1;
					for( ch = 0; ch < 11; ch++ ) {
						if( dest->chmap[ch] == CH_2A03_PULSE1 && (ch != chwrite) ) {
							dest->chmap[chwrite]++;
							break;
						}
					}
					break;
				case 1:
					dest->chmap[chwrite] = CH_2A03_TRI;
					break;
				case 2:
					dest->chmap[chwrite] = CH_2A03_NOISE;
					break;
				case 3:
					dest->chmap[chwrite] = CH_VRC6_PULSE1;
					dest->vrc6 = 1;
					for( ch = 0; ch < 11; ch++ ) {
						if( dest->chmap[ch] == CH_VRC6_PULSE1 && (ch != chwrite) ) {
							dest->chmap[chwrite]++;
							break;
						}
					}
					break;
				case 4:
					dest->chmap[chwrite] = CH_VRC6_SAWTOOTH;
					dest->vrc6 = 1;
					break;
				case 5:
					dest->chmap[chwrite] = CH_PCM1;
					dest->pcm_channels++;
					for( ch = 0; ch < 11; ch++ ) {
						if( dest->chmap[ch] >= CH_PCM1 && dest->chmap[ch] <= CH_PCM4 && (ch != chwrite) ) {
							dest->chmap[chwrite]++;
						}
					}
					break;
				}
			}
			
			} break;
		case CMD_MIXING: {
			int position = 0;
			find_term();
			while( !end_of_command() ) {
				dest->mixing[position++] = term2number(1);
				find_next_term();
			}
			
			}break;
		case CMD_MACRO: {
			int macro_index = term2number(1);
			mml_macro *mac = dest->macros + (macro_index-1);
			int position = 0;
			int foundloop = 0;
			find_term();
			while( !end_of_command() ) {
				if( (*text) == '|' ) {
					mac->loop = position;
					foundloop = 1;
				} else {
					mac->nodes[position++] = term2number(0);
				}
				
				find_next_term();
			}

			if( !foundloop )
				mac->loop = position - 1;
			mac->length = position;
			} break;
		}
		next_line();
	}

	// get pcm levels
	{
		int pcm_max=0;
		int i;
		for( i = 0; i < 11; i++ ) {
			if( dest->chmap[i] >= CH_PCM1 || dest->chmap[i] <= CH_PCM4 ) {
				if( dest->mixing[i] > pcm_max ) {
					pcm_max = dest->mixing[i];
				}
			}
		}

		if( pcm_max != 100 && pcm_max != 0 ) {
			double normalize = 100.0 / (double)pcm_max;
			
			for( i = 0; i < 11; i++ ) {
				if( dest->chmap[i] >= CH_PCM1 || dest->chmap[i] <= CH_PCM4 ) {
					dest->mixing[i] = (int)floor(dest->mixing[i] * normalize + 0.5);
				}
			}

			dest->pcm_mixing = (1.0 / normalize);
		}
	}
}
