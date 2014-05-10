
#include "it_file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FILE *myfile;

u8 read8(){
	u8 a[1];
	fread( a, 1, 1, myfile );
	return a[0];
}

u16 read16() {
	u16 a;
	a = read8();
	return a | (read8()<<8);
}

u32 read32() {
	u32 a;
	a = read16();
	return a | (read16()<<16);
}

static void LoadPattern( IT_Pattern *pat ) {
	int row;
	int length = read16();

	u8 prev_note[64];
	u8 prev_ins[64];
	u8 prev_vol[64];
	u8 prev_cmd[64];
	u8 prev_param[64];
	u8 prev_mask[64];

	pat->Rows = read16();
	read32();
	pat->Columns = 11;

	pat->Data = (IT_PatternEntry*)malloc( sizeof(IT_PatternEntry) * pat->Rows * pat->Columns );

	for( row = 0; row < pat->Rows; row++ ) {
		int ch;
		for( ch = 0; ch < pat->Columns; ch++ ) {
			IT_PatternEntry *pe = pat->Data + row*pat->Columns + ch;
			pe->Note = 200;
			pe->Instrument = 0;
			pe->SubCommand = 255;
			pe->Command = 0;
			pe->Param = 0;
		}
	}

	for( row = 0; row < pat->Rows; ) {
		u8 chvar = read8();
		int ch = (chvar-1) & 63;
		int maskvar;
		if( chvar == 0 ) {
			row++;
			continue;
		}
		maskvar = (chvar&128) ? read8() : prev_mask[ch];
		prev_mask[ch] = maskvar;

		if( ch < pat->Columns ) {
			IT_PatternEntry *pe = pat->Data + row*pat->Columns + ch;
			
			if( maskvar&1 ) {
				pe->Note = read8();
				prev_note[ch] = pe->Note;
			}
			if( maskvar&2 ) {
				pe->Instrument = read8();
				prev_ins[ch] = pe->Instrument;
			}
			if( maskvar&4 ) {
				pe->SubCommand = read8();
				prev_vol[ch] = pe->SubCommand;
			}
			if( maskvar&8 ) {
				pe->Command = read8();
				prev_cmd[ch] = pe->Command;
				pe->Param = read8();
				prev_param[ch] = pe->Param;
			}
			if( maskvar&16 ) {
				pe->Note = prev_note[ch];
			}
			if( maskvar&32 ) {
				pe->Instrument = prev_ins[ch];
			}
			if( maskvar&64 ) {
				pe->SubCommand = prev_vol[ch];
			}
			if( maskvar&128 ) {
				pe->Command = prev_cmd[ch];
				pe->Param = prev_param[ch];
			}
		} else {
			if( maskvar&1) read8();
			if( maskvar&2) read8();
			if( maskvar&4) read8();
			if( maskvar&8) read16();
		}
	}
}

static void LoadSample( IT_Sample *samp ) {
	int i;
	u8 HasSample;
	u8 StereoSample;
	u8 CompressedSample;
	u32 samplePointer;
	read32();
	for( i = 0; i < 12; i++ ) {
		samp->DOS_Filename[i] = read8();
	}
	read8();
	samp->GlobalVolume = read8();
	samp->Flg = read8();
	HasSample = !!(samp->Flg&1);
	samp->Bits16 = !!(samp->Flg&2);
	StereoSample = !!(samp->Flg&4);
	CompressedSample = !!(samp->Flg&8);
	samp->LoopEnabled = !!(samp->Flg&16);
	samp->SusLoopEnabled = !!(samp->Flg&32);
	samp->PingPongLoop = !!(samp->Flg&64);
	samp->PingPongSusLoop = !!(samp->Flg&128);
	samp->DefaultVolume = read8();
	for( i = 0; i < 26; i++ )
		samp->SampleName[i] = read8();
	samp->Cvt = read8();
	samp->DefaultPanning = read8();
	samp->Length = read32();
	samp->LoopStart = read32();
	samp->LoopEnd = read32();
	samp->C5Speed = read32();
	samp->SusLoopStart = read32();
	samp->SusLoopEnd = read32();
	samplePointer = read32();

	samp->VibratoSpeed = read8();
	samp->VibratoDepth = read8();
	samp->VibratoRate = read8();
	samp->VibratoType = read8();

	if( HasSample ) {
		fseek( myfile, samplePointer, SEEK_SET );
		
		if( CompressedSample ) {
			// todo: compressed sample load
		} else {
			if( samp->Bits16 ) {
				samp->SampleData = (s16*)malloc(2 * samp->Length);
				for( i = 0; i < (int)samp->Length; i++ ) {
					if( samp->Cvt & 1 ) {
						// signed
						((s16*)samp->SampleData)[i] = (s16)read16();
					} else {
						// unsigned
						((s16*)samp->SampleData)[i] = (s16)(read16() ^ 0x8000);
					}
				}
			} else {
				samp->SampleData = (s8*)malloc(samp->Length);
				for( i = 0; i < (int)samp->Length; i++ ) {
					if( samp->Cvt & 1 ) {
						// signed
						((s8*)samp->SampleData)[i] = (s8)read8();
					} else {
						// unsigned
						((s8*)samp->SampleData)[i] = (s8)(read8() ^ 0x80);
					}
				}
			}
		}
	}
}

