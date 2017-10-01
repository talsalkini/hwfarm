#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
//MPI & PThread
//#include <mpi.h>
//#include <pthread.h>

#include "../hwfarm/hwfarm.h" 

#define WORD_LEN 3

struct word{
	//3 for neclutides
	char value[WORD_LEN+1];
};

/* try to find the given pattern in the search string */
int bruteForce(char *search, char *pattern, int slen, int plen) {
	int i, j, k;
	//int found_i = -1;
	int score = 0;
	int * scores = (int*)malloc(sizeof(int)*(slen - plen + 1));
	for (i = 0; i <= slen - plen; i++) {		
		score = 0;
		for (j = 0, k = i; 
			(j < plen); j++, k++){
				//printf("--search: %c, pattern: %c--\n", search[k], pattern[j]);
				if(search[k] == pattern[j])
					score += 1;
				else
					score += -3;
			}
		scores[i] = score;
		if (j == plen)
			score = i;
	}
	
	int max_score = scores[0];
	int max_score_location = 0;
	
	for(i=0;i<slen - plen + 1;i++){
		if(max_score <= scores[i]){
			max_score = scores[i];
			max_score_location = i;
		}
	}
	
	//printf("MAXSCORE: %d at location %d\n", max_score, max_score_location);
	
	free (scores);
	//return found_i;
	return max_score;
}

int readGenes(char *search, int n){	
	FILE* f;
	if((f=fopen("db/chr.fsa","r")) == NULL){
		printf("Cannot open file.\n");
	}
	
	//search = (char*)malloc(sizeof(char)*n);
	char*tmp = (char*)malloc(100);
	
	int i = 0, tmp_i = 0;
	int l = 0;
	l = fscanf(f, "%s\n", tmp);
	while(l != -1){		
		//printf("l: %d - %s\n", l, tmp);
		while(tmp[tmp_i] != '\0' && tmp[tmp_i] != '\n'){
			//printf("tmp_i: %d - %c, i: %d\n", tmp_i, tmp[tmp_i], i);
			search[i] = tmp[tmp_i];
			tmp_i++; i++;
			if(i >= n) break;
		}
		if(i >= n) break;
		tmp_i = 0;
		l = fscanf(f, "%s\n", tmp);		 
	}	
	
	free (tmp);
	fclose(f);
	
	return i;
}

void printGenes(char *search, int n){
	int i = 0;
	while(i < n){
		printf("i: %d - search: %c\n", i, search[i]);
		i++;
	}
}

void printWords(struct word * words, int len){
	int i=0;
	for(i=0;i<len;i++){
		printf("word %d: %s\n", i,words[i].value);
	}
}

struct word * getSubs(char *pattern, int plen){	
	int words_len = plen - WORD_LEN + 1;
	struct word * words = (struct word *)malloc(sizeof(struct word)*(words_len));
	int i = 0;
	for(i =0;i<words_len;i++){		
		words[i].value[0] = pattern[i];
		words[i].value[1] = pattern[i+1]; 
		words[i].value[2] = pattern[i+2];
		words[i].value[3] = '\0';
	}
	
	return words;
}

void hwfarm_blast( hwfarm_task_data* t_data, chFM checkForMobility){
	int *i = t_data->counter;
	int *i_max = t_data->counter_max;	
	int *output_p = (int *)t_data->output_data;	
	struct word * input_p = t_data->input_data; 
	char * shared_p = (char *)t_data->shared_data;
	int input_len = t_data->input_len;	
	
	printf("[%d]. *******-----------************------***************\n", rank);
	printf("[%d]. task_id: %d\n", rank, t_data->task_id);
	printf("[%d]. input_len: %d\n", rank, input_len);
	//printf("[%d]. input_data: %10s\n", rank, (char*)t_data->input_data);
	printf("[%d]. counter: %d\n", rank, *(t_data->counter));
	printf("[%d]. counter_max: %d\n", rank, *(t_data->counter_max));
	printf("[%d]. shared_len: %d\n", rank, t_data->shared_len);
	printf("[%d]. shared_data: %s\n", rank, (char*)t_data->shared_data);
	printf("[%d]. *******-----------************------***************\n", rank);
	
	//int ii=0;
	//for(ii=0;ii<input_len;ii++)
	//	printf("[%d]. input_data[%d]=%c\n", rank, ii, input_p[ii]);
		
	//sleep(2);
	
	struct word * words = input_p;
	
	//int ii=0;
	//for(ii=0;ii<input_len;ii++)
		//printf("[%d]. words: %s\n", rank, words[ii].value);
		
	//getSubs(t_data->shared_data, t_data->shared_len);
	
	while(*i < *i_max){
		printf("[%d]. words: %s\n", rank, words[*i].value);
		int r = bruteForce(shared_p, words[*i].value, input_len, WORD_LEN);
		
		//printf("[%d]. Word : %s is at location %d\n", rank, words[*i].value, r);
		
		output_p[*i] = r;
		
		(*i)++;
		
		checkForMobility();
	}
	
	
	printf("[%d]. *******-----------************------***************\n", rank);
}

