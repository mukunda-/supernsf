;===============================================================================
; update vector table
;===============================================================================

.include "settings.inc"

.segment "UVT"

;-------------------------------------------------------------------------------
.macro ute label, name ; update table entry macro
name: .word label
.endmacro 
;-------------------------------------------------------------------------------

.export update_vector_table

update_vector_table:

	ute	square1_proc1,	UV_SEQ_CH0	; 2a03-square1
	ute	square2_proc1,	UV_SEQ_CH1	; 2a03-square2
	ute	triangle_proc1,	UV_SEQ_CH2	; 2a03-triangle
	ute	noise_proc1,	UV_SEQ_CH3	; 2a03-noise
	ute	vrc61_proc1,	UV_SEQ_CH4	; vrc6-square1
	ute	vrc62_proc1,	UV_SEQ_CH5	; vrc6-square2
	ute	vrc63_proc1,	UV_SEQ_CH6	; vrc6-sawtooth
	ute	pcm1_proc1,	UV_SEQ_CH7	; pcm1
	ute	pcm2_proc1,	UV_SEQ_CH8	; pcm2
	ute	pcm3_proc1,	UV_SEQ_CH9	; pcm3
	ute	pcm4_proc1,	UV_SEQ_CH10	; pcm4
	ute	sequencer_cross_page,		UV_SEQ_CH11	; sequencer pagecross signal
	ute	sequencer_end_frame,		UV_SEQ_CH12	; end of frame
	ute	reserved_sequencer_channel,	UV_SEQ_CH13
	ute	reserved_sequencer_channel,	UV_SEQ_CH14
	ute	sequencer_end_track,		UV_SEQ_CH15	; end of track
	
	ute	square1_proc1, UV_SQUARE1_PROC1
	ute	square1_proc2, UV_SQUARE1_PROC2
	ute	square1_proc3, UV_SQUARE1_PROC3
	
	ute	square2_proc1, UV_SQUARE2_PROC1
	ute	square2_proc2, UV_SQUARE2_PROC2
	ute	square2_proc3, UV_SQUARE2_PROC3
	
	ute	triangle_proc1, UV_TRIANGLE_PROC1
	ute	triangle_proc2, UV_TRIANGLE_PROC2
	ute	triangle_proc3, UV_TRIANGLE_PROC3
	ute	triangle_copyfreq, UV_TRIANGLE_COPYFREQ
	
	ute	noise_proc1, UV_NOISE_PROC1
	
	ute	vrc61_proc1, UV_VRC61_PROC1
	ute	vrc61_proc2, UV_VRC61_PROC2
	ute	vrc61_proc3, UV_VRC61_PROC3
	ute	vrc61_copyfreq, UV_VRC61_COPYFREQ
	
	ute	vrc62_proc1, UV_VRC62_PROC1
	ute	vrc62_proc2, UV_VRC62_PROC2
	ute	vrc62_proc3, UV_VRC62_PROC3
	ute	vrc62_copyfreq, UV_VRC62_COPYFREQ
	
	ute	vrc63_proc1, UV_VRC63_PROC1
	ute	vrc63_proc2, UV_VRC63_PROC2
	ute	vrc63_proc3, UV_VRC63_PROC3
	ute	vrc63_copyfreq, UV_VRC63_COPYFREQ
	
	ute	pcm1_proc1, UV_PCM1_PROC1
	ute	pcm1_proc2, UV_PCM1_PROC2
	ute	pcm1_proc3, UV_PCM1_PROC3
	ute	pcm1_proc4, UV_PCM1_PROC4
	ute	pcm1_proc5, UV_PCM1_PROC5
	ute	pcm1_proc6, UV_PCM1_PROC6
	ute	pcm1_proc7, UV_PCM1_PROC7
	ute	pcm1_proc8, UV_PCM1_PROC8
	ute	pcm1_proc9, UV_PCM1_PROC9
	
	ute	pcm2_proc1, UV_PCM2_PROC1
	ute	pcm2_proc2, UV_PCM2_PROC2
	ute	pcm2_proc3, UV_PCM2_PROC3
	ute	pcm2_proc4, UV_PCM2_PROC4
	ute	pcm2_proc5, UV_PCM2_PROC5
	ute	pcm2_proc6, UV_PCM2_PROC6
	ute	pcm2_proc7, UV_PCM2_PROC7
	ute	pcm2_proc8, UV_PCM2_PROC8
	ute	pcm2_proc9, UV_PCM2_PROC9
	
	ute	pcm3_proc1, UV_PCM3_PROC1
	ute	pcm3_proc2, UV_PCM3_PROC2
	ute	pcm3_proc3, UV_PCM3_PROC3
	ute	pcm3_proc4, UV_PCM3_PROC4
	ute	pcm3_proc5, UV_PCM3_PROC5
	ute	pcm3_proc6, UV_PCM3_PROC6
	ute	pcm3_proc7, UV_PCM3_PROC7
	ute	pcm3_proc8, UV_PCM3_PROC8
	ute	pcm3_proc9, UV_PCM3_PROC9
	
	ute	pcm4_proc1, UV_PCM4_PROC1
	ute	pcm4_proc2, UV_PCM4_PROC2
	ute	pcm4_proc3, UV_PCM4_PROC3
	ute	pcm4_proc4, UV_PCM4_PROC4
	ute	pcm4_proc5, UV_PCM4_PROC5
	ute	pcm4_proc6, UV_PCM4_PROC6
	ute	pcm4_proc7, UV_PCM4_PROC7
	ute	pcm4_proc8, UV_PCM4_PROC8
	ute	pcm4_proc9, UV_PCM4_PROC9
	
	ute	triangle_process_sweep, UV_TRIANGLE_SWEEP
	ute	vrc61_sweep, UV_VRC61_SWEEP
	ute	vrc62_sweep, UV_VRC62_SWEEP
	ute	vrc63_sweep, UV_VRC63_SWEEP
	ute	pcm1_sweep, UV_PCM1_SWEEP
	ute	pcm2_sweep, UV_PCM2_SWEEP
	ute	pcm3_sweep, UV_PCM3_SWEEP
	ute	pcm4_sweep, UV_PCM4_SWEEP
	
	ute	pcm1_testend,		UV_PCM1_TESTEND
	ute	pcm1_loopsound,		UV_PCM1_LOOPSOUND
	
	.if CHCOUNT > 1
	ute	pcm2_testend,		UV_PCM2_TESTEND
	ute	pcm2_loopsound,		UV_PCM2_LOOPSOUND
	.endif
	
	.if CHCOUNT > 2
	ute	pcm3_testend,		UV_PCM3_TESTEND
	ute	pcm3_loopsound,		UV_PCM3_LOOPSOUND
	.endif
	
	.if CHCOUNT > 3
	ute	pcm4_testend,		UV_PCM4_TESTEND
	ute	pcm4_loopsound,		UV_PCM4_LOOPSOUND
	.endif
	
	ute	sequencer_process_bf,	UV_PROC_SEQU
	ute	process_frame_event,	UV_PROCESS_FRAME_EVENT
