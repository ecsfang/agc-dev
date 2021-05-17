IDIR =.
CC=g++
CFLAGS=-I$(IDIR)

ODIR=obj

LIBS=-lncurses

_DEPS = cpu.h memory.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = agc.o memory.o cpu_dis.o cpu_inst.o cpu_opc.o cpu_opex.o cpu_imu.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

agc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
