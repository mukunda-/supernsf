;
; sequencer
;

.include "settings.inc"

.zeropage
	sequencer_timer:	.res	1
	pattern:		.res	3	; pattern address
	pattern_L:		.res	1	; 
	event_header:		.res	1	;
	
	tri_freq_l:		.res	1
	tri_freq_h:		.res	1
	tri_sweep:		.res	1
	
	vrc6_freq_l:		.res	3
	vrc6_freq_h:		.res	3
	vrc6_sweep:		.res	3
	
	pcm_sweep:		.res	4
.code

;===============================================================================
sequencer_process_bf:
;===============================================================================
	dec	sequencer_timer		; 5
	beq	@process_frame		; 2/3
	uret	UV_PCM1_TESTEND, 7
;-------------------------------------------------------------------------------
@process_frame:
;-------------------------------------------------------------------------------
	ldy	pattern_L
	uret	UV_PROCESS_FRAME_EVENT, 11
	
;===============================================================================
reserved_sequencer_channel:
;===============================================================================
	jmp	reserved_sequencer_channel
	; lockup system
	
;===============================================================================
sequencer_cross_page:
;===============================================================================

	ldy	#0			; increment page/bank
	inc	pattern+1		;
	lda	pattern+1		;
	cmp	#$d0			;
	beq	@crossed_bank		;
	uret	UV_PROCESS_FRAME_EVENT, 14
;-------------------------------------------------------------------------------
@crossed_bank:
	lda	#$c0			;
	sta	pattern+1		;
	inc	pattern+2		;		
	lda	pattern+2		;
	sta	$5ffc			;
	uret	UV_PROCESS_FRAME_EVENT, 32
	
;===============================================================================
sequencer_end_frame:
;===============================================================================

	lda	(pattern), y		; read duration
	iny				;
	;lda	#255	;; DEBUG
	sty	pattern_L		; save pattern position
	sta	sequencer_timer		;
	uret	UV_PCM1_TESTEND, 13

;===============================================================================
sequencer_end_track:
;===============================================================================

	lda	(pattern), y		; copy duration
	sta	sequencer_timer		;
		
	lda	sequencer_reset		; reset sequence position
	sta	pattern_L		;
	lda	sequencer_reset+1	;
	ora	#$c0
	sta	pattern+1		;
	lda	sequencer_reset+2	;
	sta	pattern+2		;
	sta	$5ffc
	uret	UV_PCM1_TESTEND, 29

;===============================================================================
process_frame_event:
;===============================================================================
	lda	(pattern), y		; 5 
	iny				; 2
	sta	event_header		; 3
	and	#15			; 2
	asl				; 2
	sta	update_vector		; 3
	jmp	update_complete-((CYCLES_FOR_UPDATE-20)/2) ;haxxd return
	
;===============================================================================
.macro sqr_update_routines scopename, UVEC, rofs
;===============================================================================
.scope scopename
;===============================================================================
proc1: ; UVEC+0
;===============================================================================
	lda	event_header		; help proc2
	and	#3<<5			;
	sta	data1			;
	
	lda	event_header		; test/reset sweep
	and	#1<<4			;
	beq	@no_w_bit		;
	lda	#08h			;
	sta	$4001			;
	uret	UVEC+2, 21		;
@no_w_bit:
	uret	UVEC+2, 16
	
;===============================================================================
proc2: ; UVEC+2
;===============================================================================
	lda	data1			; 3 test function type
	beq	@full_period		; 2/3
	cmp	#2<<5			; 2
	beq	@lower_period		; 2/3
	bcc	@write_sweep		; 2/3
	uret	UVEC+4, 11
;-------------------------------------------------------------------------------
@full_period:				;
	lda	(pattern), y		; 5 copy period
	iny				; 2
	sta	$4002+rofs		; 4
	lda	(pattern), y		; 5
	iny				; 2
	sta	$4003+rofs		; 4
	uret	UVEC+4, 28
;-------------------------------------------------------------------------------
@lower_period:				; copy lower period
	lda	(pattern), y		; 5
	iny				; 2
	sta	$4002+rofs		; 4
	uret	UVEC+4, 21
;-------------------------------------------------------------------------------
@write_sweep:
	lda	(pattern), y		; copy sweep value
	iny				;
	sta	$4001+rofs		;
	uret	UVEC+4, 23

;===============================================================================
proc3: ; UVEC+4
;===============================================================================
	lda	event_header		; test volume bit		
	and	#1<<7			;
	beq	@no_change		;
