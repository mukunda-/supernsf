
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "eztypes.h"
#include "it_file.h"
#include "mml3.h"

#define min_loop_size 1024
#define smp_pad_size 1024

static int mod_tick;
static int mod_row;
static int mod_position;
static int mod_tempo;
static int mod_speed;
static int mod_pattern;
static int mod_posjump = -1;

static int mod_gvol;

static mml_data mod_mml;


typedef struct {
	int pcm_index;
	int sample_index;
} pcm_remap_entry;

typedef struct {
	int frequency; // sampling rate (hz)
	u8 newnote; // 1 = new note occured (cut freq slides, start samples, reset duty)
	u8 timbre; // 1-255 = sample/duty/noise index, 0=null
	u8 volume; // 0-64
	u8 sample_offset;
	u8 ignore;
} channel_event;

typedef struct {
	channel_event events[11];
	double duration; // MILLISECONDS
} frame;

typedef struct {
	int position;
	int result;
} mod_envelope;

typedef struct {
	int pitch;
	int volume;
	int note;
	int instr;
	int vcmd;
	int effect;
	int param;
	int pflags;
	int sample;
	int tpitch;
	int tvolume;
	int notecut;
	int fadeout;
	int toffset;

	int vibratoPos;
	int vibratoSpeed;
	int vibratoDepth;
	//int glisSpeed;

	int volume_scale;

	int macropos;

	u8 newnote;
	u8 noteoff;

	int vol_env_pos;
	int pitch_env_pos;
	int vol_env_data;
	int pitch_env_data;

	int memory[13];
	int vcmd_mem;
} mod_channel;

typedef enum {
	chmem_vslide,
	chmem_porta,
	chmem_glis,
	chmem_tremor,
	chmem_arp,
	chmem_cvslide,
	chmem_offset,
	chmem_panslide,
	chmem_retrig,
	chmem_tremolo,
	chmem_s,
	chmem_tempo,
	chmem_gvslide
};

#define PF_NOTE 1
#define PF_INSTR 2
#define PF_VCMD 4
#define PF_CMD 8
#define PF_NOTECUT 16
#define PF_NOTEOFF 32

#define PITCH_MIN 0
#define PITCH_MAX 7680

mod_channel channels[11];

frame *pattern = 0;
int pattern_size  =0 ;
int pattern_alloc_size =0;

int *pcm_map;
int reverse_pcm_map[128];
int pcm_sample_count;

IT_File *mod;

#define chunk_size 64

const signed char IT_FineSineData[] = {
   0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
  24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
  59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
  59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
  45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
  24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
   0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
 -24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
 -45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
 -59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
 -64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
 -59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
 -45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
 -24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2 };


frame *add_frame() {
	if( pattern_size == pattern_alloc_size ) {
		frame *newpattern;
		pattern_alloc_size += chunk_size;
		newpattern = malloc(pattern_alloc_size * sizeof(frame));
		memset( newpattern + pattern_size, 0, sizeof(frame)*chunk_size );
		
		if( pattern ) {
			memcpy( newpattern, pattern, sizeof(frame) * pattern_size );
			free(pattern);
		}
		pattern = newpattern;
	}
	pattern_size++;
	return pattern + pattern_size - 1;
}

static void copy_initial_volume() {
	int i;
	for( i = 0; i < 11; i++ ) {
		channels[i].volume_scale = mod->InitialChannelVolume[i];
	}
}

int set_position( int pos, int row ) {
	int looped=0;
	mod_tick=0;
	mod_row=0;
	if( pos > mod->OrdNum )  {
		pos=0;
		looped=1;
	}
	if( mod->OrderList[pos] == 255 ) {
		pos=0;
		looped=1;
	}
	
	mod_position = pos;
	mod_pattern = mod->OrderList[mod_position];
	return looped;
}

void read_pattern() {
	
	int i;
	for( i = 0; i < 11; i++ ) {
		IT_PatternEntry *pe = mod->Patterns[mod_pattern].Data + mod_row * mod->Patterns[mod_pattern].Columns + i;
		channels[i].pflags = 0;
		if( pe->Note != 200 ) {

			if( pe->Note < 200 ) {
				channels[i].note = pe->Note;
				channels[i].pflags |= PF_NOTE;
			} else if( pe->Note == 254 ) {
				channels[i].notecut=1;
				channels[i].pflags |= PF_NOTECUT;
			} else if( pe->Note == 255 )  {
				channels[i].pflags |= PF_NOTEOFF;
			}
		}
		if( pe->Instrument ) {
			channels[i].instr = pe->Instrument;
			channels[i].pflags |= PF_INSTR;
		}
		if( pe->SubCommand != 255 ) {
			channels[i].vcmd = pe->SubCommand;
			channels[i].pflags |= PF_VCMD;
		}
		if( pe->Command ) {
			channels[i].effect = pe->Command;
			channels[i].pflags |= PF_CMD;
			channels[i].param = pe->Param;
		}
		
	}	
}

int no_glissando( mod_channel *ch ) {
	if( (ch->effect == 'G' - 'A' + 1) && (ch->pflags & PF_CMD) ) {
		return 0;
	}
	return 1;
}

int no_notedelay( mod_channel *ch ) {
	if( (ch->effect == ('S' - 'A' +1) && (ch->pflags & PF_CMD) ) &&
		(ch->param&0xF0) == 0xD0 ) {
		return 0;
	}
	return 1;
}

int note_delayed(mod_channel *ch ) {
	if( (ch->effect == ('S' - 'A' +1) && (ch->pflags & PF_CMD) ) &&
		(ch->param&0xF0) == 0xD0 ) {

		if( mod_tick < (ch->param & 0xF) )
			return 1;
	}
	return 0;
}

