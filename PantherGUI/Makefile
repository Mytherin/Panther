
CCPP=clang++

all: panther.app

COBJECTS=basictextfield.cpp button.cpp c.cpp container.cpp control.cpp controlmanager.cpp cursor.cpp encoding.cpp file.cpp filemanager.cpp findtext.cpp keywords.cpp language.cpp logger.cpp regex.cpp renderer.cpp scheduler.cpp scrollbar.cpp simpletextfield.cpp statusbar.cpp style.cpp syntaxhighlighter.cpp tabcontrol.cpp text.cpp textbuffer.cpp textfield.cpp textfile.cpp thread.cpp togglebutton.cpp unicode.cpp utils.cpp windowfunctions.cpp xml.cpp
OPTFLAGS=-O0 -g
panther.app:
		$(CCPP) -framework AppKit -fobjc-arc -lobjc -L/opt/local/lib -licuuc -licutu -liculx -licuio -licule -licudata -licui18n -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static-Release -lskia -std=c++11 main.mm PGView.mm $(COBJECTS) $(OPTFLAGS) -o main.app
