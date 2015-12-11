#include "gtmpi.h"

void gtmpi_init(int num_threads){
    MPI_Init(NULL, NULL);
}

void gtmpi_barrier()
{
    int  numprocs, myid, i, compete;
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);

    int nb_rounds = log(numprocs)/log(2);
    int parity = myid % 2;
    int sense = 0;
    int already_sent = 0;

    if(parity == 1)
    {
        compete = 0;
        sense = 1;
    }
    else
    {
        compete = 1;
        sense = 0;
    }

    for(i=0;i<nb_rounds;i++)
    {
        int j = (int) pow(2,i);
        if (!compete && !already_sent)
        {
            int k = myid - j;
            MPI_Send(&sense, 1, MPI_INT, k, 0, MPI_COMM_WORLD);
            //printf("Process %d sends %d to process %d\n", myid, sense, k);
            already_sent = !already_sent;
        }

        else if (compete)
        {
            int k = (myid + j) % numprocs;
            while(sense != 1) MPI_Recv(&sense, 1, MPI_INT, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Process %d received number %d from process %d\n", myid, sense, k);
            //Prepare for next match
            j*=2;
            parity = (myid / j) % 2;
            if(parity == 1) compete = !compete;
            else sense = !sense;
        }
    }

    for(i=nb_rounds-1;i>=0;i--)
    {
        int j = (int) pow(2,i);
        int parity = (myid % j == 0) ? 1 : 0;
        if (compete)
        {
            int k = (myid + j) % numprocs;
            MPI_Send(&sense, 1, MPI_INT, k, 0, MPI_COMM_WORLD);
            //printf("Process %d sends %d to process %d\n", myid, sense, k);
        }
        else if(parity)
        {
            int k = (myid - j);
            while(sense != 0) MPI_Recv(&sense, 1, MPI_INT, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Process %d received number %d from process %d\n", myid, sense, k);
            //Prepare for next match
            compete = !compete;
        }
    }
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
