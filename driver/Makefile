#
# DRIVER MAKEFILE
#

.SUFFIXES:

ifeq ($(CHANNELS),)
CHANNELS	:= 3	# PICK 1 TO 4
endif

#---------------------------------------------------------------------------------

CC65		:= /c/cc65/bin
AS			:= $(CC65)/ca65
LD			:= $(CC65)/ld65
PY			:= /c/Python26/python

#---------------------------------------------------------------------------------

TARGET		:=	pcm
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
VOLTABLE	:=	python/volume_table.bin
SATTABLE	:=	python/saturation_table.bin

#---------------------------------------------------------------------------------

ASFLAGS		:=	--cpu 6502 -s -D CHCOUNT=$(CHANNELS)
INCLUDE		:=	$(foreach dir,$(INCLUDES),-I $(CURDIR)/$(dir)) -I $(CURDIR)/$(BUILD)

OUTPUT		:=	$(CURDIR)/$(TARGET)
LDFLAGS		:=	-C $(CURDIR)/config.ld
VPATH		:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) 
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.asm)))
OFILES		:=	$(SFILES:.asm=.o)
 
.PHONY: clean
 
all	: $(TARGET).nsf

#---------------------------------------------------------------------------------

clean:
	@echo clean ...
	@rm -fr $(BUILD) python/*.bin $(TARGET).nsf $(TARGET).map
	
#---------------------------------------------------------------------------------

%.nsf: $(OFILES)
	@echo LINKING
	@$(LD) $(LDFLAGS) $(OFILES) -o $@ -m $(@:.nsf=.map)
	@echo Okay! :: $(notdir $@)
	
#---------------------------------------------------------------------------------

%.o : %.asm $(VOLTABLE) $(SATTABLE)
	@echo Assembling :: $(notdir $<)
	@$(AS) $(ASFLAGS) $(INCLUDE) $< -o $@

$(VOLTABLE) : python/volume_table.py
	@$(PY) python/volume_table.py $(CHANNELS)

$(SATTABLE) : python/saturation_table.py 
	@$(PY) python/saturation_table.py $(CHANNELS)
