#include <stdlib.h>
#include <stdio.h>

#include "it_file.h"


extern void it_render( IT_File *source );

int main( int argc, char *argv[] ) {
	
	IT_File *my_it;
	char *output_bin;
	if( argc < 3 ) {
		printf( "USAGE: it_player.exe <input.it> <output.bin>\n" );
		return;
	}

	output_bin = argv[2];

	my_it = ITFile_Load( argv[1] );

	it_render( my_it );

	if( output_bin[0] == 'p' ) {
		if( output_bin[1] == 'o' ) {
			if( output_bin[2] == 'o' ) {
				if( output_bin[3] == 'd' ) {
					test_export();
				}
			}
		}
	}

	export_render( output_bin );

	ITFile_Delete( my_it );
	return 0;
}
