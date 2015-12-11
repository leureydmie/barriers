#include "gtmpi.h"

typedef struct flags
{
    int myFlags[2];
    int** partnerFlags[2];
} flags;

static flags localflags;

void gtmpi_init(int num_threads){

    MPI_Init(NULL, NULL);

    //Flags initialization
    localflags.myFlags[0] = -1;
    localflags.myFlags[1] = -1;

    //fprintf(stderr,"Processus %d parmi %d processus finit Init proc.\n", myid, numprocs);
}

void gtmpi_barrier(){
    int  numprocs, myid, i;
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);

    int L = log(numprocs-1)/log(2) + 1;

    int parity = 0;
    int sense = 1;

    //fprintf(stderr,"Processus %d parmi %d processus.\n", myid, numprocs);

    for (i = 0; i < L; i++)
    {
        unsigned int j = (myid + (int) pow(2,i)) % numprocs;
        unsigned int k = (myid + numprocs - (int) pow(2,i)) % numprocs;
        //printf("Processus %d envoie %d au processus %d\n", myid, sense, j);
        MPI_Send(&sense, 1, MPI_INT, j, 0, MPI_COMM_WORLD);

        while (localflags.myFlags[parity] != sense)
        {
            MPI_Recv(&localflags.myFlags[parity], 1, MPI_INT, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Process %d received number %d from process %d\n", myid, localflags.myFlags[parity], k);
        }
        sense = !sense;
    }
    if (parity == 1)
    {
        sense = !sense;
    }
    parity ^= 0x1;
}

void gtmpi_finalize(){
    MPI_Finalize();
}

int main(int argc, char **argv)
{
    if(argc == 2)
    {
        int nb_threads = strtol(argv[1], NULL, 10);
	double res, res2;
	int rang;

        gtmpi_init(nb_threads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	if(rang==0) res = MPI_Wtime();
	gtmpi_barrier();
	if(rang==0) 
	{
		res2 = MPI_Wtime() - res;
		printf("%lf", res2);
	}
        gtmpi_finalize();
    }
    else return 1;
    return 0;
}
