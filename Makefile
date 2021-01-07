canny_edge: *.c *.h
	gcc -m64 -o $@ $^ -lm -fopenmp