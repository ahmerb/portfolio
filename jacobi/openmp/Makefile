CC = icc
CFLAGS = -std=c99 -Wall -O3 -no-prec-div -xHOST -qopt-report=5 -ansi-alias -restrict -vec-threshold0 -qopenmp #-qopt-report-routine=run
LDFLAGS = -lm

jacobi: jacobi.c
	$(CC) $(CFLAGS) -o jacobi jacobi.c $(LDFLAGS) #(removed --prof-use for now)

# jacobi_gen:
# 	$(CC) $(CFLAGS) -prof-gen -o jacobi jacobi.c #$(LDFLAGS)
