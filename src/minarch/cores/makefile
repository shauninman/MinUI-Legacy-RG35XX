# this logic was broken out from picoarch's all-in-one makefile

CORES = fceumm gambatte gpsp pcsx_rearmed picodrive pokemini snes9x2005_plus
CORES+= beetle-vb

###############################

# optional core vars
# *_REPO=
# *_HASH=
# *_CORE=
# *_FLAGS=
# *_MAKEFILE=
# *_BUILD_PATH=

beetle-vb_REPO = https://github.com/libretro/beetle-vb-libretro
beetle-vb_CORE = mednafen_vb_libretro.so

fceumm_REPO = https://github.com/libretro/libretro-fceumm

gambatte_REPO = https://github.com/libretro/gambatte-libretro

pcsx_rearmed_MAKEFILE = Makefile.libretro

picodrive_REPO = https://github.com/irixxxx/picodrive
picodrive_MAKEFILE = Makefile.libretro

pokemini_REPO = https://github.com/libretro/PokeMini
pokemini_MAKEFILE = Makefile.libretro

snes9x2005_plus_REPO = https://github.com/libretro/snes9x2005
snes9x2005_plus_FLAGS = USE_BLARGG_APU=1

###############################

PATCH = git apply
PROCS = -j4

###############################

define TEMPLATE=

$1_REPO ?= https://github.com/libretro/$(1)
$1_MAKE ?= make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) platform=$(UNION_PLATFORM) $($(1)_FLAGS)
$1_BUILD_PATH ?= $(1)

$(1):
	mkdir -p cores
	cd cores && git clone $(if $($1_HASH),,--depth 1) --recursive $$($(1)_REPO) $(1)
	$(if $($1_HASH),cd $$($1_BUILD_PATH) && git checkout $($1_HASH) && echo $($1_HASH),)

$(1)/.patched: $(1)
	(test ! -f patches/$(1).patch) || (test -f $(1)/.patched) || (cd $(1) && $(PATCH) -p1 < ../../patches/$(1).patch && touch .patched && true)

$(1)_libretro.so: $(1)/.patched
	cd $$($1_BUILD_PATH) && $$($1_MAKE) $(PROCS)
	mv $$($1_BUILD_PATH)/$(if $($(1)_CORE),$($(1)_CORE),$(1)_libretro.so) $(1)_libretro.so

clone-$(1): $(1)
	
patch-$(1): $(1)/.patched

clean-$(1):
	test ! -d $(1) || cd $$($1_BUILD_PATH) && $$($1_MAKE) clean
	rm -rf $(1)_libretro.so

$(1): $(1)_libretro.so

endef

###############################

all: cores

$(foreach CORE,$(CORES),$(eval $(call TEMPLATE,$(CORE))))

cores: $(foreach CORE,$(CORES),$(CORE)_libretro.so)