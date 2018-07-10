CC=$(CPPHOST)
CCFLAGS=-O2 -Wall
CFILES=game.cpp gamestate.cpp gui.cpp image.cpp main.cpp panel.cpp player.cpp resources.cpp screen.cpp sector.cpp sound.cpp tutorial.cpp utils.cpp TinyXML/tinyxml.cpp TinyXML/tinyxmlerror.cpp TinyXML/tinyxmlparser.cpp
HFILES=game.h gamestate.h gui.h image.h panel.h player.h resources.h screen.h sector.h sound.h tutorial.h utils.h common.h stdafx.h TinyXML/tinyxml.h
OFILES=game.o gamestate.o gui.o image.o panel.o player.o resources.o screen.o sector.o sound.o tutorial.o utils.o main.o TinyXML/tinyxml.o TinyXML/tinyxmlerror.o TinyXML/tinyxmlparser.o
APP=gigalomania
INC=$(CPPFLAGS)
LINKPATH=$(LDFLAGS)
GAMEPATH="/games/gigalomania"

LIBS=-lSDL_mixer -lSDL_image -ljpeg -lpng -lSDL -lorbital -lz

all: $(APP)

$(APP): $(OFILES) $(HFILES) $(CFILES)
	$(CC) $(OFILES) $(CCFLAGS) $(LINKPATH) $(LIBS) -o $(APP)

.cpp.o:
	$(CC) $(CCFLAGS) -O2 $(INC) -c $< -o $@

# REMEMBER to update debian/dirs if the system directories that we use are changed!!!
install: $(APP)
	mkdir -p $(DESTDIR)$(GAMEPATH) # -p so we don't fail if folder already exists
	cp $(APP) $(DESTDIR)$(GAMEPATH)
	cp readme.html $(DESTDIR)$(GAMEPATH)
	cp -a gfx/ $(DESTDIR)$(GAMEPATH)
	cp -a islands/ $(DESTDIR)$(GAMEPATH)
	cp -a music/ $(DESTDIR)$(GAMEPATH)
	cp -a sound/ $(DESTDIR)$(GAMEPATH)
# REMEMBER to update debian/dirs if the system directories that we use are changed!!!

uninstall:
	rm -rf $(DESTDIR)$(GAMEPATH)

clean:
	rm -rf *.o
	rm -f $(APP)
