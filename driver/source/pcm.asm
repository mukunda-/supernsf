
.include "settings.inc"

;==============================================================================
	.segment "VOLTABLE"
;==============================================================================

;------------------------------------------------------------------------------
; 128x32 multiplication table
;------------------------------------------------------------------------------
.incbin "python/volume_table.bin"

;==============================================================================
	.segment "TABLES"
;==============================================================================

;------------------------------------------------------------------------------
; output saturation table
;------------------------------------------------------------------------------

saturation_table:
.incbin "python/saturation_table.bin"

;===============================================================================
	.zeropage
;===============================================================================

__pcm_loop_l:	.res CHCOUNT
__pcm_loop_h:	.res CHCOUNT
__pcm_loop_b:	.res CHCOUNT

__pcm_sample:	.res CHCOUNT

__pcm_frac:	.res CHCOUNT
__pcm_bank:	.res CHCOUNT
dummy:		.res 1

;===============================================================================
	.segment "ZPCODE": zeropage
;===============================================================================

; zeropage code
; (for quick modification of addresses)
;

;===============================================================================
pcm_mix_next:
;===============================================================================
	ldx	$d000			; 4 mix samples together
	lda	$d000, x		; 4
	
.repeat CHCOUNT-1
	ldx	$d000			; 8*x cycles
	adc	$d000, x		; 
.endrep
;-------------------------------------------------------------------------------

.if CHCOUNT <> 0 			; <skip saturation/bias if only one channel>
	tax				; 2 saturate/bias
	lda	saturation_table, x	; 4
.endif					;

;-------------------------------------------------------------------------------
	sta	DAC			; 4
;-------------------------------------------------------------------------------
__update_vector:			; update vector, patch here
	jmp	(update_vector_table)	; 5
					; (39 cycles)
				
;===============================================================================
.segment "RAMCODE"
;===============================================================================
.repeat	15
	sec
	clc
.endrep
;-------------------------------------------------------------------------------
update_complete:		; return to here
;-------------------------------------------------------------------------------
	
;-------------------------------------------------------------------------------
.macro do_resample index, next
;-------------------------------------------------------------------------------
	lda	<pcm_frac(index)	; position += rate
	adc	#$00			;
	sta	<pcm_frac(index)	;
	lda	<pcm_addr(index)	;
	adc	#$00			;
	sta	<pcm_addr(index)	;
	bcc	next			; (19 cycles normally)
;-------------------------------------------------------------------------------
	inc	pcm_page(index)		; increment page
	lda	pcm_page(index)		;
	cmp	#$90+index*$10		;
	bne	next			; (31 cycles page-cross)
;-------------------------------------------------------------------------------
	clc
	lda	#$80+index*$10		; increment bank 
	sta	pcm_page(index)		;
	inc	pcm_bank(index)		;
	lda	pcm_bank(index)		;
	sta	$5ff8+index		; (47 cycles bank-cross)
;-------------------------------------------------------------------------------
.endmacro

;-------------------------------------------------------------------------------

resampling_start:			; resample channels
	do_resample	0, rs_return1	;
rs_return1:				; 19 cycles

.if CHCOUNT > 1
	do_resample	1, rs_return2	;
rs_return2:				; 38 cycles
.endif

.if CHCOUNT > 2
	do_resample	2, rs_return3	;
rs_return3:				; 57 cycles
.endif

.if CHCOUNT > 3
	do_resample	3, rs_return4	;
rs_return4:				; 76 cycles
.endif

resampling_end:
;-------------------------------------------------------------------------------

	jmp	pcm_mix_next		; return to top (+3 cycles)
	
					; 99 cycles total
		
.code

.macro loop_function idx, final, label_testend, label_loopsound
;===============================================================================
label_testend:
;===============================================================================
	
	ldx	pcm_sample(idx)		; check for sample loop
	lda	pcm_page(idx)		; 
	and	#0fh			; 
	cmp	sample_end_h, x		; 
	lda	pcm_bank(idx)
	bcc	@notend1		; 
	cmp	sample_end_b, x		; 
	bcc	@notend2		; 
	
	uret	UV_PCM1_LOOPSOUND+idx*4, (23+5)	; return
;-------------------------------------------------------------------------------
@notend1:				; test if bank > end_bank
	sbc	#0			; -1
	cmp	sample_end_b, x
	bcc	@notend3
	uret	UV_PCM1_LOOPSOUND+idx*4, (26+5)
@notend3:
					
	uret	final, 27
;-------------------------------------------------------------------------------
@notend2:				;; cyc=24
	uret	final, 24

;===============================================================================
label_loopsound:
;===============================================================================

	sec				;  2|
	lda	pcm_addr(idx)		;  3|
	sbc	pcm_loop_l(idx)		;  3|
	sta	pcm_addr(idx)		;  3|
	lda	pcm_page(idx)		;  3|
	sbc	pcm_loop_h(idx)		;  3|
	and	#0fh			;  2|
	ora	#080h+idx*10h		;  2|
	sta	pcm_page(idx)		;  3|
	lda	pcm_bank(idx)		;  3|
	sbc	pcm_loop_b(idx)		;  3|
	sta	pcm_bank(idx)		;  3|
	sta	$5ff8+idx		;  4|
	uret	final, (37-5)		; return
.endmacro

.if CHCOUNT > 3
loop_function 3, UV_PROC_SEQU, pcm4_testend, pcm4_loopsound
loop_function 2, UV_PCM4_TESTEND, pcm3_testend, pcm3_loopsound
loop_function 1, UV_PCM3_TESTEND, pcm2_testend, pcm2_loopsound
loop_function 0, UV_PCM2_TESTEND, pcm1_testend, pcm1_loopsound
.elseif CHCOUNT > 2
loop_function 2, UV_PROC_SEQU, pcm3_testend, pcm3_loopsound
;loop_function 2, UV_PCM1_TESTEND, pcm3_testend, pcm3_loopsound ;; DEBUG
loop_function 1, UV_PCM3_TESTEND, pcm2_testend, pcm2_loopsound
loop_function 0, UV_PCM2_TESTEND, pcm1_testend, pcm1_loopsound
.elseif CHCOUNT > 1
loop_function 1, UV_PROC_SEQU, pcm2_testend, pcm2_loopsound
loop_function 0, UV_PCM2_TESTEND, pcm1_testend, pcm1_loopsound
.else
loop_function 0, UV_PROC_SEQU, pcm1_testend, pcm1_loopsound
.endif
