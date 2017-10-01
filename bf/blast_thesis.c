#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../hwfarm/hwfarm.h" 

#define WORD_LEN 3

struct word{
   //3 for neclutides
   char value[WORD_LEN];
};

//find the given pattern in the search string 
int find(char *search, char *pattern, int slen, int plen) {
   int i, j, k;
   int score = 0;
   int * scores = (int*)malloc(sizeof(int)*(slen - plen + 1));
   for (i = 0; i <= slen - plen; i++) {		
      score = 0;
      for (j = 0, k = i; (j < plen); j++, k++){
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
	
   free (scores);
   return max_score;
}

int readGenes(char *search, int n){	
   FILE* f;
   if((f=fopen("db/chr.fsa","r")) == NULL){
      printf("Cannot open file.\n");
      exit(0);
   }

   char*tmp = (char*)malloc(100);
	
   int i = 0, tmp_i = 0, l = 0;
   l = fscanf(f, "%s\n", tmp);
   while(l != -1){		
      while(tmp[tmp_i] != '\0' && tmp[tmp_i] != '\n'){
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
   }	
   return words;
}

void hwfarm_blast( hwfarm_task_data* t_data, chFM checkForMobility){
   int *i = t_data->counter;
   int *i_max = t_data->counter_max;	
   int *output_p = (int *)t_data->output_data;	
   struct word * words = t_data->input_data; 
   char * shared_p = (char *)t_data->shared_data;
   int input_len = t_data->input_len;	

   while(*i < *i_max){
      output_p[*i] = find(shared_p, words[*i].value, input_len, WORD_LEN);
      (*i)++;		
      checkForMobility();
   }	
}

int main(int argc, char** argv){ 	
   initHWFarm(argc,argv); 
	
   int problem_size = atoi(argv[1]);   
   int tasks = atoi(argv[2]);	   
   int mobility = atoi(argv[3]);
   //input data
   void * input_data = NULL;
   int input_data_size = sizeof(struct word);
   int input_data_len = 0;	
   //shared data
   char * shared_data = NULL;
   int shared_data_size = sizeof(char);
   int shared_data_len = problem_size;
   //output data
   int *output_data = NULL;
   int output_data_size = sizeof(int);
   int output_data_len = 0;
	
   hwfarm_state main_state;
   main_state.counter = 0;
   main_state.max_counter = 0;
   main_state.state_data = NULL;
   main_state.state_len = 0;	
	
   if(rank == 0){   
      //input ( the words that would be distrbuted into tasks ) 
      char * pattern = "CTGGCCATTACTAGAAGAAGAA";      
      int num_of_sub = strlen(pattern) - WORD_LEN + 1;
      input_data = getSubs(pattern, strlen(pattern));      
      int chunk = num_of_sub / tasks;
      input_data_len = chunk;
      
      //shared data is the sequences of genes
      char * blast_input_data = (char*)malloc(sizeof(char)*problem_size);		
      readGenes(blast_input_data, problem_size);
      shared_data = blast_input_data;
      shared_data_len = problem_size;
      
      //Output Data(in this example the output is based on the search pattern)      
      output_data = malloc(output_data_size*num_of_sub);
      output_data_len = chunk;
		
	  //modify state data based on the search pattern
      main_state.max_counter = chunk;	
   } 
	
   hwfarm( hwfarm_blast, tasks,
        input_data, input_data_size, input_data_len, 
        shared_data, shared_data_size, shared_data_len, 
        output_data, output_data_size, output_data_len, 
        main_state, mobility);
        
   if(rank == 0){
      //print the output to a file (output_data, num_of_sub)
   }		 
		  
    finalizeHWFarm();
	
	return 1;   
}
