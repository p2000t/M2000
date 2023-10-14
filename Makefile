default:
	@echo "To compile M2000, use one of the following:"
	@echo " make allegro - Make the Allegro version (requires Allegro5 libs)"
	@echo " make svga    - Make the Linux/SVGALib version"
	@echo " make x       - Make the Unix/X version"
	@echo " make msdos   - Make the MS-DOS version (DJGPP only)"
	@echo " make linux   - Build both the Linux/SVGALib and the Unix/X versions"
	@echo "When compiling the SVGALib version, make sure you are logged in as root"
	@echo "Please check Makefile.SVGALib, Makefile.X and Z80.h before compiling M2000"

svga:
	make -f Makefile.SVGALib

x:
	make -f Makefile.X

msdos:
	make -f Makefile.MSDOS

allegro:
	make -f Makefile.allegro

linux:
	make clean
	make x
	mv m2000 m2000x
	make clean
	make svga

clean:
ifeq ($(OS),Windows_NT)
	del /Q /F *.o 2>NUL
else
	rm *.o *~
endif

distclean:
	make clean
ifeq ($(OS),Windows_NT)
	del /Q /F z80dasm.exe fontc.exe m2000.exe splitape.exe P2000.cas 2>NUL
else
	rm z80dasm fontc m2000 splitape P2000.cas
endif
