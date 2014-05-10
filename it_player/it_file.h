#ifndef ITFILE_H
#define ITFILE_H

#include "eztypes.h"

typedef struct t_IT_Envelope {
	u8		Enabled;
	u8		LoopEnabled;
	u8		SusLoopEnabled;
	u8		FilterEnvelope;
	u8		NodeCount;
	u8		LoopStart;
	u8		LoopEnd;
	u8		SusLoopStart;
	u8		SusLoopEnd;
	u16		NodeX[25];
	u8		NodeY[25];
} IT_Envelope;

typedef struct t_IT_Instrument {
	char	DOS_Filename[13];
	u8		NewNoteAction;
	u8		DuplicateCheckType;
	u8		DuplicateCheckAction;
	u16		Fadeout;
	u8		PitchPanSeparation;
	u8		PitchPanCenter;
	u8		GlobalVolume;
	u8		DefaultPanning;
	u8		RandomVolume;
	u8		RandomPanning;
	u16		TrackerVersion;
	u8		NumberOfSamples;
	char	InstrumentName[26+1];
	u8		InitialFilterCutoff;
	u8		InitialFilterResonance;
	u8		MIDI_Channel;
	u8		MIDI_Program;
	u16		MIDI_Bank;
	u8		NoteMap[120];
	u8		SampleMap[120];

	IT_Envelope VolumeEnvelope;
	IT_Envelope PanningEnvelope;
	IT_Envelope PitchEnvelope;
} IT_Instrument;

typedef struct t_IT_Sample {
	char	DOS_Filename[13];
	u8		GlobalVolume;
	u8		Flg;
	u8		Bits16;
	u8		CompressedSample;
	u8		LoopEnabled;
	u8		SusLoopEnabled;
	u8		PingPongLoop;
	u8		PingPongSusLoop;
	u8		DefaultVolume;
	char	SampleName[26+1];
	u8		Cvt;
	u8		DefaultPanning;
	u32		Length;
	u32		LoopStart;
	u32		LoopEnd;
	u32		C5Speed;
	u32		SusLoopStart;
	u32		SusLoopEnd;

	u8		VibratoSpeed;
	u8		VibratoDepth;
	u8		VibratoRate;
	u8		VibratoType;
	
	void	*SampleData;
} IT_Sample;

typedef struct t_IT_PatternEntry {
	u8	Note;
	u8	Instrument;
	u8	SubCommand;
	u8	Command;
	u8	Param;
} IT_PatternEntry;

typedef struct t_IT_Pattern {
	u16		Rows;
	u16		Columns;
	IT_PatternEntry *Data;
} IT_Pattern;

typedef struct t_IT_Header {
	char	SongName[27];
	u16		PatternHighlight;
	u16		OrdNum;
	u16		InsNum;
	u16		SmpNum;
	u16		PatNum;
	u16		Cwtv;
	u16		Cmwt;
	u16		Flags;
	u16		Special;
	u8		GlobalVolume;
	u8		MasterVolume;
	u8		InitialSpeed;
	u8		InitialTempo;
	u8		PanningSeparation;
	u8		PitchWheelDepth;
	char	*Message;
	u32		MessageLength;
	u8		InitialChannelPanning[64];
	u8		InitialChannelVolume[64];
	u8		*OrderList;

	IT_Instrument *Instruments;
	IT_Sample *Samples;
	IT_Pattern *Patterns;
} IT_File;

IT_File * ITFile_Load( char *filename );
void ITFile_Delete( IT_File* );


#endif
