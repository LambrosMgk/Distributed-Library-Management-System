MPICC?=mpicc

# Reminder: $@ is an automatic variable that contains the target name(s).
#			$^ contains all prerequisites.

# Rule names
TARGET = main

all: $(TARGET)

# Rules to create executables
main: main.c ansi-color-codes.h my_funcs.c my_funcs.h client.c client.h server.c server.h book.h
	$(MPICC) -o $@ $^ -lm

# Clean rule to remove executables
clean:
	rm -f $(TARGET)