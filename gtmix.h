#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "gtmpi.h"
#include "gtmp.h"

#include <omp.h>
#include <mpi.h>

void gtmix_init(int, int);
void gtmix_barrier(void);
void gtmix_finalize(void);