void* initOutputData(void* output_data_in, int len){
	
	printf("[%d]. *******-----------************------*************** (%d)\n", rank, len);
	
	void* output_data = (void*)malloc(sizeof(void)*(len));
	
	return output_data;
}

int main(argc,argv)
int argc;
char **argv;
{ 
	initHWFarm(argc,argv);
	
	if(argc != 3){  
		printf("mpirun ... <binary file> <dna-size> <task-num>\n");
		exit(0);
	} 
	
	int problem_size = atoi(argv[1]);
	int tasks = atoi(argv[2]);	
	
	//chunk: number of particles for one task
	
	if(tasks == 0){  
		printf("Illegal task value...\n");
		exit(0);
	} 
	if(tasks > problem_size){  
		printf("Illegal task value...\n");
		exit(0);
	} 
	if(problem_size % tasks != 0){  
		printf("Illegal task value...\n");
		exit(0);
	} 
	int chunk = problem_size / tasks;
	
	char * shared_data = NULL;
	int *output_data = NULL;

	int mobility = 1;
		
	//input
	void * input_data = NULL;
	int input_data_size = sizeof(struct word);
	int input_data_len = chunk;	
		
	//shared
	int shared_data_size = sizeof(char);
	int shared_data_len = 0;
	
	//output
	int output_data_size = sizeof(int);
	int output_data_len = 0;
	//int output_data_total = problem_size;
	
	//int start_particle = (problem_size / tasks) + ;

	hwfarm_state main_state;
	main_state.counter = 0;
	main_state.max_counter = chunk;
	main_state.state_data = NULL;
	main_state.state_len = 0;	
	
	printf("[%d]. problem_size: %d, chunk: %d\n", rank, problem_size, chunk);	
	
	//MPI_Barrier(MPI_COMM_WORLD);
	
	if(rank == 0)
	{
		printf("MASTER START...\n");		
	
		
		
		//int ii=0;
		//for(ii=0;ii<problem_size;ii++)
		//	printf("[%d]. input_data[%d]=%c\n", rank, ii, ((char*)input_data)[ii]);
		
		//prepare data to be sent to the workers			
		
		//Shared data		
		//char * pattern = "CTGGCCATTACTAGAAGAAGAGAAACAATTAGTGTATTGGATTCGACAAGAGGCAAGCAAGGGAGCCAAGTTTTCCGCATGTCTGGAAGGCAGATCAAAGAGTTGTATTATAAAGTATGGAGCAACTTGCGTGAATCGAAGACAGAGGTGCTGCAGTACTTTTTGAACTGGGACGAGAAAAAGTGCCGGGAAGAATGGGAGGCAAAAGACGATACGGTCTTTGTGTGGAAGCGCTCGAGAAAGTTGGAGTTTTTCAGCGTTTGCGTTCCATGACGAGCGCTGGACTGCAGGGTCCGCAGTACGTCAAGCTGCAGTTTAGCAGGCATCATCGACAGTTGAGGAGCAGATATGAATTAAGTCTAGGAATGCACTTGCGAGATCAGCTTGCGCTGGGAGTTACCCCATCTAAAGTGCCGCATTGGACGGCATTCCTGTCGATGCTGATAGGGCTGTTCTACAATAAAACATTTCGGCAGAAACTGGAATATCTTTTGGAGCAG";
		char * pattern = "CTGGCCATTACTAGAAGAAGAA";
		//char * pattern = "CTGGCC";
		int num_of_sub = strlen(pattern) - WORD_LEN + 1;
		printf("[%d]. len of pattern: %d\n", rank, (int)strlen(pattern));		
		
		printf("[%d]. Total number of patterns: %d\n", rank, num_of_sub);
		struct word * words = getSubs(pattern, strlen(pattern));
		int ii=0;
		for(ii=0;ii<num_of_sub;ii++)
			printf("[%d]. words: %s\n", rank, words[ii].value);
			
		input_data = words;
		input_data_len = (num_of_sub)/tasks;
			
		char * blast_input_data = (char*)malloc(sizeof(char)*problem_size);		
		int numOfNucleotides = readGenes(blast_input_data, problem_size);
		
		printf("[%d]. numOfNucleotides: %d\n", rank, numOfNucleotides);
		
		shared_data = blast_input_data;
		shared_data_len = problem_size;	
		
		//Output Data		
		output_data_len = (num_of_sub)/tasks;
		output_data = (int*)initOutputData(output_data, output_data_len);	
		
		main_state.max_counter = (num_of_sub)/tasks;	
		
		start_time = MPI_Wtime();
		printf("MASTER START: %.6f\n",MPI_Wtime());	
	} 
	
	//MPI_Barrier(MPI_COMM_WORLD);
	
	//return 0;

	hwfarm( hwfarm_blast, tasks,
		input_data, input_data_size, input_data_len, 
		shared_data, shared_data_size, shared_data_len, 
		output_data, output_data_size, output_data_len, 
		main_state, mobility);		 
		  
    finalizeHWFarm();
	
	return 1;   
}