;-------------------------------------------------------------------------------
	lda	(pattern), y		; copy volume/duty/envelope
	iny				;
	sta	$4000+rofs		;
	uret	UV_PROCESS_FRAME_EVENT, 18
;-------------------------------------------------------------------------------
@no_change:				
	uret	UV_PROCESS_FRAME_EVENT, 8
	
.endscope
.endmacro

;===============================================================================
triangle_proc1:
;===============================================================================
	lda	event_header		; help proc2
	and	#3<<5
	sta	data1
	
	lda	event_header		; reset sweep if prep bit is set
	and	#1<<4			;
	beq	@no_reset		;
	lda	#0			;
	sta	tri_sweep		;
	uret	UV_TRIANGLE_PROC2, 20	;
@no_reset:				;
	uret	UV_TRIANGLE_PROC2, 16	;

;===============================================================================
triangle_proc2:
;===============================================================================
	lda	data1			; 3 test function type
	beq	@full_period		; 2/3
	cmp	#2<<5			; 2
	beq	@lower_period		; 2/3
	bcc	@write_sweep		; 2/3
	uret	UV_TRIANGLE_PROC3, 11
;-------------------------------------------------------------------------------
@write_sweep:				; copy sweep value
	lda	(pattern), y		;
	iny				;
	sta	tri_sweep		;
	uret	UV_TRIANGLE_PROC3, 22	;
;-------------------------------------------------------------------------------
@full_period:				;
	
	lda	(pattern), y		; copy period
	iny				;
	sta	tri_freq_l		;
	lda	(pattern), y		;
	iny				;
	sta	tri_freq_h		;
	
	uret	UV_TRIANGLE_COPYFREQ, 26
;-------------------------------------------------------------------------------
@lower_period:				;
	
	lda	(pattern), y		; copy period
	iny				;
	sta	tri_freq_l		;
	sta	$400A			;
	uret	UV_TRIANGLE_PROC3, 24	;
	
;===============================================================================
triangle_copyfreq:
;===============================================================================
	ldx	tri_freq_l		; copy period
	lda	tri_freq_h		;
	ora	#31<<3
	sta	$400B			;
	stx	$400A			;
	uret	UV_TRIANGLE_PROC3, 16	;

;===============================================================================
triangle_proc3:
;===============================================================================
	bit	event_header		; copy enable status
	bmi	@enable_sound		;
	lda	#0			;
	sta	$4008			;
	uret	UV_PROCESS_FRAME_EVENT, 11
@enable_sound:				;
	lda	#128+20			;
	sta	$4008			;
	uret	UV_PROCESS_FRAME_EVENT, 12
	
;===============================================================================
triangle_process_sweep:
;===============================================================================
	clc
	lda	tri_sweep		; get sweep speed & test direction
	bmi	@negative_sweep		;
;-------------------------------------------------------------------------------
	adc	tri_freq_l		; sweep upwards
	sta	tri_freq_l		;		
	sta	$400A			;
	lda	tri_freq_h		;
	adc	#0			;
	sta	$400B			;
	sta	tri_freq_h		;
	
.if VRC6 <> 0
	uret	UV_VRC61_SWEEP, 29	;
.else
	uret	UV_PCM1_SWEEP, 29
.endif
;-------------------------------------------------------------------------------
@negative_sweep:			; sweep downward
	adc	tri_freq_l		;
	sta	tri_freq_l		;
	sta	$400A			;
	lda	tri_freq_h		;
	adc	#$ff			;
	sta	$400B			;
	sta	tri_freq_h		;
	
	
.if VRC6 <> 0
	uret	UV_VRC61_SWEEP, 30
.else
	uret	UV_PCM1_SWEEP, 30
.endif

;===============================================================================
noise_proc1:
;===============================================================================
	lda	event_header		; copy volume	
	lsr				;
	lsr				;
	lsr				;
	lsr				;
	ora	#3<<4			;
	sta	$400C			;
	lda	(pattern), y		; copy period
	iny				;
	sta	$400E			;
	uret	UV_PROCESS_FRAME_EVENT, 28
	
.define vrcreg(channel,index) $9000+channel*$1000+index
	
.macro vrc6_routines scopename, UVBASE, chan, sweep_link
.scope scopename
;===============================================================================
proc1: ;UVBASE+0
;===============================================================================
	lda	event_header		; sweep reset
	and	#1<<4			;
	beq	@no_prep		;
	lda	#0			;
	sta	vrc6_sweep+chan		;
	uret	UVBASE+2, 12		;
@no_prep:				;
	uret	UVBASE+2, 8		;

