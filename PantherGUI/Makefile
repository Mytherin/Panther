
CCPP=clang++

all: panther.app

COBJECTS=c.cpp container.cpp control.cpp controlmanager.cpp cursor.cpp file.cpp filemanager.cpp keywords.cpp logger.cpp renderer.cpp scheduler.cpp syntaxhighlighter.cpp tabcontrol.cpp text.cpp textfield.cpp textfile.cpp textline.cpp thread.cpp unicode.cpp utils.cpp windowfunctions.cpp xml.cpp
OPTFLAGS=-O3
panther.app:
		$(CCPP) -framework AppKit -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm PGView.mm $(COBJECTS) $(OPTFLAGS) -o main.app
