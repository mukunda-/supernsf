
.define CYCLES_FOR_UPDATE 40
.define VRC6 1


; cfm = 39,72,99,126
;       1channel
;          2channels
;             3channels
;                4channels

.if CHCOUNT=1
	.define CYCLES_FOR_MIXING 39
.elseif CHCOUNT=2
	.define CYCLES_FOR_MIXING 72
.elseif CHCOUNT=3
	.define CYCLES_FOR_MIXING 99
.elseif CHCOUNT=4
	.define CYCLES_FOR_MIXING 126
.endif


;sr=1789773/cps

;-------------------------------------------------------------------------------
; registers
;-------------------------------------------------------------------------------
APU_CTRL	:=	4015h
DAC		:=	4011h

.globalzp __update_vector

update_vector	:= __update_vector+1

.macro nop3
	sta	dummy
.endmacro

;------------------------------------------------------------------------------
; macro for returning from update routines
;
; jumps to proper return address depending on how many cycles were used
;------------------------------------------------------------------------------
.macro uret vector, cycles
	lda	#<vector		;2
	.if (cycles & 1) = 0		;
		sta	update_vector	;+3 make odd
		
		.if (cycles+2+3+3) <= CYCLES_FOR_UPDATE
			jmp	update_complete-((CYCLES_FOR_UPDATE-(cycles+2+3+3))/2)
		.else
			.warning "ROUTINE USING MORE THAN CFU"
			jmp	update_complete
		.endif
	
	.else
		; if cycles exceed CFU then just cheat!
		;
		sta	a:update_vector		;+4 keep odd
		.if (cycles+2+4+3) <= CYCLES_FOR_UPDATE
			jmp	update_complete-((CYCLES_FOR_UPDATE-(cycles+2+4+3))/2)
		.else
			.warning "ROUTINE USING MORE THAN CFU"
			jmp	update_complete
		.endif
	
	.endif
.endmacro

;-------------------------------------------------------------------------------


; imports

.global __init
.global __play

.globalzp data1
.globalzp data2
.globalzp data3
.globalzp data4

;---------------------------

.global UV_PROC_SEQU
.global UV_PCM1_TESTEND
.global UV_PCM1_LOOPSOUND
.global UV_PCM2_TESTEND
.global UV_PCM2_LOOPSOUND
.global UV_PCM3_TESTEND
.global UV_PCM3_LOOPSOUND
.global UV_PCM4_TESTEND
.global UV_PCM4_LOOPSOUND

.global UV_SQUARE1_PROC1
.global UV_SQUARE1_PROC2
.global UV_SQUARE1_PROC3

.global UV_SQUARE2_PROC1
.global UV_SQUARE2_PROC2
.global UV_SQUARE2_PROC3

.global UV_TRIANGLE_PROC1
.global UV_TRIANGLE_PROC2
.global UV_TRIANGLE_PROC3
.global UV_TRIANGLE_COPYFREQ

.global UV_VRC61_PROC1
.global UV_VRC62_PROC1
.global UV_VRC63_PROC1

.global UV_PCM1_PROC1
.global UV_PCM2_PROC1
.global UV_PCM3_PROC1
.global UV_PCM4_PROC1

.global UV_PROCESS_FRAME_EVENT

.global UV_TRIANGLE_SWEEP
.global UV_VRC61_SWEEP
.global UV_VRC62_SWEEP
.global UV_VRC63_SWEEP
.global UV_PCM1_SWEEP
.global UV_PCM2_SWEEP
.global UV_PCM3_SWEEP
.global UV_PCM4_SWEEP

.global sample_addresses_l
.global sample_addresses_h
.global sample_addresses_b
.global sample_end_h
.global sample_end_b
.global sample_loop_sizes_l
.global sample_loop_sizes_h
.global sample_loop_sizes_b
.global sequencer_reset

.global update_vector_table

.global update_complete

;-----------------------------------
; sequencer

.global sequencer_process_bf
.global process_frame_event
.global sequencer_cross_page
.global sequencer_end_frame
.global sequencer_end_track
.global square1_proc1
.global square1_proc2
.global square1_proc3
.global square2_proc1
.global square2_proc2
.global square2_proc3
.global triangle_proc1
.global triangle_proc2
.global triangle_proc3
.global triangle_copyfreq
.global noise_proc1

.global vrc61_proc1
.global vrc61_proc2
.global vrc61_proc3
.global vrc61_copyfreq

.global vrc62_proc1
.global vrc62_proc2
.global vrc62_proc3
.global vrc62_copyfreq

.global vrc63_proc1
.global vrc63_proc2
.global vrc63_proc3
.global vrc63_copyfreq

.global pcm1_proc1
.global pcm1_proc2 
.global pcm1_proc3 
.global pcm1_proc4
.global pcm1_proc5
.global pcm1_proc6
.global pcm1_proc7
.global pcm1_proc8
.global pcm1_proc9

.global pcm2_proc1 
.global pcm2_proc2 
.global pcm2_proc3 
.global pcm2_proc4
.global pcm2_proc5
.global pcm2_proc6
.global pcm2_proc7
.global pcm2_proc8
.global pcm2_proc9

.global pcm3_proc1 
.global pcm3_proc2
.global pcm3_proc3
.global pcm3_proc4
.global pcm3_proc5
.global pcm3_proc6
.global pcm3_proc7
.global pcm3_proc8
.global pcm3_proc9

.global pcm4_proc1 
.global pcm4_proc2
.global pcm4_proc3
.global pcm4_proc4
.global pcm4_proc5
.global pcm4_proc6
.global pcm4_proc7
.global pcm4_proc8
.global pcm4_proc9

.global triangle_process_sweep
.global vrc61_sweep
.global vrc62_sweep
.global vrc63_sweep
.global pcm1_sweep
.global pcm2_sweep
.global pcm3_sweep
.global pcm4_sweep

.global reserved_sequencer_channel

.globalzp sequencer_timer
.globalzp pattern_L
.globalzp pattern

;-----------------------------------
; pcm

.global pcm4_loopsound
.global pcm4_testend
.global pcm3_loopsound
.global pcm3_testend
.global pcm2_loopsound
.global pcm2_testend
.global pcm1_loopsound
.global pcm1_testend

.global poopler_buttles

.globalzp __pcm_loop_l, __pcm_loop_h, __pcm_loop_b
.globalzp __pcm_sample
.globalzp __pcm_frac
.globalzp __pcm_bank

.globalzp pcm_mix_next
.global resampling_start
.global resampling_end

.global volume_table_start
.global saturation_table

.define pcm_frac(i) __pcm_frac+i
.define pcm_bank(i) __pcm_bank+i
.define pcm_addr(i) pcm_mix_next+1+i*6
.define pcm_page(i) pcm_mix_next+2+i*6
.define pcm_loop_l(i) __pcm_loop_l+i
.define pcm_loop_h(i) __pcm_loop_h+i
.define pcm_loop_b(i) __pcm_loop_b+i
.define pcm_sample(i) __pcm_sample+i

.define pcm_rate_l(i) resampling_start + 3 + i*((resampling_end-resampling_start)/CHCOUNT)
.define pcm_rate_h(i) resampling_start + 9 + i*((resampling_end-resampling_start)/CHCOUNT)

.define pcm_volmap_l(i) pcm_mix_next + 4 + i*6	; volume table address (L)
.define pcm_volmap_h(i) pcm_mix_next + 5 + i*6	; volume table address (H)

.macro probe
.local @pood
	nop
@pood:
	jmp	@pood
	nop
.endmacro
