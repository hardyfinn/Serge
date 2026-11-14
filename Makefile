# Serge for VCV Rack.
# Build:  export RACK_DIR=/path/to/Rack-SDK && make && make install   (or make dist)
# Tests:  make test   (host build of the DSP models, no Rack SDK needed)

RACK_DIR ?= ../..

FLAGS += -Isrc
CFLAGS +=
CXXFLAGS += -Wall -Wextra $(EXTRA_CXXFLAGS)
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp src/modules/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += README.md

ifneq ($(wildcard $(RACK_DIR)/plugin.mk),)
include $(RACK_DIR)/plugin.mk
endif

TEST_CXX ?= g++
build/serge_tests: $(wildcard tests/*.cpp tests/*.hpp src/dsp/*.hpp)
	@mkdir -p build
	$(TEST_CXX) -std=c++17 -O2 -Wall -Wextra -o $@ $(wildcard tests/*.cpp)

.PHONY: test panels
test: build/serge_tests
	./build/serge_tests

# Regenerate res/*.svg and src/generated/Layout.hpp (needs Python 3 and fonttools).
panels:
	python3 scripts/make_panels.py
