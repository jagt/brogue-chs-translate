RELEASENAME = brogue-1.7.2-clight2

MACHINE_NAME = $(shell uname -m)
SYSTEM_NAME = $(shell uname -s)

# For x86, we will set the machine type to enable MMX
# (It is enabled by default for x86_64, so we don't need to 
# do anything special there.)
ifneq ($(filter %86,${MACHINE_NAME}),)
	PLATFORM_CFLAGS = -mpentium-mmx
else
	PLATFORM_CFLAGS = 
endif

#BASE_CFLAGS = -O2 -g -Isrc/brogue -Isrc/platform 
BASE_CFLAGS = -g -Isrc/brogue -Isrc/platform 

ifeq (${SYSTEM_NAME},Darwin)
	PLATFORM=osx
	FRAMEWORK_FLAGS = \
		-framework SDL \
		-framework SDL_ttf \
		-framework Cocoa
	SDL_CFLAGS = -DBROGUE_SDL -F build/osx
	SDL_LIB = -F build/osx ${FRAMEWORK_FLAGS} -Wl,-rpath,@executable_path/../Frameworks
	PKG_CONFIG = build/osx/gtk/bin/pkg-config
	OBJC_SOURCE = src/platform/SDLMain.m
else
	PLATFORM=linux
	SDL_CFLAGS = -DBROGUE_SDL
	SDL_LIB = -lSDL -lSDL_ttf
	PKG_CONFIG = pkg-config
	OBJC_SOURCE = 
endif

RSVG_CFLAGS = $(shell ${PKG_CONFIG} --cflags librsvg-2.0 cairo)
RSVG_LIB = $(shell ${PKG_CONFIG} --libs librsvg-2.0 cairo)

CFLAGS = \
	${BASE_CFLAGS} ${PLATFORM_CFLAGS} \
	-Wall -Wno-parentheses \
	${SDL_CFLAGS} ${RSVG_CFLAGS}
LDFLAGS = ${SDL_LIB} ${RSVG_LIB}

TARFLAGS = --transform 's,^,${RELEASENAME}/,'
TARFILE = dist/${RELEASENAME}-source.tar.gz
ZIPFILE = dist/${RELEASENAME}-win32.zip
DMGFILE = dist/${RELEASENAME}-osx.dmg
PATCHFILE = dist/${RELEASENAME}.patch.gz

GLOBAL_HEADERS = \
	src/brogue/IncludeGlobals.h \
	src/brogue/Rogue.h

SOURCE = \
	src/brogue/Architect.c \
	src/brogue/Combat.c \
	src/brogue/Dijkstra.c \
	src/brogue/Globals.c \
	src/brogue/IO.c \
	src/brogue/Items.c \
	src/brogue/Light.c \
	src/brogue/Monsters.c \
	src/brogue/Buttons.c \
	src/brogue/Movement.c \
	src/brogue/Recordings.c \
	src/brogue/RogueMain.c \
	src/brogue/Random.c \
	src/brogue/MainMenu.c \
	src/brogue/Grid.c \
	\
	src/platform/main.c \
	src/platform/platformdependent.c \
	src/platform/sdl-platform.c \
	src/platform/sdl-display.c \
	src/platform/sdl-draw.c \
	src/platform/sdl-effects.c \
        src/platform/sdl-keymap.c \
	src/platform/sdl-svgset.c 

DISTFILES = \
	readme \
	brogue \
	Makefile \
	agpl.txt \
	brogue-seed-catalog.txt \
	$(wildcard src/brogue/*.[mch]) \
	$(wildcard src/platform/*.[mch]) \
	$(wildcard svg/*.svg) \
	$(wildcard res/*) \
	$(wildcard build/*.sh) \
	$(wildcard build/*.py) \
	$(wildcard build/brogue.rc) \
	$(wildcard build/Makefile.*)

RES = \
	res/Andale_Mono.ttf \
	res/FreeSans.ttf \
	res/keymap \
	res/icon.bmp

SVG = $(wildcard svg/*.svg)

DSTRES = $(patsubst res/%,build/${PLATFORM}/%,${RES})
DSTSVG = $(patsubst svg/%,build/${PLATFORM}/svg/%,${SVG})

OBJS = \
	$(patsubst src/%.c,build/${PLATFORM}/obj/%.o,${SOURCE}) \
	$(patsubst src/%.m,build/${PLATFORM}/obj/%.o,${OBJC_SOURCE})

.PHONY: all clean debdepends linux win32 osx dist distdir builddirs

ifeq (${SYSTEM_NAME},Darwin)
all: osx
else
all: linux
endif

include build/Makefile.win32
include build/Makefile.osx

linux: debdepends builddirs build/linux/brogue ${DSTRES} ${DSTSVG}

debdepends:
	build/checkdeps.sh librsvg2-dev libsdl-ttf2.0-dev libsdl1.2-dev

builddirs: build/${PLATFORM}/obj/brogue build/${PLATFORM}/obj/platform build/${PLATFORM}/svg

build/${PLATFORM}/obj/brogue:
	_mkdir -p $@

build/${PLATFORM}/obj/platform:
	_mkdir -p $@

build/${PLATFORM}/svg:
	_mkdir -p $@

build/${PLATFORM}/brogue: ${OBJS}
	@echo '    LD' $@
	@gcc -o $@ ${OBJS} ${LDFLAGS}

build/${PLATFORM}/obj/%.o: src/%.c ${GLOBAL_HEADERS}
	@echo '    CC' $@
	@gcc ${CFLAGS} -c $< -o $@ 

build/${PLATFORM}/obj/%.o: src/%.m ${GLOBAL_HEADERS}
	@echo '    CC' $@
	@gcc ${CFLAGS} -c $< -o $@ 

build/${PLATFORM}/%.svg: svg/%.svg
	cp $< $@

build/${PLATFORM}/%: res/%
	cp $< $@

build/${PLATFORM}/svg/%: svg/%
	cp $< $@

clean: osxdist_ssh_clean
	rm -fr build/linux build/win32 build/osx

objclean:
	rm -fr build/${PLATFORM}/obj

dist: ${TARFILE} ${ZIPFILE} ${PATCHFILE} osxdist_ssh

distdir:
	_mkdir -p dist

tar: ${TARFILE} ${PATCHFILE}

${TARFILE}: distdir all
	rm -f ${TARFILE}
	tar ${TARFLAGS} -czf ${TARFILE} ${DISTFILES}

${PATCHFILE}: distdir
	git diff upstream HEAD | gzip >${PATCHFILE}