;===============================================================================
proc2: ;UVBASE+2
;===============================================================================
	lda	event_header		; 3 test function type
	and	#3<<5			; 2
	beq	@full_period		; 2/3
	cmp	#2<<5			; 2
	beq	@lower_period		; 2/3
	bcc	@write_sweep		; 2/3
	uret	UVBASE+4, 13
;-------------------------------------------------------------------------------
@full_period:
	lda	(pattern), y		; copy period
	iny				;
	sta	vrc6_freq_l+chan	;
	lda	(pattern), y		;
	iny				;
	sta	vrc6_freq_h+chan	;
	
	uret	UVBASE+6, 28
;-------------------------------------------------------------------------------
@write_sweep:				; copy sweep value

	lda	(pattern), y		;
	iny				;
	sta	vrc6_sweep+chan		;
	uret	UVBASE+4, 24		;
;-------------------------------------------------------------------------------
@lower_period:				; copy lower freq
	
	lda	(pattern), y		;
	iny				;
	sta	vrc6_freq_l+chan	;
	sta	vrcreg(chan,1)		;
	uret	UVBASE+4, 26		;

;===============================================================================
proc3: ;UVBASE+4
;===============================================================================
	lda	event_header		; copy volume
	and	#1<<7			;
	beq	@no_volume		;
	lda	(pattern), y		;
	iny				;
	sta	vrcreg(chan,0)		;
	uret	UV_PROCESS_FRAME_EVENT, 18
@no_volume:				;
	uret	UV_PROCESS_FRAME_EVENT, 8


;===============================================================================
copyfreq: ;UVBASE+6
;===============================================================================
	ldx	vrc6_freq_l+chan
	lda	vrc6_freq_h+chan
	ora	#80h
	sta	vrcreg(chan,2)
	stx	vrcreg(chan,1)
	uret	UVBASE+4, 16
	
;===============================================================================
process_sweep:
;===============================================================================
	clc
	
	lda	vrc6_sweep+chan		; get sweep speed & test direction
	bmi	@negative_sweep		;
;-------------------------------------------------------------------------------
	adc	vrc6_freq_l+chan	; sweep upwards
	sta	vrc6_freq_l+chan	;
	sta	vrcreg(chan,1)		;
	lda	vrc6_freq_h+chan	;
	adc	#0			;
	sta	vrc6_freq_h+chan	;
	ora	#80h
	sta	vrcreg(chan,2)		;
	uret	sweep_link, 31		;
;-------------------------------------------------------------------------------
@negative_sweep:			; sweep downward
	adc	vrc6_freq_l+chan	;
	sta	vrc6_freq_l+chan	;
	sta	vrcreg(chan,1)		;
	lda	vrc6_freq_h+chan	;
	adc	#$ff			;
	sta	vrc6_freq_h+chan	;
	ora	#80h
	sta	vrcreg(chan,2)		;
	uret	sweep_link, 32
	
.endscope
.endmacro

.macro pcm_routines UVEC, scopename, idx, sweep_link
.scope scopename
;===============================================================================
proc1: 		;	; +0
;===============================================================================
	lda	event_header			; test PREP
	and	#1<<4				;
	beq	@noprep				;
;-------------------------------------------------------------------------------
	lda	#0				; rate=0
	sta	pcm_rate_l(idx)			;
	sta	pcm_rate_h(idx)			;
;-------------------------------------------------------------------------------
	lda	(pattern), y			; get sample index
	bmi	@has_sample_offset		; test MSB for sample offset
	iny					;
;-------------------------------------------------------------------------------
	sta	pcm_sample(idx)			; save sample index
	uret	UVEC+8, 29
;-------------------------------------------------------------------------------
@noprep:					;
	uret	UVEC+10, 8			;
;-------------------------------------------------------------------------------
@has_sample_offset:
	iny					; mask out sample offset bit
	eor	#128				;
	sta	pcm_sample(idx)			;
	uret	UVEC+2, 32
	
;===============================================================================
sampleoffset:		; +2
;===============================================================================
	ldx	pcm_sample(idx)			; copy sample address L
	lda	sample_addresses_l, x		;
	sta	pcm_addr(idx)			;
	lda	(pattern), y			; compute SA.H + OFFSET.L
	and	#$0f				;
	clc					;
	adc	sample_addresses_h, x		;
	and	#$0f				;
	ora	#$80+idx*$10			;
	sta	pcm_page(idx)			;
	php
	;ror	data1				; <save carry bit>
	uret	UVEC+4, (33-1)			; compensate +1
	
