default:
	@echo "To compile M2000, use one of the following:"
	@echo " make allegro - Make the Allegro version (requires Allegro5 libs)"
	@echo " make x       - Make the Unix/X version"
	@echo " make dos     - Make the MS-DOS version (DJGPP only)"

allegro:
	$(MAKE) -f Makefile.allegro

x:
	$(MAKE) -f Makefile.X

dos:
	$(MAKE) -f Makefile.dos

clean:
ifeq ($(OS),Windows_NT)
	del /Q /F *.o *.MAP 2>NUL
else
ifneq ($(DJGPP),)
	del *.o >NUL
	del *.map >NUL
else
	rm -f *.o *~ *.MAP 
endif
endif
