CXX ?= clang
HALIDE_PATH = /home/lar/src/Halide
# HALIDE_PATH = /afs/cs/academic/class/15769-f16/assignments/Halide
CUDA_PATH = /usr/local/cuda
CXXFLAGS += -O3 -g -Wall -std=c++11

HALIDE_INC += -I$(HALIDE_PATH)/include -I$(HALIDE_PATH)/tools
CUDA_INC += -I$(CUDA_PATH)/include

LDFLAGS += $(HALIDE_PATH)/lib/libHalide.a -L$(HALIDE_PATH)/bin -lpng -lz -lcurses -lpthread -ldl

all:
	$(CXX) $(CXXFLAGS) HDRPlus.cpp align.cpp merge.cpp finish.cpp util.cpp $(HALIDE_INC) $(LDFLAGS) -o hdrplus

clean:
	rm -rf hdrplus
