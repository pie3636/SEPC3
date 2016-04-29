#include <assert.h>
#include <string.h>

#include "tsp-types.h"
#include "tsp-genmap.h"
#include "tsp-print.h"
#include "tsp-tsp.h"
#include "tsp-lp.h"
#include "tsp-hkbound.h"
#include "tsp-job.h"

pthread_mutex_t mtxJob;
pthread_mutex_t mtxMinCut;

/* dernier minimum trouvé */
int minimum;

/* résolution du problème du voyageur de commerce */
int present(int city, int hops, tsp_path_t path, uint64_t vpres) {
	(void)hops;
	(void)path;
	return (vpres & (1 << city)) != 0;
}

int tsp(int hops, int len, uint64_t vpres, tsp_path_t path,
	 long long int *cuts, tsp_path_t sol, int *sol_len, int minimum) {
	if (len + cutprefix[(nb_towns - hops)] >= minimum) {
		(*cuts)++;
		return minimum;
	}

	/* calcul de l'arbre couvrant comme borne inférieure */
	if ((nb_towns - hops) > 6 &&
	    lower_bound_using_hk(path, hops, len, vpres) >= minimum) {
		(*cuts)++;
		return minimum;
	}

	/* un rayon de coupure à 15, pour ne pas lancer la programmation
	   linéaire pour les petits arbres, plus rapide à calculer sans */
	if ((nb_towns - hops) > 22
	    && lower_bound_using_lp(path, hops, len, vpres) >= minimum) {
		(*cuts)++;
		return minimum;
	}

	if (hops == nb_towns) {
		int me = path[hops - 1];
		int dist = tsp_distance[me][0];	// retourner en 0
		if (len + dist < minimum) {
			minimum = len + dist;
			*sol_len = len + dist;
			memcpy(sol, path, nb_towns * sizeof(int));
			if (!quiet)
				print_solution(path, len + dist);
		}
	} else {
		int me = path[hops - 1];
		for (int i = 0; i < nb_towns; i++) {
			if (!present(i, hops, path, vpres)) {
				path[hops] = i;
				vpres |= (1 << i);
				int dist = tsp_distance[me][i];
				minimum = tsp(hops + 1, len + dist, vpres, path, cuts,
				    sol, sol_len, minimum);
				vpres &= (~(1 << i));
			}
		}
	}
	return minimum;
}

void *tsp2(void *args) {
	arguments *myArgs = args;
	struct tsp_queue *q = myArgs->queue;
	tsp_path_t *solution = myArgs->solution;
	int *solution_len = myArgs->solution_len;
	uint64_t vpres = myArgs->vpres;
	tsp_path_t path;
	long long *cuts = myArgs->cuts;
	
	int steps = 0;
	int localMin = minimum, localSolution_len = 0;
	long long localCuts = 0;
	tsp_path_t localSolution;
	
	int copySize = nb_towns * sizeof(int);

	while(!empty_queue(q)) {
		int len = 0, hops = 0;
		pthread_mutex_lock(&mtxJob);
		get_job(q, path, &hops, &len, &vpres);
		pthread_mutex_unlock(&mtxJob);
		
		int minimum_2;
		int solution_len_2 = *solution_len;
		long long cuts_2 = 0;
		tsp_path_t solution_2;
		
		minimum_2 = tsp(hops, len, vpres, path, &cuts_2, solution_2, &solution_len_2, minimum);

		localCuts = cuts_2;
		if(minimum_2 < localMin) {
			localMin = minimum_2;
			localSolution_len = solution_len_2;
			memcpy(&localSolution, solution_2, copySize);
			steps++;
			if (steps == 1) {
				pthread_mutex_lock(&mtxMinCut);
				if (localMin < minimum) {
					steps = 0;
					*solution_len = localSolution_len;
					*cuts += localCuts;
					localCuts = 0;
					memcpy(solution, &localSolution, copySize);
					minimum = localMin;
				}
				pthread_mutex_unlock(&mtxMinCut);
			}
		}
	}
	pthread_mutex_lock(&mtxMinCut);
	if (localMin < minimum && steps != 0) {
		*solution_len = localSolution_len;
		*cuts += localCuts;
		memcpy(solution, &localSolution, copySize);
		minimum = localMin;
	}
	pthread_mutex_unlock(&mtxMinCut);
	return NULL;
}
