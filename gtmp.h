#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include <omp.h>

void gtmp_init(int);
void gtmp_barrier(void);
void gtmp_finalize(void);
