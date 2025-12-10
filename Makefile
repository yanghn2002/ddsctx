CC ?= ccc
CXX ?= c++
IDLC ?= idlc
BUILD_DIR ?= $(shell pwd)/build

CFLAGS := -I$(BUILD_DIR)
CXXFLAGS := -std=c++17
LDFLAGS := -lddsc

build: libddsctx.so

demo: libddsctx.so demo.o demo.c
	$(CC) demo.c $(BUILD_DIR)/demo.o -o $(BUILD_DIR)/demo $(CFLAGS) -L$(BUILD_DIR) -lddsctx $(LDFLAGS)
	@rm -fv $(BUILD_DIR)/pub $(BUILD_DIR)/sub
	@ln -sv $(BUILD_DIR)/demo $(BUILD_DIR)/pub
	@ln -sv $(BUILD_DIR)/demo $(BUILD_DIR)/sub
	@echo "RUN SUB: 'LD_LIBRARY_PATH=$(shell pwd)/build PATH=$(shell pwd)/build sub'"
	@echo "RUN PUB: 'LD_LIBRARY_PATH=$(shell pwd)/build PATH=$(shell pwd)/build pub'"

demo.o: $(BUILD_DIR) demo.idl
	$(IDLC) demo.idl -o $(BUILD_DIR)
	$(CC) -c $(BUILD_DIR)/demo.c -o $(BUILD_DIR)/demo.o

libddsctx.so: $(BUILD_DIR) ddsctx.hpp
	$(CXX) -fPIC ddsctx.cpp $(CXXFLAGS) -o $(BUILD_DIR)/libddsctx.so -shared $(LDFLAGS)

$(BUILD_DIR):
	@rm -rfv $(BUILD_DIR)
	@mkdir -pv $(BUILD_DIR)

clean:
	@rm -rfv $(BUILD_DIR) pub sub libddsctx.so

.PHONY: build clean
