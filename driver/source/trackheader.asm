
.export sample_addresses_l
.export sample_addresses_h
.export sample_addresses_b
.export sample_end_h
.export sample_end_b
.export sample_loop_sizes_l
.export sample_loop_sizes_h
.export sample_loop_sizes_b
.export sequencer_reset

;-------------------------------------------------------------------------------
.segment "TRACKHEADER"
;-------------------------------------------------------------------------------
sample_addresses_l:		.res	64, $aa
sample_addresses_h:		.res	64, $bb
sample_addresses_b:		.res	64, $cc

sample_end_h:			.res	64, $aa
sample_end_b:			.res	64, $bb

sample_loop_sizes_l:	.res	64, $aa
sample_loop_sizes_h:	.res	64, $bb
sample_loop_sizes_b:	.res	64, $cc

sequencer_reset:		.res	 3, $aa
						.res	13, $bb


;-------------------------------------------------------------------------------
; meta data
;-------------------------------------------------------------------------------
	.res	15, $aa		; exporter version
	.res	1, '.'		; .
	.res	1, '3'		; channels
	.res	1, 'C' 
	.res	1, 'H'
	.res	1, '.'
	.res	4, '-'		; exp chip
	.res	8, $aa

	.byte	"zebra           " ; animal
