
;*********************************************************
;* SuperNSF(tm) Audio Driver
;* (C) 2010 Mukunda Johnson
;*********************************************************

;* insert retarded software license here *

.include "settings.inc"

;-----------------------------------
;// memory bank layout:
;// 8000-8fff = pcm1 window (4k) 
;// 9000-9fff = pcm2 window (4k)
;// a000-afff = pcm3 window (4k)
;// b000-bfff = pcm4 window (4k)
;// c000-cfff = pattern data window (4k)
;// d000-dfff = volume table
;// e000-feff = program code (8k static)
;// ff00-ffff = top page
;-----------------------------------

.zeropage

	data1:	.res 1
	data2:	.res 1
	data3:	.res 1
	data4:	.res 1
	data5:	.res 1
	data6:	.res 1

.code

;=============================================================================================
.macro copy_memory_with_banks_89 source, dest, size
;=============================================================================================
	.local @loop1
	lda	#0
	sta	data1
	lda	#((>source) & $f) | $80
	sta	data2
	
	ldy	#(>source >> 4) - $e
	sty	$5ff8
	iny
	sty	$5ff9
	
	lda	#<dest
	sta	data3
	lda	#>dest
	sta	data4
	ldy	#<source
	ldx	#0
	
	
	lda	#<size
	sta	data5
	lda	#>size
	sta	data6
	
	sec
	
@loop1:
	lda	(data1),y
	sta	(data3,x)
	iny
	bne	:+
	inc	data2
:	inc	data3
	bne	:+
	inc	data4
:	lda	data5
	sbc	#1
	sta	data5
	lda	data6
	sbc	#0
	sta	data6
	ora	data5
	bne	@loop1
.endmacro

.import __ZPCODE_LOAD__, __ZPCODE_RUN__, __ZPCODE_SIZE__
.import __RAMCODE_LOAD__, __RAMCODE_RUN__, __RAMCODE_SIZE__

;===============================================================================
__init:
;===============================================================================
	
	lda	#0				; clear zeropage
	ldx	#0				;
:	sta	<0, x				;
	inx					;		
	bne	:-				;
	
;-------------------------------------------------------------------------------	
						; copy data regions
	copy_memory_with_banks_89 __ZPCODE_LOAD__, __ZPCODE_RUN__, __ZPCODE_SIZE__ 
	copy_memory_with_banks_89 __RAMCODE_LOAD__, __RAMCODE_RUN__, __RAMCODE_SIZE__
;-------------------------------------------------------------------------------
	lda	sequencer_reset			; reset pattern position
	sta	pattern_L			;
	lda	sequencer_reset+1		;
	ora	#$c0
	sta	pattern+1			;
	lda	sequencer_reset+2		;
	sta	pattern+2			;
	sta	$5ffc
;-------------------------------------------------------------------------------
	lda	#<UV_PCM1_TESTEND		; init update vector
	sta	update_vector			;
;-------------------------------------------------------------------------------
	lda	#1				; reset sequencer timer (to instant)
	sta	sequencer_timer			;

	lda	#1
.repeat CHCOUNT, i
	sta	pcm_loop_h(i)
.endrep
;-------------------------------------------------------------------------------
;xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	
;	lda	sample_addresses_l
;	sta	pcm_addr(0)
;	lda	sample_addresses_h
;	ora	#0a0h
;	sta	pcm_page(0)
;	lda	sample_addresses_b
;	sta	pcm_bank(0)
;	sta	$5ffa
	
;	lda	#<(volume_table_start+31*64)
;	sta	pcm_volmap_l(0)
;	lda	#>(volume_table_start+31*64)
;	sta	pcm_volmap_h(0)
;	lda	#99
;	sta	pcm_rate_l(0)
;	lda	#1
;	sta	pcm_rate_h(0)
;	lda	#0
;	sta	pcm_sample(0)
	
;	lda	sample_loop_sizes_l
;	sta	pcm_loop_l(0)
;	lda	sample_loop_sizes_h
;	sta	pcm_loop_h(0)
;	lda	sample_loop_sizes_b
;	sta	pcm_loop_b(0)
;xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
;-------------------------------------------------------------------------------
	lda	#15
	sta	$4015
	
	lda	#$08
	sta	$4001
	sta	$4005
	
	lda	#3<<4
	sta	$400C
	lda	#31<<3
	sta	$400F
	
	lda	#2
	sta	$5ffd
	
	rts
__play:
	
	jmp	pcm_mix_next
	