int satmax( int value, int max ) {
	return value > max ? max : value;
}

int satmin( int value, int min ) {
	return value < min ? min : value;
}

int sat( int value, int lower, int upper ) {
	if( value < lower ) value = lower;
	if( value > upper ) value = upper;
	return value;
}

void process_volume_command( mod_channel *ch ) {
	int vcmd = ch->vcmd;
	if( !(ch->pflags & PF_VCMD) )
		return;
	if( vcmd >= 0 && vcmd <= 64 ) {
		// set volume

		if( mod_tick == 0 )
			ch->volume = vcmd;
	} else if( vcmd >= 65 && vcmd <= 74 ) {
		// fine volume up
		if( mod_tick == 0 )
			ch->volume = satmax( ch->volume + vcmd - 65, 64 );
	} else if( vcmd >= 75 && vcmd <= 84 ) {
		// fine volume donw
		if( mod_tick == 0 )
			ch->volume = satmin( ch->volume - (vcmd-75), 64 );
	} else if( vcmd >= 85 && vcmd <= 94 ) {
		// volume up
		if( mod_tick != 0 )
			ch->volume = satmax( ch->volume + vcmd - 85, 64 );
	} else if( vcmd >= 95 && vcmd <= 104 ) {
		// volume down
		if( mod_tick != 0 )
			ch->volume = satmin( ch->volume - (vcmd-95), 64 ); 
	} else if( vcmd >= 105 && vcmd <= 114 ) {
		// pitch slide down
	} else if( vcmd >= 115 && vcmd <= 124 ) {
		// pitch slide up
	} else if( vcmd >= 193 && vcmd <= 202 ) {
		// glissando
	} else if( vcmd >= 203 && vcmd <= 212 ) {
		// vibrato
	}
}

#define EFFECT(c) (1+(c)-'A')

void do_vibrato( mod_channel *ch ) {
	if( mod_tick != 0 ) {
		ch->vibratoPos += ch->vibratoSpeed;
		ch->vibratoPos &= 255;
	}
	ch->tpitch = sat( ch->pitch - IT_FineSineData[ch->vibratoPos] * ch->vibratoDepth / 64, PITCH_MIN, PITCH_MAX ); // todo: scale properly
}

int translate_vslide( int param ) {
	int x = param >> 4;
	int y = param & 0xF;

	if( y == 0 ) {
		// slide up
		if( mod_tick != 0 || x == 15 )
			return x;
		return 0;
	} else if( x == 0 ) {
		// slide down
		if( mod_tick != 0 || y == 15 )
			return -y;
		return 0;
	} else if( y == 15 )  {
		// fine slide up
		if( mod_tick == 0 )
			return x;
		return 0;
	} else {
		// fine slide down
		if( mod_tick == 0 )
			return -y;
		return 0;
	}
}

void do_vslide( mod_channel *ch, int param ) {
	int slide = translate_vslide(param);
	ch->tvolume = ch->volume = sat( ch->volume + slide, 0, 64 );
}

void do_glissando( mod_channel *ch ) {
	if( mod_tick != 0 ) {
		int targetPitch = ch->note*64;
					
		if( ch->pitch < targetPitch ) {
			ch->tpitch = ch->pitch = satmax( ch->pitch + ch->memory[(mod->Flags&32) ? chmem_porta : chmem_glis]*4, targetPitch );
		} else if( ch->pitch > targetPitch ) {
			ch->tpitch = ch->pitch = satmin( ch->pitch - ch->memory[(mod->Flags&32) ? chmem_porta : chmem_glis]*4, targetPitch );
		}
	}
}

void exchange_mem( mod_channel *ch, int mem ) {
	if( ch->param == 0 ) {
		ch->param = ch->memory[mem];
	} else {
		ch->memory[mem] = ch->param;
	}
}

void play_note_thing( mod_channel *ch ) {
	if( ch->pflags & PF_NOTE && no_glissando(ch)) {
		ch->pitch = ch->note * 64;
		ch->newnote = 1;
		ch->noteoff = 0;
	}
	
	if( ch->pflags & PF_INSTR ) {
		ch->sample = mod->Instruments[ch->instr-1].SampleMap[ch->note];
		if( ch->sample ) {

			ch->volume = mod->Samples[ch->sample-1].DefaultVolume;
		}
		
		ch->fadeout = 1024;
		ch->vol_env_pos = 0;
		ch->pitch_env_pos = 0;
		ch->pitch_env_data = 0;
		ch->vol_env_data = 64*64;

		ch->pflags &= ~(PF_INSTR|PF_NOTE);
	}

}

