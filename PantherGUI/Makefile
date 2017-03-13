

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
INCLUDE_FLAGS=$(SKIA_INCLUDE_FLAGS) $(ICU_INCLUDE_FLAGS) -Irust/include -Ithird_party/re2


COBJECTS=$(OBJDIR)/basictextfield.o \
		 $(OBJDIR)/button.o \
		 $(OBJDIR)/c.o \
		 $(OBJDIR)/container.o \
		 $(OBJDIR)/control.o \
		 $(OBJDIR)/controlmanager.o \
		 $(OBJDIR)/cursor.o \
		 $(OBJDIR)/directory.o \
		 $(OBJDIR)/encoding.o \
		 $(OBJDIR)/findresults.o \
		 $(OBJDIR)/file.o \
		 $(OBJDIR)/filemanager.o \
		 $(OBJDIR)/findtext.o \
		 $(OBJDIR)/goto.o \
		 $(OBJDIR)/keybindings.o \
		 $(OBJDIR)/keywords.o \
		 $(OBJDIR)/language.o \
		 $(OBJDIR)/logger.o \
		 $(OBJDIR)/notification.o \
		 $(OBJDIR)/projectexplorer.o \
		 $(OBJDIR)/regex.o \
		 $(OBJDIR)/renderer.o \
		 $(OBJDIR)/scheduler.o \
		 $(OBJDIR)/scrollbar.o \
		 $(OBJDIR)/searchbox.o \
		 $(OBJDIR)/settings.o \
		 $(OBJDIR)/simpletextfield.o \
		 $(OBJDIR)/splitter.o \
		 $(OBJDIR)/statusbar.o \
		 $(OBJDIR)/style.o \
		 $(OBJDIR)/syntaxhighlighter.o \
		 $(OBJDIR)/tabcontrol.o \
		 $(OBJDIR)/text.o \
		 $(OBJDIR)/textbuffer.o \
		 $(OBJDIR)/textdelta.o \
		 $(OBJDIR)/textfield.o \
		 $(OBJDIR)/textfieldcontainer.o \
		 $(OBJDIR)/textfile.o \
		 $(OBJDIR)/textiterator.o \
		 $(OBJDIR)/textline.o \
		 $(OBJDIR)/textposition.o \
		 $(OBJDIR)/thread.o \
		 $(OBJDIR)/togglebutton.o \
		 $(OBJDIR)/unicode.o \
		 $(OBJDIR)/utils.o \
		 $(OBJDIR)/windowfunctions.o \
		 $(OBJDIR)/workspace.o \
		 $(OBJDIR)/wrappedtextiterator.o \
		 $(OBJDIR)/xml.o 

OBJCOBJECTS=$(OBJDIR)/AppDelegate.o \
			$(OBJDIR)/PGView.o \
			$(OBJDIR)/PGWindow.o

RE2OBJECTS=$(OBJDIR)/bitstate.o \
		   $(OBJDIR)/compile.o \
		   $(OBJDIR)/dfa.o \
		   $(OBJDIR)/filtered_re2.o \
		   $(OBJDIR)/mimics_pcre.o \
		   $(OBJDIR)/nfa.o \
		   $(OBJDIR)/onepass.o \
		   $(OBJDIR)/parse.o \
		   $(OBJDIR)/perl_groups.o \
		   $(OBJDIR)/prefilter.o \
		   $(OBJDIR)/prefilter_tree.o \
		   $(OBJDIR)/prog.o \
		   $(OBJDIR)/re2.o \
		   $(OBJDIR)/regexp.o \
		   $(OBJDIR)/rune.o \
		   $(OBJDIR)/set.o \
		   $(OBJDIR)/simplify.o \
		   $(OBJDIR)/stringpiece.o \
		   $(OBJDIR)/strutil.o \
		   $(OBJDIR)/tostring.o \
		   $(OBJDIR)/unicode_casefold.o \
		   $(OBJDIR)/unicode_groups.o

MAINOBJECTS=$(OBJDIR)/main.o

TESTOBJECTS=$(OBJDIR)/testmain.o \
			$(OBJDIR)/tester.o

all: create_build_directory $(RE2OBJECTS) $(COBJECTS) $(OBJCOBJECTS) $(MAINOBJECTS) rust.app panther.app

test: create_build_directory $(RE2OBJECTS) $(COBJECTS) $(OBJCOBJECTS) $(TESTOBJECTS) rust.app test_output


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

$(OBJDIR)/%.o: third_party/re2/re2/%.cc
	$(CCPP) $(CPPFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(INCLUDE_FLAGS) $(OPTFLAGS) -c third_party/re2/$(subst $(OBJDIR),re2,$(subst .o,.cc,$@)) -o $@

$(OBJDIR)/%.o: third_party/re2/util/%.cc
	$(CCPP) $(CPPFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(INCLUDE_FLAGS) $(OPTFLAGS) -c third_party/re2/$(subst $(OBJDIR),util,$(subst .o,.cc,$@)) -o $@

$(OBJDIR)/%.o: %.mm
	$(CCPP) $(OBJCFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst $(OBJDIR)/,,$(subst .o,.mm,$@)) -o $@

$(OBJDIR)/%.o: %.cpp
	$(CCPP) $(CPPFLAGS) -MMD -MF $(subst $(OBJDIR),$(DEPSDIR),$(subst .o,.d,$@)) $(INCLUDE_FLAGS) $(OPTFLAGS) -c $(subst $(OBJDIR)/,,$(subst .o,.cpp,$@)) -o $@

test_output:
	$(CCPP) $(OBJCFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OBJCOBJECTS) $(RE2OBJECTS) $(COBJECTS) $(TESTOBJECTS) $(OPTFLAGS) -o test.out

panther.app: 
	$(CCPP) $(OBJCFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(OBJCOBJECTS) $(RE2OBJECTS) $(MAINOBJECTS) $(COBJECTS) $(OPTFLAGS) -o main.app
