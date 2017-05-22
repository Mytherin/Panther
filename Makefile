

OPTIMIZE=$(OPT)

CCPP=clang++

ifneq ($(OPTIMIZE), true)
	OPTFLAGS=-O0 -g
	OBJDIR=obj
else
	OPTFLAGS=-O3 -g
	OBJDIR=opt
endif

DEPSDIR=$(OBJDIR)/deps

PANTHER_INCLUDE_FLAGS=-Ifiles -Ios -Ios/macos -Irust/include -Isettings -Isyntax -Isyntax/languages \
			  -Itesting -Itext -Ithird_party/concurrent_queue -Ithird_party/json \
			  -Ithird_party/re2 -Iui -Iui/controls \
			  -Iui/controls/basic -Iui/controls/main -Iui/controls/textfield -Iutils
SKIA_INCLUDE_FLAGS=-I/Users/myth/Sources/skia/skia/include/core \
				   -I/Users/myth/Sources/skia/skia/include/config \
				   -I/Users/myth/Sources/skia/skia/include/codec \
				   -I/Users/myth/Sources/skia/skia/include/effects
SKIA_LDFLAGS=-L/Users/myth/Sources/skia/skia/out/Static-Release-PNG -lskia
ICU_INCLUDE_FLAGS=
ICU_LDFLAGS=-licuuc -licutu -liculx -licuio -licule -licudata -licui18n

CPPFLAGS=-std=c++11
OBJCFLAGS=-fobjc-arc

ifneq ($(OPTIMIZE), true)
	CPPFLAGS+=-DPANTHER_DEBUG -DPANTHER_TESTS  -fsanitize=address
	CARGO_FLAGS=
	RUST_LDFLAGS=-Lrust/target/debug -lpgrust
else
	CARGO_FLAGS=--release
	RUST_LDFLAGS=-Lrust/target/release -lpgrust
endif

LDFLAGS=-framework AppKit -lobjc -L/opt/local/lib $(ICU_LDFLAGS) $(SKIA_LDFLAGS) $(RUST_LDFLAGS)
INCLUDE_FLAGS=$(SKIA_INCLUDE_FLAGS) $(ICU_INCLUDE_FLAGS) $(PANTHER_INCLUDE_FLAGS)


COBJECTS=$(OBJDIR)/files/directory.o \
		$(OBJDIR)/files/file.o \
		$(OBJDIR)/files/filemanager.o \
		$(OBJDIR)/files/searchindex.o \
		$(OBJDIR)/os/windowfunctions.o \
		$(OBJDIR)/settings/globalsettings.o \
		$(OBJDIR)/settings/keybindings.o \
		$(OBJDIR)/settings/settings.o \
		$(OBJDIR)/settings/style.o \
		$(OBJDIR)/settings/workspace.o \
		$(OBJDIR)/syntax/language.o \
		$(OBJDIR)/syntax/languages/c.o \
		$(OBJDIR)/syntax/languages/findresults.o \
		$(OBJDIR)/syntax/languages/keywords.o \
		$(OBJDIR)/syntax/languages/xml.o \
		$(OBJDIR)/syntax/syntaxhighlighter.o \
		$(OBJDIR)/testing/replaymanager.o \
		$(OBJDIR)/text/cursor.o \
		$(OBJDIR)/text/encoding.o \
		$(OBJDIR)/text/findtextmanager.o \
		$(OBJDIR)/text/regex.o \
		$(OBJDIR)/text/text.o \
		$(OBJDIR)/text/textbuffer.o \
		$(OBJDIR)/text/textdelta.o \
		$(OBJDIR)/text/textfile.o \
		$(OBJDIR)/text/textiterator.o \
		$(OBJDIR)/text/textline.o \
		$(OBJDIR)/text/textposition.o \
		$(OBJDIR)/text/textview.o \
		$(OBJDIR)/text/unicode.o \
		$(OBJDIR)/text/wrappedtextiterator.o \
		$(OBJDIR)/ui/controls/basic/button.o \
		$(OBJDIR)/ui/controls/basic/decoratedscrollbar.o \
		$(OBJDIR)/ui/controls/basic/label.o \
		$(OBJDIR)/ui/controls/basic/scrollbar.o \
		$(OBJDIR)/ui/controls/basic/splitter.o \
		$(OBJDIR)/ui/controls/basic/togglebutton.o \
		$(OBJDIR)/ui/controls/container.o \
		$(OBJDIR)/ui/controls/control.o \
		$(OBJDIR)/ui/controls/controlmanager.o \
		$(OBJDIR)/ui/controls/main/findtext.o \
		$(OBJDIR)/ui/controls/main/goto.o \
		$(OBJDIR)/ui/controls/main/projectexplorer.o \
		$(OBJDIR)/ui/controls/main/searchbox.o \
		$(OBJDIR)/ui/controls/main/statusbar.o \
		$(OBJDIR)/ui/controls/main/statusnotification.o \
		$(OBJDIR)/ui/controls/main/toolbar.o \
		$(OBJDIR)/ui/controls/notification.o \
		$(OBJDIR)/ui/controls/textfield/basictextfield.o \
		$(OBJDIR)/ui/controls/textfield/simpletextfield.o \
		$(OBJDIR)/ui/controls/textfield/tabcontrol.o \
		$(OBJDIR)/ui/controls/textfield/textfield.o \
		$(OBJDIR)/ui/controls/textfield/textfieldcontainer.o \
		$(OBJDIR)/ui/renderer.o \
		$(OBJDIR)/utils/logger.o \
		$(OBJDIR)/utils/scheduler.o \
		$(OBJDIR)/utils/thread.o \
		$(OBJDIR)/utils/utils.o