void process_effect( mod_channel *ch ) {

	switch( ch->effect ) {
		case EFFECT('A'):
			// set speed
			if( mod_tick == 0 && ch->param != 0 )
				mod_speed = ch->param;
			break;
		case EFFECT('B'):
			// todo: position jump
			mod_posjump = ch->param;
			break;
		case EFFECT('C'):
			// todo: pattern breka;
			mod_posjump = mod_position+1;
			break;
		case EFFECT('D'):
			exchange_mem( ch, chmem_vslide );
			// volume slide
			do_vslide( ch, ch->param );
			break;
		case EFFECT('E'):
		case EFFECT('F'):
			exchange_mem( ch, chmem_porta );
			{
 				int negate = ch->effect == EFFECT('E');

				int x = ch->param >> 4;
				int y = ch->param & 0xF;
				// pitch slide
				if( x == 15 ) {
					// fine slide
					if( mod_tick == 0 ) {
						if( !negate )
							ch->tpitch = ch->pitch = satmax( ch->pitch+y*4, PITCH_MAX );
						else
							ch->tpitch = ch->pitch = satmin( ch->pitch-y*4, PITCH_MIN );
					}
				} else if( x == 14 ) {
					// extra fine slide
					if( mod_tick == 0 ) {
						if( !negate )
							ch->tpitch = ch->pitch = satmax( ch->pitch+y, PITCH_MAX );
						else
							ch->tpitch = ch->pitch = satmin( ch->pitch-y, PITCH_MIN );
					}
				} else {
					// normal slide
					if( mod_tick != 0 ) {
						if( !negate )
							ch->tpitch = ch->pitch = satmax( ch->pitch+(x*16+y)*4, PITCH_MAX );
						else
							ch->tpitch = ch->pitch = satmin( ch->pitch-(x*16+y)*4, PITCH_MIN );
					}
				}
			}
			break;
		case EFFECT('G'):
			exchange_mem( ch, (mod->Flags & 32) ? chmem_porta : chmem_glis );
			// glissando
			do_glissando(ch);
				
			break;
		case EFFECT('H'):
			{
				if( mod_tick == 0 ) {
					int x = ch->param >> 4;
					int y = ch->param & 0xF;
					if( x != 0 ) 
						ch->vibratoSpeed = x*4;
					if( y != 0 ) {
						ch->vibratoDepth = y*4;
						if( mod->Flags & 16 )
							ch->vibratoDepth *= 2;
					}
					
				} 
				do_vibrato(ch);
				
			}
			break;
		case EFFECT('I'):
			exchange_mem( ch, chmem_tremor );
			// todo: tremor;
			break;
		case EFFECT('J'):
			// arpeggio
			exchange_mem( ch, chmem_arp );

			switch( mod_tick % 3 ) {
				case 1:
					ch->tpitch = ch->pitch + (ch->param>>4)*64;
					break;
				case 2:
					ch->tpitch = ch->pitch + (ch->param&0xF)*64;
					break;
			}
			break;
		case EFFECT('K'):
			// vib+vslide
			exchange_mem( ch, chmem_vslide );
			do_vibrato(ch);
			do_vslide(ch, ch->param);
			break;
		case EFFECT('L'):
			// glis+vslide
			exchange_mem( ch, chmem_vslide );
			do_glissando(ch);
			do_vslide(ch, ch->param);
			break;
		case EFFECT('M'):
			// channel volume
			ch->volume_scale = satmax( ch->param, 64 );
			break;
		case EFFECT('N'):
			// slide channel volume
			exchange_mem( ch, chmem_cvslide );
			ch->volume_scale = sat( ch->volume_scale + translate_vslide( ch->param ), 0, 64 );
			break;
		case EFFECT('O'):
			// sample offset
			exchange_mem( ch, chmem_offset );
			ch->toffset = ch->param;
			break;
		case EFFECT('P'):
			// panning slide
			exchange_mem( ch, chmem_panslide );
			break;
		case EFFECT('Q'):
			// retrigger note
			exchange_mem( ch, chmem_retrig );
			break;
		case EFFECT('R'):
			// tremolo
			exchange_mem( ch, chmem_tremolo );
			break;
		case EFFECT('S'): {
			// extended
			int y;
			exchange_mem( ch, chmem_s );
			y = ch->param & 0xF;

			switch( ch->param >> 4 ) {
				
				case 0xC:
					//note cut
					if( mod_tick == y ) {
						ch->tvolume = ch->volume = 0;
					}
					break;
				case 0xD:
					//note delay
					if( mod_tick == 0 ) {
						ch->newnote = 0;
					}
					if( mod_tick == y ) {
						ch->newnote = 1;
					} 
			}
			}break;
		case EFFECT('T'):
			// tempo
			exchange_mem( ch, chmem_tempo ); {
			int x = ch->param >> 4;
			int y = ch->param & 0xF;
			if( x == 0 ) {
				if( mod_tick != 0 )
					mod_tempo = sat(mod_tempo - y, 32, 255 );
			} else if( x == 1 ) {
				if( mod_tick != 0 )
					mod_tempo = sat(mod_tempo+y, 32,255);
			} else{
				if( mod_tick == 0 )
					mod_tempo = ch->param;
			}
			}break;
		case EFFECT('U'):
			// fine vibrato
			if( mod_tick == 0 ) {
				
				int x = ch->param >> 4;
				int y = ch->param & 0xF;
				if( x != 0 )
					ch->vibratoSpeed = 4*x;
				if( y != 0 ) {
					ch->vibratoDepth = y;
					
					if( mod->Flags & 16 )
						ch->vibratoDepth *= 2;
				}
			}
			do_vibrato(ch);
			break;
		case EFFECT('V'):
			if( mod_tick == 0 ) {
				mod_gvol = sat(ch->param,0,0x80);
			}
			break;
		case EFFECT('W'):
			exchange_mem( ch, chmem_gvslide );
			mod_gvol = sat( mod_gvol + translate_vslide(ch->param), 0, 0x80 );
			break;
		case EFFECT('X'):
			//pood
			break;
		case EFFECT('Y'):
			//panbrello
			break;
		case EFFECT('Z'):
			//macro
			break;
	}
}

