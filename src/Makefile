default:
	@echo "To compile M2000, use one of the following:"
	@echo " make allegro - Make the Allegro version (requires Allegro5 libs)"
	@echo " make x       - Make the Unix/X version"
	@echo " make msdos   - Make the MS-DOS version (DJGPP only)"

allegro:
	make -f Makefile.allegro

x:
	make -f Makefile.X

msdos:
	make -f Makefile.MSDOS

clean:
ifeq ($(OS),Windows_NT)
	del /Q /F *.o 2>NUL
else
	rm *.o *~
endif
