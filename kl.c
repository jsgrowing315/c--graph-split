#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kl.h"


void input_gen(FILE *output, int D, int V);
int cut(Graph *G, Vertex *a[], Vertex *b[], FILE *output);
void devide(Graph *G, Vertex *a[], Vertex *b[]);

int main(int argc, char ** argv){
	if(argc == 1) {
	   printf("Please input Matrix Market file path\n");	
	   return 0; 
	}
	FILE *input, *output; 
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		int line_n = 0, V, E, (*elist)[2], i, j, (*epair)[2] ;
		Vertex **vlist, **a, **b ;
		Graph *G, *G2 ;
		

		input = fopen(argv[1], "r");
		if (input == NULL){
			exit(EXIT_FAILURE);}
        //// read Matrix Market file and construct Graph 
		while ((read = getline(&line, &len, input)) != -1) {
			if(line[0] == '%') continue; /// this line is comment line
			int num1, num2 ;  /// source vertex number and target vertex number
			char *saveptr ;
			char *str1 = strtok_r(line, " \t\v\f\r", &saveptr), *str2 = strtok_r(NULL, " \t\v\f\r", &saveptr)  ;
			if(str1 != NULL && str2 != NULL ){
				num1 = atoi( str1 );
				num2 = atoi( str2 );
			}else{
				continue;
			}
			if(line_n == 0){
				/// this line has vertices' count and edges' count. 
				/// then allocate memory to save vertices and edges infos.
				V = num1 ;
				E = num2 ;
				vlist = (Vertex **)calloc(V + 1, sizeof(Vertex *)) ;
				elist = (int (*)[2])calloc(E + 1, sizeof *elist) ;
				epair = (int (*)[2])calloc(E + 1, sizeof *epair) ;
				memset(elist, 0, sizeof *elist) ;
				memset(epair, 0, sizeof *epair) ;
				for(i = 0; i <= V; i++){
					vlist[i] = new_vertex(i);
				}
			}else{
				/// in this line, can get edge's info(src and target)
				Vertex *v1, *v2 ;
				elist[line_n][0] = line_n ;
				elist[line_n][1] = 1 ;
				epair[line_n][0] = num1 ;
				epair[line_n][1] = num2 ;
				v1 = vlist[num1] ;
				v2 = vlist[num2] ;
				add_adjacency_vertex(v1, v2->label, 1) ;
				add_adjacency_vertex(v2, v1->label, 1) ;
				vlist[v1->label] = v1 ;
				vlist[v2->label] = v2 ;
			}
			line_n += 1 ;
		}


		G = new_graph(0, NULL);
		G->V = V ;
		G->E = E ;
		free_vertex(G->adj_list[0] );
		free(G->edge_list);
		free(G->adj_list);
		free(G->edge_pair) ;
		G->edge_list = elist ;
		G->edge_pair = epair ;
		G->adj_list = vlist ;
		
		a = (Vertex **)calloc(V / 2, sizeof(Vertex *));		
		for(i = 0; i < V / 2; i++){
			a[i] = G->adj_list[i + 1] ;
		}
		b = (Vertex **)calloc(V - V / 2, sizeof(Vertex *));		
		for(i = 0; i < V - V / 2; i++){
			b[i] = G->adj_list[V / 2 + i + 1] ;
		}
		int initialCutSize = cut(G, a, b, NULL);

		devide(G, a, b);

        int finalCutSize = cut(G, a, b, NULL);
		printf("%d %d\n",initialCutSize,finalCutSize) ;
		free(a) ; 
		free(b) ; 
		free_graph (G);
		free(line);
		fclose(input) ;

}

void input_gen(FILE *output, int D, int V){
	Graph *G = gen(D, V) ;
	if(output == NULL){
		return ;
	}
	edges(G, output) ;
	free_graph(G) ;
	return ;
}

