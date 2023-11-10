default:
	$(MAKE) -f Makefile.allegro

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
