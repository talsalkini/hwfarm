#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
//MPI & PThread
#include <mpi.h>
#include <pthread.h>

#include "../hwfarm/hwfarm.h" 

struct db_file_path{
	int file_id;
	char file_path[50];
	char file_output_path[50];
};

void printPath(struct db_file_path p){
	printf("[%d]. File info {%d}......\nPath: %s\nOutput: %s\n\n", rank, p.file_id, p.file_path, p.file_output_path);
}
 
void printAllPaths(struct db_file_path * ps, int n){
	int i = 0;
	for(;i<n;i++)
		printPath(ps[i]);
}



int readLocalFile(char *file_name){	
	FILE* f;
	if((f=fopen(file_name,"r")) == NULL){
		printf("Cannot open file.\n");
	}
	
	//search = (char*)malloc(sizeof(char)*n);
	char*tmp = (char*)malloc(1000);
	
	int i = 0, tmp_i = 0;
	int l = 0;
	l = fscanf(f, "%s\n", tmp);
	while(l != -1){		
		printf("+++++++++++++++++++++++++++++++++++++++++++++++++\n[%d]. l: %d , tmp_i: %d - %s\n+++++++++++++++++++++++++++++++++++++++++++++++++\n", rank, l, tmp_i, tmp);
		//		
		l = fscanf(f, "%s\n", tmp);		
	}	
	
	free (tmp);
	fclose(f);
	
	return i;
}

void printFile(char *search, int n){
	int i = 0;
	while(i < n){
		printf("i: %d - search: %c\n", i, search[i]);
		i++;
	}
}

char SMALL_A = 'a';
char SMALL_Z = 'z';
char CAP_A = 'A';
char CAP_Z = 'Z';
int TOTAL_ALPH = 26;

void setAlph(int * alph, int n, char c){

	if(c >= SMALL_A && c <= SMALL_Z){
		c = c - 32 - 65;
		alph[(int)c]++;
		return;
	}
	
	if(c >= CAP_A && c <= CAP_Z){
		c = c - 65;
		alph[(int)c]++;
		return;
	}
}

void printAlph(int * alph, int n){
	int i;
	for(i=0;i<n;i++)	
		printf("[%d]. %c %d\n", rank, (i+65), alph[i]);
}

void printAlphToFile(char * output_path, int * alph, int n){
	FILE* f_output;
	if((f_output=fopen(output_path,"w")) == NULL){
		printf("Cannot open file(%s).\n", output_path);
	}
	
	int i;
	for(i=0;i<n;i++)	
		fprintf(f_output, "%c %d\n", (i+65), alph[i]);
		
	fclose(f_output);
}

void printStatsToFile(char * output_path, char* caption, int value){
	FILE* f_output;
	if((f_output=fopen(output_path,"a")) == NULL){
		printf("Cannot open file(%s).\n", output_path);
	}
	
	fprintf(f_output, "\n%s: %d\n", caption, value);
		
	fclose(f_output);
}

void resetAlph(int * alph, int n){
	int i;
	for(i=0;i<n;i++)	
		alph[i] = 0;	
}

int isDot(char * w){
	int i=0;
	for(i=0;i<strlen(w);i++){
		if(w[i]=='.')
			return 0;
	}
	return 1;
}

