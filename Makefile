# Makefile
CREATORID = CGLF
FILENAME = MiniGolf
SRCFILES = $(wildcard sauce/*.c) $(wildcard sauce/game/*.c) $(wildcard sauce/startscreen/*.c)
HIRES = false

# Palm SDK config
SDK_VERSION = 5
PALMCC = m68k-palmos-gcc
PALMINC = /opt/palmdev/sdk-5r3/include
PILRC = /usr/bin/pilrc
PALMCFLAGS = -O2 -DPALMOS -DSDK_$(SDK_VERSION)=1 \
	-I$(PALMINC) \
	-I$(PALMINC)/Dynamic \
	-I$(PALMINC)/Core \
	-I$(PALMINC)/Core/UI \
	-I$(PALMINC)/Core/Hardware \
	-I$(PALMINC)/Core/System \
	-I$(PALMINC)/Core/System/Unix \
	-I$(PALMINC)/Libraries \
	-I$(PALMINC)/Libraries/PalmOSGlue \
	-I$(PALMINC)/Libraries/Lz77 \
	-I$(PALMINC)/Libraries/ExgLocal \
	-I$(PALMINC)/Libraries/Sms \
	-I$(PALMINC)/Libraries/Pdi \
	-I$(PALMINC)/Libraries/Telephony \
	-I$(PALMINC)/Libraries/Telephony/UI \
	-I$(PALMINC)/Libraries/INet \
	-I$(PALMINC)/Extensions \
	-I$(PALMINC)/Extensions/ExpansionMgr
WARNINGFLAGS = -Wswitch -Wunused

all:
	$(MAKE) EXT="_lowres" HIRES=false build
	$(MAKE) EXT="_hires" HIRES=true PILRCFLAGS="-D PALMHIRES" GCCFLAGS="-DHIRESBUILD" build
	$(MAKE) debug

debug: 
	$(MAKE) EXT="_debug" HIRES=false GCCFLAGS="-DDEBUG" build

build: compile prebin bin gen_grc combine cleanup

compile: 
	$(PALMCC) $(GCCFLAGS) $(PALMCFLAGS) ${WARNINGFLAGS} $(SRCFILES)

ifeq ($(HIRES), true)
prebin:
	./generateBitmaps.sh
	./generateResourceFile.sh --hires
else
prebin:
	./generateBitmaps.sh
	./generateResourceFile.sh
endif

bin:
	$(PILRC) $(PILRCFLAGS) resources/ui.rcp 
	$(PILRC) $(PILRCFLAGS) resources/graphicResources.rcp

gen_grc: 
	m68k-palmos-obj-res a.out

combine:	
	build-prc artifacts/$(FILENAME)$(EXT).prc "$(FILENAME)" $(CREATORID) *.a.out.grc *.bin

cleanup:
	rm *.grc *.out *.bin
	rm -Rf resources/assets
	rm -Rf resources/144
	rm resources/graphicResources.rcp
