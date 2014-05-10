#-------------------------------------------------------------------------
# NSF Memory Specification
#-------------------------------------------------------------------------

#*************************************************************************
MEMORY {
#*************************************************************************

#-------------------------------------------------------------------------
# Program (5K)
#-------------------------------------------------------------------------
	HEADER:		start = $DF80, size = $80, fill = yes;		# NSF header
	UVTPAGE:	start = $E000, size = $100, fill = yes;		# update vector table
	SATPAGE:	start = $E100, size = $100, fill = yes;		# saturation table
	TRACKHEAD:	start = $E200, size = $240, fill = yes;		# track header
	PROG:		start = $E440, size = $1ac0, fill = yes;	# main prog
	XPAGE:		start = $FF00, size = $100, fill = yes;		# THE X PAGE
	VOLTAB:		start = $10000, size = $1000, fill = yes;	# VOLUME TABLE
	
#-------------------------------------------------------------------------
# Memory
#-------------------------------------------------------------------------
	
	ZMEM:		start = $0000, size = 128;		# 128 bytes data
	ZCODE:		start = $0080, size = 128;		# 128 bytes code
	RAMCODE:	start = $0200, size = 512;		# 512 bytes code
	MEM:		start = $0400, size = 512;		# 512 bytes data
}

#*************************************************************************
SEGMENTS {
#*************************************************************************

#-------------------------------------------------------------------------
# standard segments
#-------------------------------------------------------------------------
	HEADER:					# NSF header
		load = HEADER, 		#
		type = ro;			#
		
	TRACKHEADER:			#
		load = TRACKHEAD,	#
		type = ro;			#
	
	ZPCODE:					# fast-modify zeropage code
		load = PROG,		#
		run = ZCODE,		#
		type = rw,			#
		define = yes;		#
		
	RAMCODE:				# moddable code
		load = PROG,		#
		run = RAMCODE,		#
		type = ro,			#
		define = yes;		#
		
	CODE:					# 4k main program
		load = PROG, 		#
		type = ro;
		
	TABLES:					# other tables
		load = SATPAGE,		#
		type = ro;			#
		
	UVT:					# 256-byte update vector table
		load = UVTPAGE,
		type = ro;
	
	ZEROPAGE:				# 128b zpage data
		load = ZMEM,		#
		type = zp;			#
		
	XPAGE:					# XPAGE	
		load = XPAGE,		#
		type = ro;			#
		
	VOLTABLE:				# 4k volume table	
		load = VOLTAB,		#
		type = ro;			#
}
