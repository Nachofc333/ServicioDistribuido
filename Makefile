# Makefile para compilar cliente, servidor y crear libclaves.so
# Nombres de los archivos binarios
BIN_FILES = servidor

# Compilador y opciones
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INSTALL_PATH = /ruta/a/tu/install/path
CPPFLAGS = -I$(INSTALL_PATH)/include
LDFLAGS = -L$(INSTALL_PATH)/lib/
LDLIBS = -lpthread -lrt

# Regla para compilar todo
all: $(BIN_FILES)
.PHONY: all


servidor: servidor.o lines.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Regla para compilar archivos fuente a objetos
%.o: %.c 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<


# Regla para limpiar archivos generados
clean:
	rm -f $(BIN_FILES) *.o
.PHONY: clean
