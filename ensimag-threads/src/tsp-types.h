#ifndef _TSP_TYPES_H
#define _TSP_TYPES_H
#include <stdbool.h>
#include <inttypes.h>

#define MAX_TOWNS 40

typedef int tsp_distance_matrix_t[MAX_TOWNS][MAX_TOWNS];
typedef int tsp_path_t[MAX_TOWNS];

#define MAXX	100
#define MAXY	100

/* tableau des distances */
extern tsp_distance_matrix_t tsp_distance;

/* parametres du programme */
extern int nb_towns;
extern long int myseed;
extern bool affiche_progress;
extern bool quiet;

typedef struct arguments {
	tsp_path_t *solution;
	int *solution_len;
	struct tsp_queue *queue;
	long long *cuts;
	uint64_t vpres;
} arguments;

#endif
