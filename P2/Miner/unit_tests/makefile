EXE2=files_test

$(EXE2): files_test.o files.o logger.o pow.o
	$(CC) -o $(EXE2) $^

run: $(EXE2)
	valgrind --leak-check=full ./$(EXE2)
