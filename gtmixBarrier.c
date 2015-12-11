#include "gtmix.h"

// ------------------ GTMPI Part -------------------------

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

//------------------- GTMP Part --------------------------

typedef struct _node_t
{
    int k;
    int count;
    int locksense;
    struct _node_t* parent;
}
node_t;

static int num_leaves;
static node_t* nodes;

void gtmix_barrier_aux(node_t* node, int sense);

node_t* _gtmp_get_node(int i){
    return &nodes[i];
}

void gtmp_init(int num_threads){

    int i, v, num_nodes;
    node_t* curnode;

    /* Setting constants */
    v = 1;
    while( v < num_threads) v *= 2;

    num_nodes = v - 1;
    num_leaves = v/2;

    /* Setting up the tree */
    nodes = (node_t*) malloc(num_nodes * sizeof(node_t));

    for(i = 0; i < num_nodes; i++){
        curnode = _gtmp_get_node(i);
        curnode->k = i < num_threads - 1 ? 2 : 1;
        curnode->count = curnode->k;
        curnode->locksense = 0;
        curnode->parent = _gtmp_get_node((i-1)/2);
    }

    curnode = _gtmp_get_node(0);
    curnode->parent = NULL;
}

void gtmp_finalize(){
  free(nodes);
}

//--------------------------- GTmix Part --------------------------

void gtmix_init(int num_proc, int num_threads){
    gtmpi_init(num_proc);
    gtmp_init(num_threads);
}

void gtmix_barrier(){
  node_t* mynode;
  int sense;
  int num_threads = omp_get_thread_num();
  mynode = _gtmp_get_node(num_leaves - 1 + (num_threads % num_leaves));
  sense = !mynode->locksense;
  gtmix_barrier_aux(mynode, sense);
}

void gtmix_barrier_aux(node_t* node, int sense)
{
    if(1 == __sync_fetch_and_sub(&node->count, 1))
    {
        if(node->parent != NULL)
        gtmix_barrier_aux(node->parent, sense);
	else gtmpi_barrier();
        node->count = node->k;
        node->locksense = !node->locksense;
    }
    while (node->locksense != sense);
}

void gtmix_finalize()
{
	gtmp_finalize();
	gtmpi_finalize();
}

//--------------------------- Main Part ---------------------------

int main(int argc, char** argv)
{
    if(argc == 3)
    {
        int nb_proc = strtol(argv[1], NULL, 10);
	int nb_threads = strtol(argv[1], NULL, 10);
        double res, res2;
	int rang;

        gtmix_init(nb_proc, nb_threads);
        MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	if(rang==0) res = MPI_Wtime();
	#pragma omp parallel
        {
            gtmix_barrier();
        }
	if(rang==0) 
	{
		res2 = MPI_Wtime() - res;
		printf("%lf", res2);
	}
        gtmix_finalize();
    }
    else return 1;
    return 0;
}
