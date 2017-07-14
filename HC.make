
HCC = hcc

CXXFLAGS+=-O3 $(shell hcc-config --cxxflags)
LDFLAGS+=-lhc_am $(shell hcc-config --ldflags)

hc-stream: main.cpp HCStream.cpp
	$(HCC) $(CXXFLAGS) -DHC $^ $(LDFLAGS) -g -o $@

.PHONY: clean
clean:
	rm -f hc-stream