void process_envelope( const IT_Envelope *source, int *ch_pos, int *ch_result, int noteoff ) {
	if( source->Enabled ) {
		int cnode;
		int tick0;
		for( cnode = source->NodeCount-1; cnode >= 0; cnode-- ) {
			if( (*ch_pos) >= source->NodeX[cnode] )
				break;
		}
		if( (*ch_pos) == source->NodeX[cnode] ) {
			(*ch_result) = source->NodeY[cnode]*64;
			tick0=1;
		} else {
			int noderel = (*ch_pos) - source->NodeX[cnode];
			int range = source->NodeX[cnode+1] - source->NodeX[cnode];

			// linear interpolation
			(*ch_result) = 
				((source->NodeY[cnode] * (range-noderel) + source->NodeY[cnode+1] * (noderel))*64) / range;
			tick0=0;
		}

		if( tick0 ) {
			int looped=0;

			// sustain loops
			if( !noteoff ) {
				if( source->SusLoopEnabled ) {
					if( cnode == source->SusLoopEnd ) {
						(*ch_pos) = source->NodeX[source->SusLoopStart];
						looped=1;
					}
				}
			}
			
			if( !looped ) {
				if( source->LoopEnabled ) {
					if( cnode == source->LoopEnd ) {
						(*ch_pos) = source->NodeX[source->LoopStart];
						looped=1;
					}
				}
			}
			
			if( !looped ) {
				if( cnode != source->NodeCount-1 ) {
					(*ch_pos)++;
				}
			}
		} else {
			(*ch_pos)++;
		}
		
		
	}
}



void update_channel( mod_channel *ch ) {
	ch->newnote = 0;
	if( mod_tick == 0 ) {

		if( ch->pflags & PF_NOTEOFF ) {
			ch->noteoff = 1;
		}
		
		if( ch->pflags & PF_NOTECUT ) {
			ch->volume  = 0;
		}
		
		//if( no_notedelay(ch) ) {
			play_note_thing(ch);
		//} else {
			
		//}
	}

	process_volume_command( ch );

	ch->tpitch = ch->pitch;
	ch->tvolume = ch->volume;
	ch->toffset = 0;

	if( ch->pflags & PF_CMD )
		process_effect( ch );

	if( !note_delayed(ch) ) {
		if( ch->sample ) {
			ch->macropos++;
			if( ch->macropos >= mod_mml.macros[ch->sample-1].length ) {
				ch->macropos = mod_mml.macros[ch->sample-1].loop;
			}
		}

		// process envelopes/auto vibrato
		if( ch->instr ) {
			IT_Instrument *ins = mod->Instruments + ch->instr - 1;
			IT_Envelope *venv = &ins->VolumeEnvelope;
			IT_Envelope *penv = &ins->PitchEnvelope;
			process_envelope( venv, &ch->vol_env_pos, &ch->vol_env_data, ch->noteoff );
			process_envelope( penv, &ch->pitch_env_pos, &ch->pitch_env_data, ch->noteoff );
			
		}
	}
}

int compute_vol( mod_channel *ch, int range, int mixing ) {
	//FV = Vol * SV * IV * CV * GV * VEV * NFC / 2^41
	
	int vol;
	vol = ch->tvolume; // 6bit
	if( ch->sample && ch->instr ) {
		vol *= mod->Samples[ch->sample-1].GlobalVolume; // +6bit
		vol *= mod->Instruments[ch->instr-1].GlobalVolume; // +7bit
		vol *= ch->volume_scale; // +6bit
		vol /= 256; // -8bit
		vol *= mod_gvol; // +7bit
		vol /= 128; // -7bit
		vol *= ch->vol_env_data; // +12bit
		vol /= 1024; // -10
		vol *= ch->fadeout; // +10bit
		vol /= 512; // -9 (=20)
		vol *= mixing;
		vol /= 100;

		vol *= range; // 
		vol /= 1048576;
		return vol;
	} else {
		return 0;
	}
}

void record_channel( channel_event *e, int channel_index ) {
	mod_channel *ch = channels + channel_index;
	
	if( ch->sample ) {
		double c5speed = mod->Samples[ch->sample -1].C5Speed;
		e->frequency = (int)floor(c5speed * pow(2.0, ((double)ch->tpitch-3840.0)/768.0) + 0.5);
	} else {
		e->frequency = 0;
	}

	e->newnote = ch->newnote;
	e->timbre = 0;
	
	if( ch->sample ) {
		switch( mod_mml.chmap[channel_index] ) {
			case CH_2A03_PULSE1:
			case CH_2A03_PULSE2:
			case CH_VRC6_PULSE1:
			case CH_VRC6_PULSE2:
				e->timbre = mod_mml.macros[ch->sample-1].nodes[ch->macropos];
				break;
			case CH_2A03_NOISE:
				if( mod_mml.macros[ch->sample-1].length == 0 ) {
					int i;
					const int noise_periods[] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};
					int best_freq=0;
					double best_error=999999;
					for( i = 0; i < 16; i++ ) {
						double real_freq = 1789773.0 / (double)noise_periods[i];
						double pitch_diff = log(real_freq/(double)e->frequency)/log(2.0);
						if( pitch_diff < 0 ) pitch_diff = -pitch_diff;
						
						if( pitch_diff < best_error ) {
							best_error = pitch_diff;
							best_freq = i;
						}
					}
					e->timbre = best_freq;
				} else {
					e->timbre = mod_mml.macros[ch->sample-1].nodes[ch->macropos];
				}
				break;
			case CH_PCM1:
			case CH_PCM2:
			case CH_PCM3:
			case CH_PCM4:
				e->timbre = reverse_pcm_map[ch->sample-1];
				e->sample_offset = ch->toffset;
				break;
		}
	}
	e->volume = compute_vol(ch,64,mod_mml.mixing[channel_index]);
	
	e->ignore = note_delayed(ch);
}

