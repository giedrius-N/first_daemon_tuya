.PHONY: clean all src

all: src

src:
	make -C src

clean:
	make -C src clean