OBJCOBJECTS=$(OBJDIR)/os/macos/AppDelegate.o \
			$(OBJDIR)/os/macos/main.o \
			$(OBJDIR)/os/macos/PGView.o \
			$(OBJDIR)/os/macos/PGWindow.o

RE2OBJECTS=$(OBJDIR)/third_party/re2/re2/bitstate.o \
		$(OBJDIR)/third_party/re2/re2/compile.o \
		$(OBJDIR)/third_party/re2/re2/dfa.o \
		$(OBJDIR)/third_party/re2/re2/filtered_re2.o \
		$(OBJDIR)/third_party/re2/re2/mimics_pcre.o \
		$(OBJDIR)/third_party/re2/re2/nfa.o \
		$(OBJDIR)/third_party/re2/re2/onepass.o \
		$(OBJDIR)/third_party/re2/re2/parse.o \
		$(OBJDIR)/third_party/re2/re2/perl_groups.o \
		$(OBJDIR)/third_party/re2/re2/prefilter.o \
		$(OBJDIR)/third_party/re2/re2/prefilter_tree.o \
		$(OBJDIR)/third_party/re2/re2/prog.o \
		$(OBJDIR)/third_party/re2/re2/re2.o \
		$(OBJDIR)/third_party/re2/re2/regexp.o \
		$(OBJDIR)/third_party/re2/re2/set.o \
		$(OBJDIR)/third_party/re2/re2/simplify.o \
		$(OBJDIR)/third_party/re2/re2/stringpiece.o \
		$(OBJDIR)/third_party/re2/re2/tostring.o \
		$(OBJDIR)/third_party/re2/re2/unicode_casefold.o \
		$(OBJDIR)/third_party/re2/re2/unicode_groups.o \
		$(OBJDIR)/third_party/re2/util/rune.o \
		$(OBJDIR)/third_party/re2/util/strutil.o

#MAINOBJECTS=$(OBJDIR)/main.o

#TESTOBJECTS=$(OBJDIR)/testmain.o \
			$(OBJDIR)/tester.o

.PHONY: all clean test create_build_directory rust.app panther.app

all: create_build_directory $(RE2OBJECTS) $(COBJECTS) $(OBJCOBJECTS) $(MAINOBJECTS) rust.app panther.app

#test: create_build_directory $(RE2OBJECTS) $(COBJECTS) $(OBJCOBJECTS) $(TESTOBJECTS) rust.app test_output


clean:
	rm -rf obj
	rm -rf opt
	rm -f main.app

create_build_directory:
	mkdir -p $(OBJDIR)
	mkdir -p $(DEPSDIR)


DEPS = $(shell find $(DEPSDIR) -name "*.d")
-include $(DEPS)

rust.app:
	cd rust && cargo build $(CARGO_FLAGS) && cd ..

$(OBJDIR)/%.o: %.cc
	mkdir -p $(shell dirname $@)
	mkdir -p $(subst $(OBJDIR),$(DEPSDIR),$(shell dirname $@))
	$(CCPP) $(CPPFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst $(OBJDIR)/,,$(subst .o,.cc,$@)) -o $@

$(OBJDIR)/%.o: %.mm
	mkdir -p $(shell dirname $@)
	mkdir -p $(subst $(OBJDIR),$(DEPSDIR),$(shell dirname $@))
	$(CCPP) $(OBJCFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst $(OBJDIR)/,,$(subst .o,.mm,$@)) -o $@

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(shell dirname $@)
	mkdir -p $(subst $(OBJDIR),$(DEPSDIR),$(shell dirname $@))
	$(CCPP) $(CPPFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst $(OBJDIR)/,,$(subst .o,.cpp,$@)) -o $@

test_output:
	$(CCPP) $(OBJCFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OBJCOBJECTS) $(RE2OBJECTS) $(COBJECTS) $(TESTOBJECTS) $(OPTFLAGS) -o test.out

panther.app: 
	$(CCPP) $(OBJCFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OBJCOBJECTS) $(RE2OBJECTS) $(MAINOBJECTS) $(COBJECTS) $(OPTFLAGS) -o main.app
