ifeq ($(OS),Windows_NT)
target_ext = .exe
endif

target = aiwt$(target_ext)

outdir = bin
srcdir = src
objdir = obj

srcs = $(wildcard $(srcdir)/*.cpp)
objs = $(patsubst $(srcdir)/%,$(objdir)/%,$(srcs:.cpp=.o))

cxxflags = -std=c++14 -Os -O3 -pedantic -Iinclude
ldflags = -s -Wall -Wextra

.PHONY: clean

all: $(outdir)/$(target)

# Compile
$(objdir)/%.o: $(srcdir)/%.cpp
	$(CXX) -c $< $(cxxflags) -o $@

# Link
$(outdir)/$(target): $(objs)
	$(CXX) $(objs) $(ldflags) -o $@

clean:
	rm -f $(objs) bin/$(target)