int cut(Graph *G, Vertex *a[], Vertex *b[], FILE *output){ 
	int 
	(*cost)[G->V + 1] = (int (*)[G->V + 1])calloc(G->V + 1, sizeof *cost),//cost[G->V + 1][G->V + 1], 
	i, j, total_cost ;
	char *block = (char *)calloc(G->V + 1, sizeof *block) ; //block[G->V + 1] ;
	Vertex *v1, *v2 ;

	memset(cost, 0, (G->V + 1) * sizeof *cost) ;
	memset(block, 0, (G->V + 1) * sizeof *block) ;
	total_cost = 0 ;

	for(i = 0; i < G->V / 2; i++){
		block[ a[i]->label ] = 'a' ;
		block[ b[i]->label ] = 'b' ;
	}	
	block[ b[G->V - G->V / 2 - 1]->label ] = 'b' ;

	for(i = 0; i < G->V / 2; i++){
		v1 = a[i] ;
		v2 = b[i] ;
		for(j = 0; j < v1->degree; j++){
			cost[ v1->label ][ v1->list[j][0] ] = v1->list[j][1] ;
			if(block[ v1->label ] != block[ v1->list[j][0] ]){
				total_cost +=  v1->list[j][1] ;
				if(output != NULL){
					//fprintf(output, "%d\t--\t%d\n", v1->label, v1->list[j][0]);
				}
			}
		}
	}

	free(cost);
	free(block);
	return total_cost  ;
}


