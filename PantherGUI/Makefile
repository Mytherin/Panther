
CCPP=clang++
OPTFLAGS=-O0 -g
LDFLAGS=-framework AppKit -fobjc-arc -lobjc -DPANTHER_DEBUG -L/opt/local/lib -licuuc -licutu -liculx -licuio -licule -licudata -licui18n -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -I/Users/myth/Sources/skia/skia/include/effects -L/Users/myth/Sources/skia/skia/out/Static-Release -lskia -std=c++11 

all: panther.app
COBJECTS=basictextfield.cpp button.cpp c.cpp container.cpp control.cpp controlmanager.cpp cursor.cpp encoding.cpp file.cpp filemanager.cpp findtext.cpp keybindings.cpp keywords.cpp language.cpp logger.cpp notification.cpp regex.cpp renderer.cpp searchbox.cpp settings.cpp scheduler.cpp scrollbar.cpp simpletextfield.cpp statusbar.cpp style.cpp syntaxhighlighter.cpp tabcontrol.cpp text.cpp textbuffer.cpp textfield.cpp textfile.cpp thread.cpp togglebutton.cpp unicode.cpp utils.cpp windowfunctions.cpp wrappedtextiterator.cpp textiterator.cpp textline.cpp xml.cpp textdelta.cpp workspace.cpp
panther.app:
		$(CCPP) $(LDFLAGS) main.mm PGView.mm PGWindow.mm $(COBJECTS) $(OPTFLAGS) -o main.app