static void LoadEnvelope( IT_Envelope *env ) {
	int i;
	u8 flags = read8();
	env->Enabled = !!(flags&1);
	env->LoopEnabled = !!(flags&2);
	env->SusLoopEnabled = !!(flags&4);
	env->FilterEnvelope = !!(flags&128);
	env->NodeCount = read8();
	env->LoopStart = read8();
	env->LoopEnd = read8();
	env->SusLoopStart = read8();
	env->SusLoopEnd = read8();
	for( i = 0; i < 25; i++ ) {
		env->NodeY[i] = read8();
		env->NodeX[i] = read16();
	}
	read8();
}

static void LoadInstrument( IT_Instrument *ins ) {
	int i;
	read32();
	for( i = 0; i < 12; i++ )
		ins->DOS_Filename[i] = read8();
	read8();
	ins->NewNoteAction = read8();
	ins->DuplicateCheckType = read8();
	ins->DuplicateCheckAction = read8();
	ins->Fadeout = read16();
	ins->PitchPanSeparation = read8();
	ins->PitchPanCenter = read8();
	ins->GlobalVolume = read8();
	ins->DefaultPanning = read8();
	ins->RandomVolume = read8();
	ins->RandomPanning = read8();
	ins->TrackerVersion = read16();
	ins->NumberOfSamples = read8();
	read8();
	for( i = 0; i < 26; i++ ) {
		ins->InstrumentName[i] = read8();
	}
	ins->InitialFilterCutoff = read8();
	ins->InitialFilterResonance = read8();
	ins->MIDI_Channel = read8();
	ins->MIDI_Program = read8();
	ins->MIDI_Bank = read16();
	for( i = 0; i < 120; i++ ) {
		ins->NoteMap[i] = read8();
		ins->SampleMap[i] = read8();
	}
	LoadEnvelope( &ins->VolumeEnvelope );
	LoadEnvelope( &ins->PanningEnvelope );
	LoadEnvelope( &ins->PitchEnvelope );
}

IT_File* ITFile_Load( char *filename ) {

	IT_File *itf;
	int i;

	u32 messageOffset;

	u32 *instrumentOffsets;
	u32 *sampleOffsets;
	u32 *patternOffsets;

	myfile = fopen( filename, "rb" );

	itf = (IT_File*)malloc(sizeof(IT_File));
	memset( itf, 0, sizeof( IT_File ) );

	read32(); // IMPM
	fread( itf->SongName, 1, 26, myfile );
	itf->PatternHighlight = read16();
	itf->OrdNum = read16();
	itf->InsNum = read16();
	itf->SmpNum = read16();
	itf->PatNum = read16();
	itf->Cwtv = read16();
	itf->Cmwt = read16();
	itf->Flags = read16();
	itf->Special = read16();
	itf->GlobalVolume = read8();
	itf->MasterVolume = read8();
	itf->InitialSpeed = read8();
	itf->InitialTempo = read8();
	itf->PanningSeparation = read8();
	itf->PitchWheelDepth = read8();
	itf->MessageLength = read16();
	messageOffset = read32();
	read32(); // reserved
	for( i = 0; i < 64; i++ )
		itf->InitialChannelPanning[i] = read8();
	for( i = 0; i < 64; i++ )
		itf->InitialChannelVolume[i] = read8();
	itf->OrderList = (u8*)malloc( itf->OrdNum );
	for( i = 0; i < itf->OrdNum; i++ )
		itf->OrderList[i] = read8();
	
	instrumentOffsets = (u32*)malloc( itf->InsNum * 4 );
	sampleOffsets = (u32*)malloc( itf->SmpNum * 4 );
	patternOffsets = (u32*)malloc( itf->PatNum * 4 );

	for( i = 0; i < itf->InsNum; i++ )
		instrumentOffsets[i] = read32();
	for( i = 0; i < itf->SmpNum; i++ )
		sampleOffsets[i] = read32();
	for( i = 0; i < itf->PatNum; i++ )
		patternOffsets[i] = read32();
	
	// load instruments
	itf->Instruments = (IT_Instrument*)malloc( sizeof(IT_Instrument) * itf->InsNum );
	for( i = 0; i < itf->InsNum; i++ ) {
		memset( itf->Instruments+i,0,sizeof( IT_Instrument ) ); 
		if( instrumentOffsets[i] ) {
			fseek( myfile, instrumentOffsets[i], SEEK_SET );
			LoadInstrument( itf->Instruments+i );
		}
	}

	// load samples
	itf->Samples = (IT_Sample*)malloc( sizeof(IT_Sample) * itf->SmpNum );
	for( i = 0; i < itf->SmpNum; i++ ) {
		memset( itf->Samples+i, 0, sizeof( IT_Sample ) );
		if( sampleOffsets[i] ) {
			fseek( myfile, sampleOffsets[i], SEEK_SET );
			LoadSample( itf->Samples + i );
		}
	}

	// load patterns
	itf->Patterns = (IT_Pattern*)malloc( sizeof(IT_Pattern) * itf->PatNum );
	for( i = 0; i < itf->PatNum; i++ ) {
		memset( itf->Patterns+i, 0, sizeof( IT_Pattern ) );
		if( patternOffsets[i] ) {
			fseek( myfile, patternOffsets[i], SEEK_SET );
			LoadPattern( itf->Patterns + i );
		}
	}

	// read message
	if( itf->MessageLength ) {
		fseek( myfile, messageOffset, SEEK_SET );
		itf->Message = malloc( itf->MessageLength+1 );
		itf->Message[itf->MessageLength] = 0;
		fread( itf->Message, 1, itf->MessageLength, myfile );
	}

	return itf;
}

void ITFile_Delete( IT_File *mod ) {

	// todo: free stuff lol

	free( mod->Instruments );
	free( mod->Patterns );
	free( mod->Samples );
	free( mod->OrderList );
	free( mod );
}