int render_frame() {
	if( mod_tick == 0 ) {
		read_pattern();
	}

	{
		int ch;
		for( ch = 0; ch < 11; ch++ ) {
			update_channel( channels + ch );
		}
	}

	{ // record data here
		frame *newframe = add_frame();
		
		int ch;
		for( ch = 0; ch < 11; ch++ ) {
			record_channel( newframe->events+ch, ch );
		}
		newframe->duration = 2500.0 / ((double)mod_tempo);
	}
	
	mod_tick++;
	if( mod_tick >= mod_speed ) {
		mod_tick = 0;
		
		// todo, position jump (end song with backward jump)
		if( mod_posjump != -1 ) {
			if( mod_posjump <= mod_position ) {
				return 0;
			}
			set_position( mod_posjump, 0 );
			mod_posjump=-1;

		} else {
			
			mod_row++;
			if( mod_row >= mod->Patterns[mod_pattern].Rows ) {
				if( mod_position +1 == mod->OrdNum ) {
					return 0; // end of song
				}
				if( set_position( mod_position+1, 0 ) ) {
					return 0;// end of song
				}
			}
		}
	}
	return 1;
}

void set_mml_macro_prim( mml_macro *m, int value ) {
	m->length = 1;
	m->nodes[0] = value;
	m->loop = 0;
}

