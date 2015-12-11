#include "gtmp.h"
#include <time.h>

static int count, sense;

void gtmp_init(int num_threads)
{
    omp_set_dynamic(0);
    omp_set_num_threads(num_threads);
    count = num_threads;
    sense = 1;
}

void gtmp_barrier(){
    static int local_sense;
    local_sense = !sense;
    if (__sync_fetch_and_sub(&count, 1) == 1)
    {
        count = omp_get_num_threads();
        sense = local_sense;
    }
    else
    {
        while(sense != local_sense);
    }
}

void gtmp_finalize(){
    omp_set_num_threads(1);
}

int main(int argc, char** argv)
{
	struct timespec tms;
	if(argc == 2)
	{
		int nb_threads = strtol(argv[1], NULL, 10);
		gtmp_init(nb_threads);
		if(clock_gettime(CLOCK_REALTIME, &tms)){
			return -1;
		}
		double t0 = (double) tms.tv_nsec/1000000000.0;

		#pragma omp parallel
		{
		    gtmp_barrier();
		}
		if(clock_gettime(CLOCK_REALTIME, &tms)){
			return -1;
		}
		double t1 = (double) tms.tv_nsec/1000000000.0;///tms.tv_sec;
		//t1 += tms.tv_nsec/1000000;
		t1 -= t0;
		printf("%lf", t1);

		gtmp_finalize();
	}
	else return 1;
	return 0;
}