;===============================================================================
sampleoffset2:		; +4
;===============================================================================
	ldx	pcm_sample(idx)			; compute SA.B + OFFSET.H
	lda	(pattern),y			;
	iny					;
	lsr					;
	lsr					;
	lsr					;
	lsr					;
	plp					; <restore carry bit>
	adc	sample_addresses_b, x		;
	sta	pcm_bank(idx)			;
	sta	$5ff8+idx			;
	uret	UVEC+6, (33-1)			; compensate +1

;===============================================================================
sampleoffset3:		; +6
;===============================================================================
	ldx	pcm_sample(idx)			; copy loop data
	lda	sample_loop_sizes_l, x		;
	sta	pcm_loop_l(idx)			;
	lda	sample_loop_sizes_h, x		;
	ora	#$80+idx*$10
	sta	pcm_loop_h(idx)			;
	lda	sample_loop_sizes_b, x		;
	sta	pcm_loop_b(idx)			;
	uret	UVEC+12, (26+2)			; compensate -2
	
;===============================================================================
copysample: 		; +8
;===============================================================================
	ldx	pcm_sample(idx)			; copy sample info
	lda	sample_addresses_l, x		;
	sta	pcm_addr(idx)			;
	lda	sample_addresses_h, x		; copy sample address H
	eor	#($80 + idx*$10)^$F0		;
	sta	pcm_page(idx)			;
	lda	sample_addresses_b, x		;
	sta	pcm_bank(idx)			;
	sta	$5ff8+idx			;
	uret	UVEC+10, 30
	
;===============================================================================
copysample2:		; +10
;===============================================================================
	ldx	pcm_sample(idx)			; cache loop size
	lda	sample_loop_sizes_l, x		; 
	sta	pcm_loop_l(idx)			;
	lda	sample_loop_sizes_h, x		;
	ora	#$80+idx*$10
	sta	pcm_loop_h(idx)			;
	lda	sample_loop_sizes_b, x		;
	sta	pcm_loop_b(idx)			;
	uret	UVEC+12, 26
	
;===============================================================================
proc2: 			; +12
;===============================================================================
	lda	event_header			; copy volume setting
	and	#1<<5				;
	beq	@no_volume			;
	lda	(pattern), y			;
	iny					;	
	sta	pcm_volmap_h(idx)		;
	and	#%10000000			;
	sta	pcm_volmap_l(idx)		;
	eor	pcm_volmap_h(idx)		;
	ora	#$d0				;
	sta	pcm_volmap_h(idx)		;
	uret	UVEC+14, 32			;
@no_volume:
	uret	UVEC+14, 8
	
;===============================================================================
proc3: 			; +14
;===============================================================================
	lda	event_header		; 3 test function type
	and	#3<<6			; 2
	beq	@full_rate		; 2/3
	cmp	#2<<6			; 2
	beq	@lower_rate		; 2/3
	bcc	@write_sweep		; 2/3
	uret	UV_PROCESS_FRAME_EVENT, 13
;===============================================================================
@full_rate:
;===============================================================================
	lda	(pattern), y		; copy full rate and reset sweep
	iny				;
	sta	pcm_rate_l(idx)		;
	lda	(pattern), y		;
	iny				;
	sta	pcm_rate_h(idx)		;
	uret	UVEC+16, 30
;===============================================================================
@write_sweep:
;===============================================================================
	lda	(pattern), y		; copy sweep
	iny				;
	sta	pcm_sweep+idx		;
	uret	UV_PROCESS_FRAME_EVENT, 24
;===============================================================================
@lower_rate:
;===============================================================================
	lda	(pattern), y		; copy rate.l
	iny				;
	sta	pcm_rate_l(idx)		;
	lda	#0			;
	sta	pcm_sweep+idx		;
	uret	UV_PROCESS_FRAME_EVENT, 28
	
;===============================================================================
reset_sweep:	; +16
;===============================================================================
	lda	#0
	sta	pcm_sweep+idx
	uret	UV_PROCESS_FRAME_EVENT, 5
	
;===============================================================================
process_sweep:
;===============================================================================
	clc
	lda	pcm_sweep+idx		; get sweep speed & test direction
	bmi	@negative_sweep		;
;-------------------------------------------------------------------------------
	adc	pcm_rate_l(idx)		; sweep upwards
	sta	pcm_rate_l(idx)		;
	lda	pcm_rate_h(idx)		;
	adc	#0			;
	sta	pcm_rate_h(idx)		;
	uret	sweep_link, 25		;
;-------------------------------------------------------------------------------
@negative_sweep:			; sweep downward
	adc	pcm_rate_l(idx)		;
	sta	pcm_rate_l(idx)		;
	lda	pcm_rate_h(idx)		;
	adc	#$ff			;
	sta	pcm_rate_h(idx)		;
	uret	sweep_link, 26

