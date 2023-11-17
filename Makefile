all: build

build:
	$(MAKE) -C src all

# test: build
# 	$(MAKE) -C test all
	
clean:
	$(MAKE) -C src clean