void devide(Graph *G, Vertex *a[], Vertex *b[]){
	int 
	INT_MIN = (int)(~0) << (sizeof(int) * 8 - 1), V = G->V ,
	INT_MAX = ~INT_MIN ;

	/*
	 * XXX 
	 * DONOT use stack. The size of these arrays can be very large
	 * and will cause stack overflow.
	 */
	int
	*label2index = calloc(V + 1, sizeof *label2index) ,
	*counting_a = (int *)calloc( MAX_DEGREE * 2 + 1, sizeof *counting_a ), 
	*counting_b = (int *)calloc( MAX_DEGREE * 2 + 1, sizeof *counting_b ), 
	*d = (int *)calloc(V + 1, sizeof *d ), //d[V + 1] 
	(*cost)[V + 1] = (int (*)[V + 1])calloc(V + 1, sizeof *cost),//cost[V + 1][V + 1], 
	*gsum = (int *)calloc(V / 2 + 1, sizeof *gsum), //gsum[V / 2 + 1], 
	(*ex)[2] = (int (*)[2])calloc(V / 2 + 1, sizeof *ex),//ex[V / 2 + 1][2], 
	*locked = (int *)calloc(V + 1, sizeof *locked), //locked[V + 1] ,
	i, j, k, maxk, maxgain, passes = 0, cumulative_gain = 0 ;
	char *block = (char *)calloc(V + 1, sizeof *block) ; //block[V + 1] ;
	Vertex *v1, *v2, 
	**sorted_a = (Vertex **)calloc( V / 2, sizeof *sorted_a),
	**sorted_b = (Vertex **)calloc(V - V / 2, sizeof *sorted_b);

	mainloop:
	/* Initialization */
	memset(counting_a, 0, (MAX_DEGREE * 2 + 1) * sizeof *counting_a);
	memset(counting_b, 0, (MAX_DEGREE * 2 + 1) * sizeof *counting_b);
	memset(d, 0, (V + 1) * sizeof *d)  ;
	memset(cost, 0, (V + 1) * sizeof *cost) ;
	memset(gsum , 0, (V / 2 + 1) * sizeof *gsum ) ;
	memset(ex, 0, (V / 2 + 1) * sizeof *ex) ;
	memset(block, 0, (V + 1) * sizeof *block) ;
	memset(locked, 0, (V + 1) * sizeof *locked ) ;

	gsum[0] = 0 ;
	maxk = 0 ;
	maxgain = INT_MIN ;

	for(i = 0; i < V / 2; i++){
		block[ a[i]->label ] = 'a' ;
		block[ b[i]->label ] = 'b' ;
	}	
	if( V - V / 2  != V / 2) {
		block[ b[V - V / 2 - 1]->label ] = 'b' ;
	}

	/* Compute the D value */
	int max_d_a = 0, min_d_a = MAX_DEGREE + MAX_DEGREE ;
	for (i = 0; i < V / 2; i++){
		v1 = a[i];
		for(j = 0; j < v1->degree; j++){
			if(block[ v1->list[j][0] ] == block[ v1->label ]){
				d[v1->label] -= v1->list[j][1] ;
			}else{
				d[v1->label] += v1->list[j][1] ;
			}
			cost[ v1->label ][ v1->list[j][0] ] = v1->list[j][1] ;
		}	
		counting_a[ MAX_DEGREE + d[v1->label] ] += 1;
		min_d_a = min_d_a > MAX_DEGREE + d[v1->label] ? MAX_DEGREE + d[v1->label] : min_d_a ;
		max_d_a = max_d_a < MAX_DEGREE + d[v1->label] ? MAX_DEGREE + d[v1->label] : max_d_a ;
	}	

	int max_d_b = 0, min_d_b = MAX_DEGREE + MAX_DEGREE ;
	for(i = 0; i < V - V / 2 ; i++){
		v2 = b[i] ;
		for(j = 0; j < v2->degree; j++){
			if(block[ v2->list[j][0] ] == block[v2->label ]){
				d[v2->label] -= v2->list[j][1] ;
			}else{
				d[v2->label] += v2->list[j][1] ;
			}
			cost[ v2->label ][ v2->list[j][0] ] = v2->list[j][1] ;
		}
		counting_b[ MAX_DEGREE + d[v2->label] ] += 1;
		min_d_b = min_d_b > MAX_DEGREE + d[v2->label] ? MAX_DEGREE + d[v2->label] : min_d_b ;
		max_d_b = max_d_b < MAX_DEGREE + d[v2->label] ? MAX_DEGREE + d[v2->label] : max_d_b ;
	}


	

	/* counting sort by D */
	int iter ;
	int sum_d_a[ max_d_a - min_d_a + 1]  ;
	memset(sum_d_a, 0,  (max_d_a - min_d_a + 1) * sizeof (int) );
	for(iter = min_d_a; iter <= max_d_a; iter ++){
		if( iter > min_d_a ){
			sum_d_a[ iter - min_d_a ] = sum_d_a[ iter - 1 - min_d_a ]  + counting_a[ iter - 1 ];
		}else{
			sum_d_a[ iter - min_d_a ] = 0;
		}
	}
	int sum_d_b[ max_d_b - min_d_b + 1] ;
	memset(sum_d_b, 0,  (max_d_b - min_d_b + 1) * sizeof (int) );
	for(iter = min_d_b; iter <= max_d_b; iter ++){
		if( iter > min_d_b ){
			sum_d_b[ iter - min_d_b ] = sum_d_b[ iter - 1 - min_d_b ]  + counting_b[ iter - 1 ];
		}else{
			sum_d_b[ iter - min_d_b ] = 0;
		}
	}
	for(iter = 0 ; iter < V / 2; iter ++){
		Vertex *v_a  =  a[ iter ] ;
		int d_MAX_DEGREE_centered = MAX_DEGREE + d[ v_a->label] ;
		counting_a[ d_MAX_DEGREE_centered  ] -= 1;
		int index = counting_a[ d_MAX_DEGREE_centered  ] ;
		sorted_a[ index + sum_d_a[ d_MAX_DEGREE_centered  - min_d_a ] ] = v_a ; 
	}
	
	for(iter = 0 ; iter <V - V / 2; iter ++){
		Vertex *v_b  =  b[ iter ] ;
		int d_MAX_DEGREE_centered = MAX_DEGREE + d[ v_b->label] ;
		counting_b[ d_MAX_DEGREE_centered  ] -= 1;
		int index = counting_b[ d_MAX_DEGREE_centered  ] ;
		sorted_b[ index + sum_d_b[ d_MAX_DEGREE_centered  - min_d_b ] ] = v_b ;
	}
	memcpy(a, sorted_a, (V / 2) * sizeof *sorted_a) ;
	memcpy(b, sorted_b, (V - V / 2) * sizeof *sorted_b) ;


	/* get the index of each label */
	for(i = 0; i < V; i++){
		if(i < V / 2){
			label2index[ a[i]->label ] = i ;
		}else{
			label2index[ b[i - V / 2]->label ] = i ;
		}
		
	}

	

	/* Inner loop, get the max-gain exchange pair  */
	for (k = 1; k <= V / 2; k++){
		int to_be_locked[2], to_be_exchanged[2] ;
		for(i =  V / 2 - 1; i >= 0 ; i--){
			int a_i_label;
			a_i_label = a[i]->label ;
			if(d[ a_i_label ] + d[ b[ V - V / 2 - 1 ]->label ] < maxgain ){
				break;
			}
			if (!locked[ a_i_label ]) {
				for(j = V - V / 2 - 1; j >= 0  ; j--){
					int b_j_label = b[j]->label ;
					if(d[ a_i_label ] + d[ b_j_label ] < maxgain ){
						break;
					}
					
					if(!locked[ b_j_label ]){
						int gain = d[ a_i_label ] + d[ b_j_label ] - 2 * cost[ a_i_label ][ b_j_label ] ;
						if( gain >= maxgain ){ 
							/* Either >= or > is OK. */
							maxgain = gain ;
							to_be_locked[0] = a_i_label ;
							to_be_exchanged[0] = i ;
							to_be_locked[1] = b_j_label ;
							to_be_exchanged[1] = j ;
						}
					}
				}
			}
		}
		
		locked[ to_be_locked[0] ] = 1 ;
		locked[ to_be_locked[1] ] = 1 ;
		ex[k][0] = a[to_be_exchanged[0] ]->label ;
		ex[k][1] = b[to_be_exchanged[1] ]->label ;		


		/* update D values */
		v1 = a[ to_be_exchanged[0] ] ;
		v2 = b[ to_be_exchanged[1] ] ;
		for(i = 0; i < v1->degree; i++){
			int label = v1->list[i][0] ;
			if(!locked[label]){
				if( block[ v1->label ] == block[ label ] ){
					d[ label ] += 2 * cost[ v1->label ][ label ] ;
					int j = label2index[ label ] ;
					/* bubble sort for the updated vertices */
					while(j + 1 < V / 2 && d[a[j]->label] > d[ a[j  + 1]->label ] ){
						Vertex *temp = a[j + 1] ;
						a[j + 1] = a[j] ;
						a[j] = temp ;
						label2index[ a[j]->label ] = j ;
						label2index[ a[j + 1]->label ] = j + 1;
						( j = j + 1 ) ;
					}
				}else{
					d[ label ] -= 2 * cost[ v1->label ][ label ] ;
					int j = label2index[ label ]  - V / 2;
					/* bubble sort for the updated vertices */
					while(j - 1 >= 0 && d[  d[ b[j]->label ] < b[j - 1]->label]  ){
						Vertex *temp = b[j - 1] ;
						b[j - 1] = b[j] ;
						b[j] = temp ;
						label2index[ b[j]->label ] = j + V / 2;
						label2index[ b[j - 1]->label ] = j - 1 + V / 2;
						( j = j - 1 ) ;
					}
				}
		
			}
		}
		for(i = 0; i < v2->degree; i++){
			int label = v2->list[i][0] ;
			if(!locked[label]){
				if(block[ v2->label ] == block[ label ]){
					d[ label ] += 2 * cost[ v2->label ][ label ] ;
					int j = label2index[ label ] - V / 2 ;
					/* bubble sort for the updated vertices */
					while(j + 1 < V - V / 2 && d[ b[j ]->label] < d[ b[j + 1]->label ] ){
						Vertex *temp = b[j + 1] ;
						b[j + 1] = b[j] ;
						b[j] = temp ;
						label2index[ b[j]->label ] = j + V / 2;
						label2index[ b[j + 1]->label ] = j + 1 + V / 2;
						( j = j + 1 ) ;
					}
				}else{
					d[ label ] -= 2 * cost[ v2->label ][ label ] ;
					int j = label2index[ label ] ;
					/* bubble sort for the updated vertices */
					while(j - 1 >= 0 && d[  d[ a[j]->label ] < a[j - 1]->label] ){
						Vertex *temp = a[j - 1] ;
						a[j - 1] = a[j] ;
						a[j] = temp ;
						label2index[ a[j]->label ] = j ;
						label2index[ a[j - 1]->label ] = j - 1;
						( j = j - 1 ) ;
					}
				}
			}
		}



		gsum[k] = gsum[k - 1] + maxgain;
		if(gsum[k] > gsum[ maxk ]){
			maxk = k ;
		}
		maxgain = INT_MIN ;
	}
	if(maxk == 0){
		free(d) ;
		free(cost);
		free(gsum);
		free(ex);
		free(block) ;
		free(locked);
		free(sorted_a); free( counting_a );
		free(sorted_b); free( counting_b );
		free(label2index);
		/*printf("\n#Total passes:\t%d\tCumulative gain:\t%d\n\
        --------------------------\n", passes, cumulative_gain);*/
		return ;
	}else{
		/*printf("Pass:\t%d\tGain:\t%d\n", passes, gsum[maxk ] ) ;*/
		passes += 1 ;
		cumulative_gain += gsum[maxk ] ;


		for(i = 1; i <= maxk; i++){
			int exi[2] = { label2index[ ex[i][0] ], label2index[ ex[i][1] ] - (V / 2) } ;
			Vertex *temp = a[ exi[0] ]  ;
			a[ exi[0] ] = b[ exi[1] ] ;
			b[ exi[1] ] = temp ;
			int tint = label2index[ ex[i][0] ] ;
			label2index[ ex[i][0] ] = label2index[ ex[i][1] ] ;
			label2index[ ex[i][1] ] = tint ;
			block[ ex[i][0] ] = 'b' ;
			block[ ex[i][1] ] = 'a' ;
		}
		goto mainloop ;
	}
}