.endscope
.endmacro

sqr_update_routines square1, UV_SQUARE1_PROC1, 0
sqr_update_routines square2, UV_SQUARE2_PROC1, 4
vrc6_routines vrc61, UV_VRC61_PROC1, 0, UV_VRC62_SWEEP
vrc6_routines vrc62, UV_VRC62_PROC1, 1, UV_VRC63_SWEEP
vrc6_routines vrc63, UV_VRC63_PROC1, 2, UV_PCM1_SWEEP

.if CHCOUNT > 1
	pcm_routines UV_PCM1_PROC1, pcm1, 0, UV_PCM2_SWEEP
.else
	pcm_routines UV_PCM1_PROC1, pcm1, 0, UV_PCM1_TESTEND
.endif

.if CHCOUNT > 2
	pcm_routines UV_PCM2_PROC1, pcm2, 1, UV_PCM3_SWEEP
.else
	pcm_routines UV_PCM2_PROC1, pcm2, 1, UV_PCM1_TESTEND
.endif

.if CHCOUNT > 3
	pcm_routines UV_PCM3_PROC1, pcm3, 2, UV_PCM4_SWEEP
.else
	pcm_routines UV_PCM3_PROC1, pcm3, 2, UV_PCM1_TESTEND
.endif

pcm_routines UV_PCM4_PROC1, pcm4, 3, UV_PCM1_TESTEND

square1_proc1 := square1::proc1
square1_proc2 := square1::proc2
square1_proc3 := square1::proc3

square2_proc1 := square2::proc1
square2_proc2 := square2::proc2
square2_proc3 := square2::proc3

vrc61_proc1 := vrc61::proc1
vrc61_proc2 := vrc61::proc2
vrc61_proc3 := vrc61::proc3
vrc61_copyfreq := vrc61::copyfreq
vrc61_sweep := vrc61::process_sweep

vrc62_proc1 := vrc62::proc1
vrc62_proc2 := vrc62::proc2
vrc62_proc3 := vrc62::proc3
vrc62_copyfreq := vrc62::copyfreq
vrc62_sweep := vrc62::process_sweep

vrc63_proc1 := vrc63::proc1
vrc63_proc2 := vrc63::proc2
vrc63_proc3 := vrc63::proc3
vrc63_copyfreq := vrc63::copyfreq
vrc63_sweep := vrc63::process_sweep

pcm1_proc1 := pcm1::proc1
pcm1_proc2 := pcm1::sampleoffset
pcm1_proc3 := pcm1::sampleoffset2
pcm1_proc4 := pcm1::sampleoffset3
pcm1_proc5 := pcm1::copysample
pcm1_proc6 := pcm1::copysample2
pcm1_proc7 := pcm1::proc2
pcm1_proc8 := pcm1::proc3
pcm1_proc9 := pcm1::reset_sweep

pcm2_proc1 := pcm2::proc1
pcm2_proc2 := pcm2::sampleoffset
pcm2_proc3 := pcm2::sampleoffset2
pcm2_proc4 := pcm2::sampleoffset3
pcm2_proc5 := pcm2::copysample
pcm2_proc6 := pcm2::copysample2
pcm2_proc7 := pcm2::proc2
pcm2_proc8 := pcm2::proc3
pcm2_proc9 := pcm2::reset_sweep

pcm3_proc1 := pcm3::proc1
pcm3_proc2 := pcm3::sampleoffset
pcm3_proc3 := pcm3::sampleoffset2
pcm3_proc4 := pcm3::sampleoffset3
pcm3_proc5 := pcm3::copysample
pcm3_proc6 := pcm3::copysample2
pcm3_proc7 := pcm3::proc2
pcm3_proc8 := pcm3::proc3
pcm3_proc9 := pcm3::reset_sweep

pcm4_proc1 := pcm4::proc1
pcm4_proc2 := pcm4::sampleoffset
pcm4_proc3 := pcm4::sampleoffset2
pcm4_proc4 := pcm4::sampleoffset3
pcm4_proc5 := pcm4::copysample
pcm4_proc6 := pcm4::copysample2
pcm4_proc7 := pcm4::proc2
pcm4_proc8 := pcm4::proc3
pcm4_proc9 := pcm4::reset_sweep



pcm1_sweep := pcm1::process_sweep
pcm2_sweep := pcm2::process_sweep
pcm3_sweep := pcm3::process_sweep
pcm4_sweep := pcm4::process_sweep

; sweep processing
; tri->vrc1->vrc2->vrcs->pcm
