include buildinfo.inc
BUILD := $(shell expr $(BUILD) + 1)
DEBUG := 1
ifeq ($(DEBUG),1)
	BUILDFLAGS=-g3 -rdynamic
	BUILDNAME=Debug
	BUILDDIR=dbg
else
	BUILDFLAGS=-O3
	BUILDNAME=Release
	BUILDDIR=rls
endif

DEFINES := -DMAJORVER=\"$(MAJOR)\" -DMINORVER=\"$(MINOR)\" -DPATCHVER=\"$(PATCH)\" -DBUILDVER=\"$(BUILD)\" -DBUILDTYPE=\"$(BUILDNAME)\"
INCLUDES := -Iinc -I../libArducam/inc -I../libRESTServer/inc -I../libMMMUtil/inc
CXX      := g++
CXXFLAGS := -Wall -pedantic -std=c++11 $(BUILDFLAGS) $(DEFINES) $(INCLUDES)


SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=obj/$(BUILDDIR)/%.o)
LIBS	:= -lpthread -L../libArducam -lArducam -L../libRESTServer -lRESTServer -L../libMMMUtil -lMMMUtil
EXENAME := hawkeye

all: $(EXENAME) 


hawkeye: $(OBJECTS) ../libArducam/libArducam.a ../libRESTServer/libRESTServer.a
	@$(CXX) $(CXXFLAGS) -MMD -o $@ $(OBJECTS) $(LIBS)

obj/$(BUILDDIR)/%.o : src/%.cpp
	@echo -n "$* "
	@$(CXX) $(CXXFLAGS) -MMD -c -o $@  $<


-include $(SOURCES:src/%.cpp=obj/$(BUILDDIR)/%.d)

buildinfo.inc : $(OBJECTS)
	@echo "MAJOR :=$(MAJOR)" > buildinfo.inc
	@echo "MINOR :=$(MINOR)" >> buildinfo.inc
	@echo "PATCH :=$(PATCH)" >> buildinfo.inc
	@echo "BUILD :=$(BUILD)" >> buildinfo.inc

.PHONY: clean
clean:
	./clean.sh