void find_primitive_macros() {
	int i;
	for( i = 0; i < mod->SmpNum; i++ ) {
		IT_Sample *smp = mod->Samples + i;
		if( mod_mml.macros[i].length == 0 ) {
			if( strcmp( smp->DOS_Filename, "SQ_DUTY0" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 0 );
			} else if( strcmp( smp->DOS_Filename, "SQ_DUTY1" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 1 );
			} else if( strcmp( smp->DOS_Filename, "SQ_DUTY2" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 2 );
			} else if( strcmp( smp->DOS_Filename, "SQ_DUTY3" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 3 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY0" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 0 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY1" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 1 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY2" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 2 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY3" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 3 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY4" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 4 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY5" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 5 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY6" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 6 );
			} else if( strcmp( smp->DOS_Filename, "VRC6_DUTY7" ) == 0 ) {
				set_mml_macro_prim( mod_mml.macros + i, 7 );
			}
		}
	}
}

void build_sample_list() {
	int pat;
	int ins;
	int smp;
	int re;

	u8 instrument_flagged[128] = {0};
	u8 sample_flagged[128] = {0};
	for( pat = 0; pat < mod->PatNum; pat++ ) {
		int col;
		for( col = 0; col < 11; col++ ) {
			if( mod_mml.chmap[col] >= CH_PCM1 && mod_mml.chmap[col] <= CH_PCM4 ) {
				IT_Pattern *patn = mod->Patterns+pat;
				int row;
				for( row = 0; row < mod->Patterns[pat].Rows; row++ ) {
					IT_PatternEntry *e = &patn->Data[row*patn->Columns+col];
					if( e->Instrument ) {
						instrument_flagged[e->Instrument - 1] = 1;
					}
				}
			}
		}
	}

	for( ins = 0; ins < 128; ins++ ) {
		if( instrument_flagged[ins] ) {
			int i;
			for( i = 0; i < 120; i++ ) {
				int se = mod->Instruments[ins].SampleMap[i];
				if( se ) 
					sample_flagged[ se-1 ] = 1;
			}
		}
	}


	pcm_sample_count = 0;
	
	for( smp = 0; smp < 128; smp++ ) {
		if( sample_flagged[smp] ) {
			pcm_sample_count++;
		}
	}

	pcm_map = (int*)malloc( sizeof( int ) * pcm_sample_count );
	re = 0;
	
	for( smp = 0; smp < 128; smp++ ) {
		if( sample_flagged[smp] ) {
			pcm_map[re] = smp;
			reverse_pcm_map[smp] = re;
			re++;
		}
	}
}

FILE *testfile;

void t_write8( u8 data ) {
 	fwrite( &data, 1, 1, testfile );
}

void t_write16( u16 data ) {
	t_write8( data & 0xFF );
	t_write8( data >> 8 );
}

void t_write32( u32 data ) {
	t_write16( data & 0xFFFF );
	t_write16( data >> 16 );
}

void t_exportsamplebyte( IT_Sample *psamp, int position ) {
	if( psamp->Bits16 ) {
		t_write8( (((u8)((((s16*)psamp->SampleData)[position]) >> 8)^128) >> 1) );
	} else {
		t_write8( (((u8)((((s8*)psamp->SampleData)[position]))^128) >> 1) );
	}
}

int to_square_period( int freq ) {
	if( freq != 0 )
		return 1789773*2 / freq - 1;
	else
		return 0;
}

int to_triangle_period( int freq ) {
	
	if( freq != 0 )
		return 1789773 / freq - 1;
	else
		return 0;
}

int to_vrc6_period( int freq)  {
	
	if( freq != 0 )
		return 1789773*2 / freq - 1;
	else
		return 0;
}
/*
int to_vrc6saw_period( int freq)  {
	
	if( freq != 0 )
		return (1789773*2*8) / (freq*7) - 1;
	else
		return 0;
}
*/
int to_pcm_freq( int freq ) {
	const int cfsmap[] = {39+40,72+40,99+40,126+40};
	int sampling_rate = 1789773 / cfsmap[mod_mml.pcm_channels-1];
	return (freq*256 + (sampling_rate/2)) / sampling_rate ;
}

void test_export() {
	
	FILE *nsftemp;
	
	int nsf_size;
	u8 *nsf_source;
	int i;
	int sample_offsets[64];
	int sample_loop_sizes[64];
	int sample_ends[64];
	int sequencer_data_start;
	

	const char meta_string[] = 
		"SUPERNSF_TEST  "
		"."
		"3CH."
		"----";

	switch(mod_mml.pcm_channels) {
		case 1:
			nsftemp = fopen( "driver1.nsf", "rb" ); break;
		case 2:
			nsftemp = fopen( "driver2.nsf", "rb" ); break;
		case 3:
			nsftemp = fopen( "driver3.nsf", "rb" ); break;
		case 4:
			nsftemp = fopen( "driver4.nsf", "rb" ); break;
	}
	
	testfile = fopen( "testout.nsf", "wb" );
	fseek( nsftemp, 0, SEEK_END );
	nsf_size = ftell( nsftemp );
	nsf_source = (u8*)malloc( nsf_size );
	fseek( nsftemp, 0, SEEK_SET );
	fread( nsf_source, 1, nsf_size, nsftemp );
	fclose( nsftemp );
	
	if( mod_mml.vrc6 )
		nsf_source[0x7b] = 1;
	// write nsf header
	for( i = 0; i < 0x80; i++ ) {
		t_write8( nsf_source[i] );
	}

	// write update vector table

	for( i = 0x80; i < (0x180); i++ ) {
		t_write8( nsf_source[i] );
	}

	// write saturation table

	{
		double scale;
		switch( mod_mml.pcm_channels ) {
			case 1:
				scale = 0;
				break;
			case 2:
				scale = 1.0 * (double)mod_mml.pcm_mixing;
				break;
			case 3:
				scale = 1.52 * (double)mod_mml.pcm_mixing;
				break;
			case 4:
				scale = 2.0 * (double)mod_mml.pcm_mixing;
				break;
		}
		for( i = 0; i < 256; i++ ) {
			int entry = i < 128 ? i : i-256;
			
			double sample = entry*scale;
			if( sample > 63 ) sample = 63;
			if( sample < -64 ) sample = -64;
			sample += 64.0;
			t_write8( floor(sample) );
		}
	}
	
	// reserve space for sample table
	for( i = 0; i < 64+64+64+64+64+64+64+64+3+13; i++ ) {
		t_write8( 0xab );
	}

	// write meta data

	fwrite( meta_string, 1, 24, testfile );
	for( i = 0; i < 8; i++ )
		t_write8( 0x0 );

	for( i = 0x4b0; i <= 0x307f; i++ ) {
		t_write8( nsf_source[i] );
	}
	
	// write samples
	
	{
		int sample;
		for( sample = 0; sample < pcm_sample_count; sample++ ) {
			int clipped_length;
			IT_Sample *psamp = mod->Samples + pcm_map[sample];
			int unpadded_end;
			int unroll =0;
			int loop_length=0;
			int loop_iterator;
			int j;
			clipped_length = psamp->Length; 
			if( psamp->LoopEnabled ) {
				
				int testloop = psamp->LoopEnd - psamp->LoopStart;
				loop_length = psamp->LoopEnd - psamp->LoopStart;
				if( clipped_length > (int)psamp->LoopEnd )
					clipped_length = (int)psamp->LoopEnd;
				while( testloop < min_loop_size ) {
					testloop += loop_length;
					unroll++;
				}
				sample_loop_sizes[sample] = loop_length + unroll * loop_length;
			} else {
				sample_loop_sizes[sample] = min_loop_size;
			}
			sample_offsets[sample] = ftell(testfile) - 0x80;
			loop_iterator = 0;//psamp->LoopStart;

			// export sample+ unrolls
			for( i = 0; i < clipped_length; i++ ) {
				t_exportsamplebyte( psamp, i );
			}

			if( psamp->LoopEnabled ) {
				for( i = 0; i < unroll; i++ ) {
					for( j = 0; j < loop_length; j++ ) {
						t_exportsamplebyte( psamp, (psamp->LoopStart + loop_iterator) );
						loop_iterator = (loop_iterator + 1) % loop_length;
					}
				}
			} else {
				for( i = 0; i < min_loop_size; i++ ) {
					t_write8( 64 );
				}
			}
			
			unpadded_end = ftell(testfile)-0x80;
			for( i = 0; i < ((256-(unpadded_end&0xFF))&0xFF); i++ ) {
				if( psamp->LoopEnabled ) {
					t_exportsamplebyte( psamp, (psamp->LoopStart + loop_iterator) );
					loop_iterator = (loop_iterator + 1) % loop_length;
				} else {
					t_write8( 64 );
				}
			}

			sample_ends[sample] = ftell(testfile) - 0x80;
			for( i = 0; i < smp_pad_size; i++ ) {
				if( psamp->LoopEnabled ) {
					t_exportsamplebyte( psamp, (psamp->LoopStart + loop_iterator) );
					loop_iterator = (loop_iterator + 1) % loop_length;
				} else {
					t_write8( 64 );
				}
			}
		}
	}


	// write sequencer data
	{
		int page_position;
		int old_newnote[11];
		int old_timbre[11];
		int old_volume[11];
		int old_freq[11];

		
		int byte_buffer[16];
		int bbsize;
		int headerbyte;
		int convfreq;
		int dutybyte;
		int hasdata;

		int j;

		
		int max_cycles_used = 0;
		int remainder_dur =0;
		double remainder_ms = 0;

		for( i = 0; i < 11; i++ ) {
			old_newnote[i] = -1;
			old_timbre[i] = -1;
			old_volume[i] = -1;
			old_freq[i] = -1;
		}

			
		sequencer_data_start = ftell( testfile ) - 0x80;
		page_position = sequencer_data_start & 0xFF;


		for( i = 0; i < pattern_size; i++ ) {
			frame *pframe = pattern + i;
			int cycles_used = 0;

			for( j = 0; j < 11; j++ ) {
				channel_event *pe = pframe->events+j;
				bbsize=0;
				headerbyte = mod_mml.chmap[j]-1;
				if( !pe->ignore ) {
					switch( headerbyte+1 ) {
						case CH_2A03_PULSE1: // 2a03 square
						case CH_2A03_PULSE2:
							/*if( pe->newnote ) {
								headerbyte |= 1<<4;
								old_volume[j] = 0;
								old_freq[j] = -1;
								hasdata=1;
							}*/
							convfreq = to_square_period( pe->frequency );
							if( convfreq != old_freq[j] ) {
								if( (convfreq >> 8) != (old_freq[j]>>8) ) {
									byte_buffer[bbsize++] = convfreq;
									byte_buffer[bbsize++] = (convfreq>>8) | (31<<3);
								} else {
									headerbyte |= 64;
									byte_buffer[bbsize++] = convfreq;
								}
								old_freq[j] = convfreq;
								hasdata=1;
							} else {
								headerbyte |= 64|32;
							}

							dutybyte = (pe->timbre << 6) | 32|16|satmax(pe->volume>>2,15);
							if( dutybyte != old_volume[j] ) {
								old_volume[j] = dutybyte;
								headerbyte |= 128;
								byte_buffer[bbsize++] = dutybyte;
								hasdata=1;
							}

							if( hasdata ) {
								cycles_used += 3;
							}
							break;
						case CH_2A03_TRI:
							/*if( pe->newnote ) {
								headerbyte |= 1<<4;
								old_volume[j] = 0;
								hasdata=1;
							}*/
							convfreq = to_triangle_period( pe->frequency );
							if( convfreq != old_freq[j] ) {
								if( (convfreq >> 8) != (old_freq[j]>>8) ) {
									
									byte_buffer[bbsize++] = convfreq;
									byte_buffer[bbsize++] = convfreq>>8;
									cycles_used++;
								} else {
									headerbyte |= 64;
									byte_buffer[bbsize++] = convfreq;
								}
								old_freq[j] = convfreq;
								hasdata=1;
							} else {
								headerbyte |= 64|32;
							}

							headerbyte |= pe->volume ? 128:0;

							if( pe->volume != old_volume[j] ) {
								old_volume[j] = pe->volume;
								
								hasdata=1;
								
							}

							if( hasdata ) {
								cycles_used+=3;
							}
							break;
						case CH_2A03_NOISE:
							if( pe->timbre != old_timbre[j] || pe->volume != old_volume[j] ) {
								headerbyte |= satmax(pe->volume>>2,15)<<4;
								byte_buffer[bbsize++] = pe->timbre;
								hasdata=1;
								cycles_used++;
							}
							break;
						case CH_VRC6_PULSE1:
						case CH_VRC6_PULSE2:
						case CH_VRC6_SAWTOOTH:
							/*if( pe->newnote ) {
								old_volume[j] = 0;
								headerbyte |= 1<<4;
								hasdata=1;
							}*/

							//if( (headerbyte+1) == CH_VRC6_SAWTOOTH )
							//	convfreq = to_vrc6saw_period( pe->frequency );
							//else
								convfreq = to_vrc6_period( pe->frequency );

							if( convfreq != old_freq[j] ) {
								if( (convfreq >> 8) != (old_freq[j]>>8) ) {
									//headerbyte |= 64|32;
									byte_buffer[bbsize++] = convfreq;
									byte_buffer[bbsize++] = convfreq>>8;
									cycles_used++;
								} else {
									headerbyte |= 64;
									byte_buffer[bbsize++] = convfreq;
								}
								old_freq[j] = convfreq;
								hasdata=1;
							} else {
								headerbyte |= 64|32;
							}

							if( (mod_mml.chmap[j]-1) == CH_VRC6_SAWTOOTH )
								dutybyte = pe->volume * 42 / 64;
							else
								dutybyte = (pe->timbre << 4) | satmax( pe->volume >> 2, 15 );

							if( dutybyte != old_volume[j] ) {
								old_volume[j] = dutybyte;
								headerbyte |= 128;
								byte_buffer[bbsize++] = dutybyte;
								hasdata=1;
							}

							if( hasdata )
								cycles_used += 3;
							break;
							
						case CH_PCM1:
						case CH_PCM2:
						case CH_PCM3:
						case CH_PCM4:
							if( pe->newnote ) {
								old_freq[j] = 0;
								headerbyte |= 1<<4;
								byte_buffer[bbsize++] = pe->timbre | (pe->sample_offset ? 128 : 0);
								if( pe->sample_offset ) {
									byte_buffer[bbsize++] = pe->sample_offset;
									cycles_used += 1;
								}
								cycles_used+=3;
								hasdata=1;
							}

							dutybyte = satmax(pe->volume >> 1, 31);
							if( dutybyte != old_volume[j] ) {
								old_volume[j] = dutybyte;
								headerbyte |= 1<<5;
								byte_buffer[bbsize++] = (dutybyte << 7) | (dutybyte >> 1);
								hasdata=1;
							}

							convfreq = to_pcm_freq( pe->frequency );
							if( convfreq != old_freq[j] ) {
								hasdata=1;
								if( (old_freq[j] >> 8) != (convfreq >> 8) ) {
									//headerbyte |= 128|64;
									byte_buffer[bbsize++] = convfreq;
									byte_buffer[bbsize++] = convfreq>>8;
									cycles_used++;
								} else {
									headerbyte |= 128;
									byte_buffer[bbsize++] = convfreq;
								}
								old_freq[j] = convfreq;
							} else {
								headerbyte |= 128|64;
							}

							if( hasdata ) {
								cycles_used += 2;
							}
							
							break;
					}
				}

				if( hasdata ) {
					int bp;
					cycles_used++;
					if( (256 - page_position) < (bbsize+1+1) ) {
						t_write8( 11 );
						cycles_used += 2;
						page_position++;
						for( ; page_position < 256; page_position++ )
							t_write8( 13 );
						page_position = 0;
					}

					t_write8( headerbyte );
					for( bp = 0; bp < bbsize; bp++ )
						t_write8( byte_buffer[bp] );
					page_position = (page_position+1+bbsize) % 256;
					hasdata=0;
				}
			}
			
			if( page_position >= 253 ) {
				t_write8( 11 );
				cycles_used += 2;
				page_position++;
				for( ; page_position < 256; page_position++ )
					t_write8( 13 );
				page_position = 0;
			}
			
			if( i != pattern_size-1 ) {
				t_write8( 12 );
			} else {
				t_write8( 15 );
			}
			
			
			cycles_used += 2;

			//cycles_used += 4+mod_mml.pcm_channels;

			if( cycles_used > max_cycles_used ) {
				max_cycles_used = cycles_used;
			}
			page_position = (page_position+1) % 256;
			
			{
				const int cfsmap[] = {39+40,72+40,99+40,126+40};
				double ms_per_uc = (double)cfsmap[mod_mml.pcm_channels-1] * 0.00055873007359033799258341700316185;
				int real_duration = floor((pframe->duration + remainder_ms) / ms_per_uc);
				remainder_ms = fmod((pframe->duration+remainder_ms), ms_per_uc);
				real_duration -= cycles_used;
				real_duration += remainder_dur;
				remainder_dur = real_duration % (mod_mml.pcm_channels+1);
				t_write8( real_duration / (mod_mml.pcm_channels+1) ); 
			}
			page_position = (page_position+1) % 256;

			
		}


		printf( "max_cycles_used: %i\n", max_cycles_used );
	}

#define address_l(x) ((x)&0xff)
#define address_h(x) (((x)>>8)&0xf)
#define address_b(x) ((x)>>12)

	fseek( testfile, 128+256+256, SEEK_SET );
	for(i = 0; i < 64; i++)
		t_write8( address_l(sample_offsets[i]) );
	for(i = 0; i < 64; i++)
		t_write8( address_h(sample_offsets[i]) | 0xF0 );
	for(i = 0; i < 64; i++)
		t_write8( address_b(sample_offsets[i]) );
 
	for(i = 0; i < 64; i++)
		t_write8( address_h(sample_ends[i]) );
	for(i = 0; i < 64; i++)
		t_write8( address_b(sample_ends[i]) );

	for(i = 0; i < 64; i++)
		t_write8( address_l(sample_loop_sizes[i]) );
	for(i = 0; i < 64; i++)
		t_write8( address_h(sample_loop_sizes[i]) );
	for(i = 0; i < 64; i++)
		t_write8( address_b(sample_loop_sizes[i]) );

	t_write8( address_l(sequencer_data_start) );
	t_write8( address_h(sequencer_data_start) );
	t_write8( address_b(sequencer_data_start) );

	fclose(testfile);
}

void it_render( IT_File *source ) {
	int rendering = 1;
	mod = source;
	
	parse_mml_data( &mod_mml, source->Message );
	find_primitive_macros();
	build_sample_list();
	
	// setup player
	copy_initial_volume();
	set_position(0,0);
	mod_tempo = mod->InitialTempo;
	mod_speed = mod->InitialSpeed;
	mod_gvol = mod->GlobalVolume;
	
	while( rendering ) {
		rendering = render_frame();
	}
	
}

void export_render( const char *filename ) {
	int i;
	testfile = fopen( filename, "wb" );
	t_write32( mod_mml.pcm_channels );
	t_write32( pcm_sample_count );
	t_write32( mod_mml.vrc6 );
	
	for( i =0 ; i < 11; i++ )
		t_write8( mod_mml.chmap[i] );

	{
		int sample;
		for( sample = 0; sample < pcm_sample_count; sample++ ) {
			int clipped_length;
			IT_Sample *psamp = mod->Samples + pcm_map[sample];
			clipped_length = psamp->Length;
			if( psamp->LoopEnabled ) {
				clipped_length = psamp->LoopEnd;
			}
			t_write32( clipped_length );
			t_write32( psamp->LoopEnabled ? (psamp->LoopEnd - psamp->LoopStart) : 0 );
			t_write8( psamp->PingPongLoop ? 1 : 0 );

			for( i = 0; i < clipped_length; i++ ) {
				t_exportsamplebyte( psamp, i );
			}
		}
	}

	t_write32( pattern_size );
	t_write32( 11 );
	
	{
		int p;
		for( p = 0; p < pattern_size; p++ ) {
			frame *f = pattern + p;
			for( i = 0; i < 11; i++ ) {
				channel_event *ce = f->events + i;
				t_write32( ce->frequency );
				t_write8( ce->newnote );
				t_write8( ce->timbre );
				t_write8( ce->volume );
				t_write8( ce->sample_offset );
			}
			t_write32( (int)(f->duration*1000) );
		}
	}
	
	fclose(testfile);
}
