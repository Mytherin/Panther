
CCPP=clang++
OPTFLAGS=-O0 -g

SKIA_INCLUDE_FLAGS=-I/Users/myth/Sources/skia/skia/include/core \
				   -I/Users/myth/Sources/skia/skia/include/config \
				   -I/Users/myth/Sources/skia/skia/include/codec \
				   -I/Users/myth/Sources/skia/skia/include/effects
SKIA_LDFLAGS=-L/Users/myth/Sources/skia/skia/out/Static-Release-PNG -lskia
ICU_INCLUDE_FLAGS=
ICU_LDFLAGS=-licuuc -licutu -liculx -licuio -licule -licudata -licui18n


CPPFLAGS=-DPANTHER_DEBUG -std=c++11
OBJCFLAGS=-fobjc-arc
LDFLAGS=-framework AppKit -lobjc -L/opt/local/lib $(ICU_LDFLAGS) $(SKIA_LDFLAGS)
INCLUDE_FLAGS=$(SKIA_INCLUDE_FLAGS) $(ICU_INCLUDE_FLAGS) -Ithird_party/re2

COBJECTS=obj/basictextfield.o \
		 obj/button.o \
		 obj/c.o \
		 obj/container.o \
		 obj/control.o \
		 obj/controlmanager.o \
		 obj/cursor.o \
		 obj/encoding.o \
		 obj/file.o \
		 obj/filemanager.o \
		 obj/findtext.o \
		 obj/keybindings.o \
		 obj/keywords.o \
		 obj/language.o \
		 obj/logger.o \
		 obj/notification.o \
		 obj/regex.o \
		 obj/renderer.o \
		 obj/scheduler.o \
		 obj/scrollbar.o \
		 obj/searchbox.o \
		 obj/settings.o \
		 obj/simpletextfield.o \
		 obj/statusbar.o \
		 obj/style.o \
		 obj/syntaxhighlighter.o \
		 obj/tabcontrol.o \
		 obj/text.o \
		 obj/textbuffer.o \
		 obj/textdelta.o \
		 obj/textfield.o \
		 obj/textfile.o \
		 obj/textiterator.o \
		 obj/textline.o \
		 obj/textposition.o \
		 obj/thread.o \
		 obj/togglebutton.o \
		 obj/unicode.o \
		 obj/utils.o \
		 obj/windowfunctions.o \
		 obj/workspace.o \
		 obj/wrappedtextiterator.o \
		 obj/xml.o 

OBJCOBJECTS=obj/AppDelegate.o \
			obj/main.o \
			obj/PGView.o \
			obj/PGWindow.o

RE2OBJECTS=obj/bitstate.o \
		   obj/compile.o \
		   obj/dfa.o \
		   obj/filtered_re2.o \
		   obj/mimics_pcre.o \
		   obj/nfa.o \
		   obj/onepass.o \
		   obj/parse.o \
		   obj/perl_groups.o \
		   obj/prefilter.o \
		   obj/prefilter_tree.o \
		   obj/prog.o \
		   obj/re2.o \
		   obj/regexp.o \
		   obj/rune.o \
		   obj/set.o \
		   obj/simplify.o \
		   obj/stringpiece.o \
		   obj/strutil.o \
		   obj/tostring.o \
		   obj/unicode_casefold.o \
		   obj/unicode_groups.o 

all: create_build_directory $(RE2OBJECTS) $(COBJECTS) $(OBJCOBJECTS) panther.app

clean:
	rm -rf obj
	rm -f main.app

create_build_directory:
	mkdir -p obj

obj/%.o: third_party/re2/re2/%.cc
	$(CCPP) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c third_party/re2/$(subst obj,re2,$(subst .o,.cc,$@)) -o $@

obj/%.o: third_party/re2/util/%.cc
	$(CCPP) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c third_party/re2/$(subst obj,util,$(subst .o,.cc,$@)) -o $@

obj/%.o: %.mm
	$(CCPP) $(OBJCFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst obj/,,$(subst .o,.mm,$@)) -o $@

obj/%.o: %.cpp
	$(CCPP) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst obj/,,$(subst .o,.cpp,$@)) -o $@

panther.app: 
	$(CCPP) $(OBJCFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OBJCOBJECTS) $(RE2OBJECTS) $(COBJECTS) $(OPTFLAGS) -o main.app
