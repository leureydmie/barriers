#include "gtmp.h"
#include <time.h>

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

void gtmp_barrier_aux(node_t* node, int sense);

node_t* _gtmp_get_node(int i){
    return &nodes[i];
}

void gtmp_init(int num_threads){

    int i, v, num_nodes;
    node_t* curnode;

    /*Setting constants */
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

void gtmp_barrier(){
  node_t* mynode;
  int sense;
  int num_threads = omp_get_thread_num();

  mynode = _gtmp_get_node(num_leaves - 1 + (num_threads % num_leaves));

  /*
     Rather than correct the sense variable after the call to
     the auxilliary method, we set it correctly before.
   */
  sense = !mynode->locksense;

  gtmp_barrier_aux(mynode, sense);
}

void gtmp_barrier_aux(node_t* node, int sense)
{
    if( 1 == __sync_fetch_and_sub(&node->count, 1) )
    {
        if(node->parent != NULL) gtmp_barrier_aux(node->parent, sense);
        node->count = node->k;
        node->locksense = !node->locksense;
    }
    while (node->locksense != sense);
}

void gtmp_finalize(){
  free(nodes);
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
		//t0 += tms.tv_nsec/1000000;

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
