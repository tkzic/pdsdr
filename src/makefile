current: d_fat

# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo -D_CRT_SECURE_NO_WARNINGS
# VC="C:\Program Files\Microsoft Visual Studio\Vc98"
VC = "C:\Program Files\Microsoft Visual Studio 9.0\VC"
VSTK = "C:\Program Files\Microsoft SDKs\Windows\v6.0A"

PDNTINCLUDE = /I. /I..\..\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = /NODEFAULTLIB:libcmt /NODEFAULTLIB:oldnames /NODEFAULTLIB:kernel32 \
	$(PDNTLDIR)\libcmt.lib $(PDNTLDIR)\oldnames.lib \
        $(VSTK)\lib\kernel32.lib \
	 ..\..\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /nologo /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(CFLAGS)

LINUXINCLUDE =  -I../../src

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	$(CC) -shared -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o

# ----------------------- Mac OSX -----------------------

d_ppc: $(NAME).d_ppc
d_fat: $(NAME).d_fat

.SUFFIXES: .d_ppc .d_fat

#DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
#    -Wno-unused -Wno-parentheses -Wno-switch
# compile with debugging information
DARWINCFLAGS = -DPD -g -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.d_ppc:
	$(CC) $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	$(CC) -bundle -undefined suppress -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o

#
# tz 1/2014 changed code below to remove the ppc option because it wouldn't compile
# also changed file extension to pd_darwin instead of d_fat
#
.c.d_fat:
#	$(CC) -arch i386 -arch ppc $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
#	$(CC) -arch i386 -arch ppc -bundle -undefined suppress -flat_namespace \
	    -o $*.d_fat $*.o 
	$(CC) -arch i386  $(DARWINCFLAGS) $(LINUXINCLUDE) -c $*.c $(LAMA)
#	$(CC) -arch i386  -bundle -undefined suppress -flat_namespace \
		    -o $*.d_fat $*.o
	$(CC) -arch i386  -bundle -undefined suppress -flat_namespace \
				    -o $*.pd_darwin $*.o $(LAMO) /usr/local/lib/libusb.dylib /usr/local/lib/libftdi.dylib
	rm -f $*.o 

# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
