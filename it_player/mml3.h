#ifndef MML3_H
#define MML3_H

#include "eztypes.h"

typedef struct {
	u8 nodes[64];
	int length;
	int loop;
} mml_macro;

typedef struct {
	mml_macro macros[64];
	int chmap[11];
	int mixing[11];
	double pcm_mixing;
	int vrc6;
	int pcm_channels;
} mml_data;

typedef enum {
	CH_UNMAPPED,
	CH_2A03_PULSE1,
	CH_2A03_PULSE2,
	CH_2A03_TRI,
	CH_2A03_NOISE,
	CH_VRC6_PULSE1,
	CH_VRC6_PULSE2,
	CH_VRC6_SAWTOOTH,
	CH_PCM1,
	CH_PCM2,
	CH_PCM3,
	CH_PCM4
} mml_channel;

void parse_mml_data( mml_data *dest, const char *input_mml );

#endif
