
.import __init
.import __play

;=================.
.segment "HEADER" ;
;=================================================================================
  .byte		"NESM", 1Ah			; marker			//
  .byte		01h				; version			//
  .byte		01h				; number of songs		//
  .byte		01h				; starting song			//
  .word		$8000				; load address			//
  .word		__init				; init address			//
  .word		__play				; play address			//
  .byte		"supernsf"			; song name			//
  .res		32-8, 0				;				//
  .res		32, 0				; artist			//
  .res		32, 0				; copyright			//
  .word		0411Ah				; NTSC speed (60hz)		//
  .byte		0, 0, 0, 0, 0, 0, 0, 1		; bankswitch init values	//
  .word		0411Ah				; PAL speed (60hz !)		//
  .byte		0				; PAL/NTSC bits			//
  .byte		0				; EXT chip support		//
  .byte		0, 0, 0, 0			; expansion bytes		//
;=================================================================================