void countWords(struct db_file_path file_p){
	FILE* f_input ;
	if((f_input=fopen(file_p.file_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_path);
	}
	
	//printf("countWords open file(%s).\n", file_p.file_path);
		
	int c = 0;
	
	char wordX[1024];
	
    while (fscanf(f_input, " %1023s", wordX) == 1) {
		if(isDot(wordX))
			c++;
    }
	fclose(f_input);	
	
	printStatsToFile(file_p.file_output_path, "Count Words", c);
	
}

struct file_word{
	char value[30];
	int occurance;
};

void resetAllWords(struct file_word * all_w, int n){
	int i;
	for(i=0;i<n;i++){
		strcpy(all_w[i].value, "");
		all_w[i].occurance = 0;
	}
}

void addWord(struct file_word * all_w, int n, char * word){
	int i;
	for(i=0;i<n;i++){	
		//printf("[%d]. (%d). %s:%d (%s, len: %d)\n", rank, i, all_w[i].value, all_w[i].occurance, word, strlen(word));	
		if(all_w[i].occurance == 0){
			strcpy(all_w[i].value, word);
			all_w[i].occurance = 1;
			return;
		}
		if(strcmp(word, all_w[i].value) == 0){
			all_w[i].occurance++;
			return;
		}
	}
}

void addWordWithOcc(struct file_word * all_w, int n, char * word, int occ){
	int i;
	for(i=0;i<n;i++){		
		if(all_w[i].occurance == 0){
			strcpy(all_w[i].value, word);
			all_w[i].occurance = occ;
			return;
		}
		if(strcmp(word, all_w[i].value) == 0){
			all_w[i].occurance = all_w[i].occurance + occ;
			return;
		}
	}
}

void printAllWords(struct file_word * all_w, int n){
	int i;
	for(i=0;i<n;i++){
		if(all_w[i].occurance == 0){
			if(i==0) printf("[%d]. No words...\n",rank);
			return;
		}
		printf("[%d]. Word(%d): %-20s with %d occurances..\n", rank, i, all_w[i].value,all_w[i].occurance);
	}
}

void printAllWordsToFile(char * output_path, struct file_word * all_w, int n){	
	FILE* f_output;
	if((f_output=fopen(output_path,"a")) == NULL){
		printf("Cannot open file(%s).\n", output_path);
	}
	
	fprintf(f_output, "\nList of Words:\n");
	
	int i;
	for(i=0;i<n;i++){
		if(all_w[i].occurance == 0)break;
		fprintf(f_output,"%s : %d\n", all_w[i].value,all_w[i].occurance);
	}
	
	fclose(f_output);
}

void printStage2WordsToFile(char * output_path, struct file_word * all_w, int n){	
	FILE* f_output;
	if((f_output=fopen(output_path,"w")) == NULL){
		printf("Cannot open file(%s).\n", output_path);
	}
	
	int i;
	for(i=0;i<n;i++){
		if(all_w[i].occurance == 0)break;
		fprintf(f_output,"%s : %d\n", all_w[i].value,all_w[i].occurance);
	}
	
	fclose(f_output);
}

int validStart(char * w){
	int c = w[0];
	if((c <= SMALL_Z && c >= SMALL_A) || (c <= CAP_Z && c >= CAP_A)){
		return 1;
	}
	return 0;
}

int validWord(char * w){
	int i = 0;
	int c;
	if(strlen(w)>25) return 0;
	for(i=0;i<strlen(w);i++){
		c = w[i];
		if(!((c <= SMALL_Z && c >= SMALL_A) 
			|| (c <= CAP_Z && c >= CAP_A)
			|| (c <= '9' && c >= '0'))){
			return 0;
		}
	}
	return 1;
}

void filterWord(char * w){
	int c = w[strlen(w)-1];
	if((c == ':')||(c == '!')||(c == ',')||(c == '.')||(c == '?')||(c == ')')||(c == '"')){
		w[strlen(w)-1] = '\0';
	}
}

void getWords(struct db_file_path file_p){
	FILE* f_input ;
	if((f_input=fopen(file_p.file_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_path);
	}
	
	//printf("getWords open file(%s).\n", file_p.file_path);
	
	char wordX[1024];
	
	struct file_word * all_words = (struct file_word *)malloc(sizeof(struct file_word)*1000);
	resetAllWords(all_words, 1000);
	
	while (fscanf(f_input, "%1023s", wordX) == 1) {		
		//printf("[%d]. wordX: %s\n", rank, wordX);
		if(isDot(wordX)){
			filterWord(wordX);
			//printf("[%d]. wordX: %s, start: %d, valid: %d\n", rank, wordX, validStart(wordX), validStart(wordX));
			if(validStart(wordX) && validWord(wordX))
				addWord(all_words, 1000, wordX);
		}
    }
    
    //printf("getWords close file(%s).\n", file_p.file_path);
	fclose(f_input);
		
	//printAllWords(all_words, 1000);
	printAllWordsToFile(file_p.file_output_path, all_words, 1000);
	
	free(all_words);	
}

void calcLongestWord(struct db_file_path file_p){
	FILE* f_input ;
	if((f_input=fopen(file_p.file_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_path);
	}
	
	//printf("calcLongestWord open file(%s).\n", file_p.file_path);
	
	char wordX[1024];
		
	int longest = 0;
	
    while (fscanf(f_input, "%1023s", wordX) == 1) {
		if(isDot(wordX)){
			if(longest < strlen(wordX)){
				longest = strlen(wordX);
			}
		}
    }
	fclose(f_input);	
	
	printStatsToFile(file_p.file_output_path, "Longest Words length", longest);
	
}

void calcAlph(struct db_file_path file_p){
	FILE* f_input ;
	if((f_input=fopen(file_p.file_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_path);
	}
	
	//printf("calcAlph open file(%s).\n", file_p.file_path);
	
	int * alph = (int*)malloc(sizeof(int)*TOTAL_ALPH);
	
	int c;
	
	resetAlph(alph, TOTAL_ALPH);
	//printAlph(alph, TOTAL_ALPH);
	
	while((c = fgetc(f_input)) != EOF) {
        /*if (c == 'b') {
            putchar(c);
        }
        printf("%c %d\n", c, c); 
        */
        setAlph(alph, TOTAL_ALPH, c);        
    }
	
	fclose(f_input);
	
	printAlphToFile(file_p.file_output_path, alph, TOTAL_ALPH);
	
	free(alph);
}

void countLetters(struct db_file_path file_p){
	FILE* f_input ;
	if((f_input=fopen(file_p.file_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_path);
	}	
	
	//printf("countLetters open file(%s).\n", file_p.file_path);

	int count = 0;
	
	int c;
	
	while((c = fgetc(f_input)) != EOF) {
        count++;      
    }
	
	fclose(f_input);
	
	printStatsToFile(file_p.file_output_path, "Count Letters", count);
}

void processFile(struct db_file_path file_p){
	calcAlph(file_p);
	
	countWords(file_p);
	
	countLetters(file_p);	
	
	calcLongestWord(file_p);
	
	getWords(file_p);
} 

void hwfarm_locality( hwfarm_task_data* t_data, chFM checkForMobility){
	
	int *i = t_data->counter;
	int *i_max = t_data->counter_max;
	
	printf("[%d]. *******-----------************------***************\n", rank);
	printf("[%d]. ***hwfarm_locality(%d files processed before %d(task: %d))***\n", rank, *i, *i_max, t_data->task_id);
	printf("[%d]. *******-----------************------***************\n", rank);
	
	struct db_file_path * db_paths = (struct db_file_path *)t_data->input_data;	
	
	double s_time = 0;	
	
	while(*i < *i_max){
		
		//printf("[%d]. File info: id: %d, path: %s\n", rank, db_paths[*i].file_id, db_paths[*i].file_path);
		//printPath(db_paths[*i]);
		s_time = MPI_Wtime();
		
		processFile(db_paths[*i]);
		
		s_time = MPI_Wtime() - s_time;
		
		//printf("[%d]. Time to process a file is : %f\n", rank, s_time);
		
		(*i)++;
		
		checkForMobility();
	}
	
	printf("[%d]. *******-----------************------***************\n", rank);
}

int doesFileExist(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

void readStage2Words(char * reduce_file, struct file_word * all_words, int n){
	if(doesFileExist(reduce_file)){
		FILE* f_reduce ;
		
		char * tmp_word = (char*)malloc(sizeof(char)*200);
		
		if((f_reduce=fopen(reduce_file,"r")) == NULL){
			printf("Cannot open file(%s).\n", reduce_file);
		}		
		
		int tmp_occ = 0;
		int l = 0;
		l = fscanf(f_reduce, "%s : %d\n", tmp_word, &tmp_occ);
		while(l != -1){		
			//printf("[%d]. tmp_word: %s\n", rank, tmp_word);
			addWordWithOcc(all_words, 1000, tmp_word, tmp_occ);
			//		
			l = fscanf(f_reduce, "%s : %d\n", tmp_word, &tmp_occ);
		}
	
		fclose(f_reduce);
		
		free(tmp_word);
	}
}

void processFileReduce(char * reduce_file_name, struct db_file_path file_p){
	
	//printf("[%d]. Processing file %s to the reduce file: %s\n", rank, file_p.file_output_path, reduce_file_name);
	
	char * tmp_word = (char*)malloc(sizeof(char)*200);
	
	struct file_word * all_words = (struct file_word *)malloc(sizeof(struct file_word)*10000);
	resetAllWords(all_words, 10000);
	
	readStage2Words(reduce_file_name, all_words, 10000);
	
	//printAllWords(all_words, 10000);
	
	FILE* f_stage1_output ;
	if((f_stage1_output=fopen(file_p.file_output_path,"r")) == NULL){
		printf("Cannot open file(%s).\n", file_p.file_output_path);
	}
	
	char * line = (char*)malloc(sizeof(char)*200);
	
	while ( fgets( line, 100, f_stage1_output))
	{ 
		//printf("[%d]. line: %s\n", rank, line);
		if(!strcmp(line, "List of Words:\n")) {
			int l = 0;
			int tmp_occ;
			l = fscanf(f_stage1_output, "%s : %d\n", tmp_word, &tmp_occ);
			while(l != -1){		
				//printf("[%d]. tmp_word: %s - %d\n", rank, tmp_word, tmp_occ);
				addWordWithOcc(all_words, 1000, tmp_word, tmp_occ);
				//		
				l = fscanf(f_stage1_output, "%s : %d\n", tmp_word, &tmp_occ);		
			}
		}
	}
	
	fclose(f_stage1_output);
	
	//printAllWords(all_words, 10000);
	
	printStage2WordsToFile(reduce_file_name, all_words, 10000);
	
	free(all_words);
	free(tmp_word);
	free(line);
} 

void hwfarm_reduce( hwfarm_task_data* t_data, chFM checkForMobility){

	int *i = t_data->counter;
	int *i_max = t_data->counter_max;
	
	printf("[%d]. *******-----------************------***************\n", rank);
	printf("[%d]. ***hwfarm_reduce(%d files processed before %d)***\n", rank, *i, *i_max);
	printf("[%d]. *******-----------************------***************\n", rank);

	int t_id = t_data->task_id;
	char * reduce_file_name = (char*)malloc(sizeof(char)*50);
	sprintf(reduce_file_name, "db/stage2_output/%d", t_id);
	
	printf("[%d]. Stage2 File name: %s\n", rank, reduce_file_name);
	
	struct db_file_path * db_paths = (struct db_file_path *)t_data->input_data;	
	
	double s_time = 0;
	
	while(*i < *i_max){
		
		//printf("[%d]. File info: id: %d, path: %s\n", rank, db_paths[*i].file_id, db_paths[*i].file_path);
		//printPath(db_paths[*i]);
		s_time = MPI_Wtime();
		
		processFileReduce(reduce_file_name, db_paths[*i]);
		
		s_time = MPI_Wtime() - s_time;
		
		//printf("[%d]. Time to process a file is : %f\n", rank, s_time);
		
		(*i)++;		
		
		checkForMobility();
	}
	
	printf("[%d]. *******-----------******hwfarm_reduce******------***************\n", rank);
}

void* initOutputData(void* output_data_in, int len){
	
	printf("[%d]. *******-----------************------*************** (%d)\n", rank, len);
	
	if(len == 0) return NULL;
	
	void* output_data = (void*)malloc(sizeof(void)*(len));
	
	return output_data;
}

struct db_file_path * assignFilePaths(struct db_file_path * ps, int n){
	char * orig_path = "db/stage1_input";
	char * orig_output_path = "db/stage1_output";
	
	char *ls_command = (char*)malloc(sizeof(char)*100);
	
	sprintf(ls_command, "ls %s -1", orig_path);
	
	FILE *fp1;
	fp1 = popen(ls_command, "r");  
	if(fp1 == 0)
		perror(ls_command);
		
	char *line = (char*)malloc(35 * sizeof(char));
	int i = 0;	
	while ( fgets( line, 35, fp1))
	{ 
		//printf("Line:L %s", line);
		ps[i].file_id = i;		
		sprintf(ps[i].file_path, "%s/%s", orig_path, line);
		ps[i].file_path[strlen(ps[i].file_path) - 1] ='\0';
		sprintf(ps[i].file_output_path, "%s/%s", orig_output_path, line);
		ps[i].file_output_path[strlen(ps[i].file_output_path) - 1] ='\0';
		
		if(++i >= n) break;
	}
	
	pclose(fp1); 
	free(line); 
	free(ls_command); 
	
	return ps;
}

int main(argc,argv)
int argc;
char **argv;
{ 
	initHWFarm(argc,argv);
	
	if(argc != 3){  
		printf("mpirun ... <binary file> <problem-size> <task-num>\n");
		exit(0);
	} 
	
	//int problem_size = atoi(argv[1]);
	//int tasks = atoi(argv[2]);	
	
	int problem_size = 50000;
	int tasks = 50;
	
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
			
	struct db_file_path * input_data = NULL;
	void * shared_data = NULL;
	int *output_data = NULL;

	int mobility = 0;
		
	//input
	int input_data_size = sizeof(struct db_file_path);
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
	
	printf("[%d]. +++problem_size: %d, chunk: %d, tasks: %d\n", rank, problem_size, chunk, tasks);
	
	if(rank == 0)
	{
		
		printf("MASTER START: %d\n", chunk);	
		
		input_data = (struct db_file_path *)malloc(sizeof(struct db_file_path)*problem_size);
		input_data = assignFilePaths(input_data, problem_size);
		
		/*
		input_data[0].file_id = 0;
		strcpy(input_data[0].file_path, "db/stage1_input/178345");
		strcpy(input_data[0].file_output_path, "db/stage1_output/178345");
		*/
		
		//printAllPaths(input_data, problem_size);
		
		//Output Data		
		output_data = initOutputData(output_data, output_data_len*output_data_size);		
		
		start_time = MPI_Wtime();
		printf("MASTER START: %.6f\n",MPI_Wtime());		
	} 
	

	hwfarm( hwfarm_locality, tasks,
		input_data, input_data_size, input_data_len, 
		shared_data, shared_data_size, shared_data_len, 
		output_data, output_data_size, output_data_len, 
		main_state, mobility);	
	
	main_state.counter = 0;
	main_state.max_counter = chunk;	
	
	hwfarm( hwfarm_reduce, tasks,
		input_data, input_data_size, input_data_len, 
		shared_data, shared_data_size, shared_data_len, 
		output_data, output_data_size, output_data_len, 
		main_state, mobility);	
	  
    finalizeHWFarm();
	
	return 1;   
}
