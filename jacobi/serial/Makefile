CC = icc
CFLAGS = -std=c99 -Wall -O3 -no-prec-div -xHOST -qopt-report=5 -qopt-report-routine=run -ansi-alias -restrict -vec-threshold0
LDFLAGS = -lm

jacobi:
	$(CC) $(CFLAGS) -prof-use -o jacobi jacobi.c #$(LDFLAGS)

jacobi_gen:
	$(CC) $(CFLAGS) -prof-gen -o jacobi jacobi.c #$(LDFLAGS)
