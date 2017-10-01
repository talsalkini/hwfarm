#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//MPI & PThread
#include <mpi.h>
#include <pthread.h>
 
#include "../hwfarm/hwfarm.h" 

 
void delay(int i){
	//int MAX = 130;
	int MAX = 20;
	while(i > 0){
		int ii=0;	
		for(ii=0;ii<MAX;ii++){
			int kk = MAX - ii;
			kk = floor(pow(kk,ii));
			kk = ceil(sqrt(kk*1.0));
		}
		i--;
	}
}
///This function is written by the user countains :
///inputData : the data should be processed by each worker(one task)
///inputLen : the length of data in one task
///result : the data resulting from processing the input data(result of one task)
///outputLen : the length of the result
///process the data(task)
void hwfarm_pow( hwfarm_task_data* t_data, chFM checkForMobility){
	
	printf("[%d]. *******-----------************------***************\n", rank);
	printf("[%d]. task_id: %d\n", rank, t_data->task_id);	
	//printf("[%d]. input_data: %10s\n", rank, (char*)t_data->input_data);
	printf("[%d]. counter: %d\n", rank, *(t_data->counter));
	printf("[%d]. counter_max: %d\n", rank, *(t_data->counter_max));
	printf("[%d]. shared_len: %d\n", rank, t_data->shared_len);
	printf("[%d]. shared_data: %s\n", rank, (char*)t_data->shared_data);
	printf("[%d]. *******-----------************------***************\n", rank);
	
	int *i = t_data->counter;
	int *i_max = t_data->counter_max;
	int *input_p = (int*)t_data->input_data;
	int *output_p = (int*)t_data->output_data;
	while(*i < *i_max){
		
		delay(1);
		
		*(output_p + (*i)) = (int)*(input_p + (*i)) * *(input_p + (*i));
		
		//delay(1);
		
		
		(*i)++;
		
		checkForMobility();
	}
}

void* initSharedData(void* shared_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	
	double* shared_data = (double*)malloc(sizeof(double)*(mat_size*mat_size));

	int k = 0, i=0, j=0;
	
	//printf("mat_size: %d\n", mat_size);
	
	for (i = 0; i < mat_size; i++) {
		for (j = 0; j < mat_size; j++) {
			shared_data[k++] = i + j + 2;
		}
	}
	
	return shared_data;
} 
 
void* initInputData(void* input_data_in, int len){
	
	int* input_data = (int*)malloc(sizeof(int)*(len));

	int j=0,k=0;
	for (j = 0; j < len; j++) {
		input_data[k++] = j+1;
	}
	
	return input_data;
}

void* initStateData(void* par_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	int chunk = *((int*)args[1]);
	
	int* state_data = (int*)malloc(sizeof(double)*(7));
	
	state_data[0] = 0;
	state_data[1] = 0;
	state_data[2] = 0;
	state_data[3] = 0;	
	state_data[4] = chunk;
	state_data[5] = mat_size;
	state_data[6] = mat_size;

	return state_data;
}



void* initOutputData(void* output_data_in, int len){
	
	int* output_data = (int*)malloc(sizeof(int)*(len));
	
	return output_data;
}

void printResultToFile(int * output_data, int problem_size){  
	FILE *fres = fopen("finalResult.txt","w");
	int i=0;
	for(i=0;i<problem_size;i++){
		fprintf(fres, "%d\n", *(output_data + (i)));
	}
	fclose(fres);
}



//*********************************************************
//---------------------------------------------------------
//ex: mpirun -n 2 -host bwlf30,bwlf31,bwlf32,bwlf29 ./pow 10 10
//---------------------------------------------------------
//*********************************************************
int main(argc,argv)
int argc;
char **argv;
{ 	
	
	if(argc!=3){  
		printf("mpirun ... <binary file> <array-size> <chunck-size>\n");
		exit(0);
	} 

	initHWFarm(argc,argv);
	
	printf("Worker/Master [%d] In Machine [%s]\n",rank,processor_name);	
	
	int problem_size = atoi(argv[1]);
	//chunk: number of rows in one task
	int chunk = atoi(argv[2]);		
	int tasks = problem_size / chunk;	

	int * input_data = NULL;	
	int * output_data = NULL;

	int mobility = 1;
		
	//input
	int input_data_size = sizeof(int);
	int input_data_len = chunk;	
		
	//output
	int output_data_size = sizeof(int);
	int output_data_len = chunk;
		
	hwfarm_state main_state;
	main_state.counter = 0;
	main_state.max_counter = chunk;
	main_state.state_data = NULL;
	main_state.state_len = 0;

	if(rank == 0)
	{
		
		printf("MASTER START: %.6f\n",MPI_Wtime());		
				
		//prepare data to be sent to the workers			
		
		//Input data		
		input_data = (int*)initInputData(input_data, problem_size);	
		
		//int i=0;
		//for(i=0;i<problem_size;i++){
		//	printf("[Master]-input: %d\n", *((int*)input_data + (i)));
		//}

		//Output Data		
		output_data = (int*)initOutputData(output_data, problem_size);		
		
		start_time = MPI_Wtime();

	} 
	/*
	if(rank == 0){
		int i=0;
		for(i=0;i<problem_size;i++){
			printf("[Master]-input: %d\n", *((int*)input_data + (i)));
		}
	}
	*/
	hwfarm( hwfarm_pow, tasks,
		    input_data, input_data_size, input_data_len, 
		    NULL, 0, 0, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
 /*
	if(rank == 0){
		int i=0;
		for(i=0;i<problem_size;i++){
			printf("[Master]-mid-input: %d\n", *((int*)output_data + (i)));
		}
	}*/
	/*	    
	main_state.counter = 0;
	main_state.max_counter = chunk;
	main_state.state_data = NULL;
	main_state.state_len = 0;
		    
	hwfarm( hwfarm_pow, tasks,
		    output_data, input_data_size, input_data_len, 
		    NULL, 0, 0, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
		    
	if(rank == 0){
		int i=0;
		for(i=0;i<problem_size;i++){
			printf("[Master]-output: %d\n", *((int*)output_data + (i)));
		}
	}
	    
	
	if(rank == 0){	
		printResultToFile(output_data, problem_size);
	}
	*/
	finalizeHWFarm();
	
	return 1;  
}
