#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX 1000


/* try to find the given pattern in the search string */
int bruteForce(char *search, char *pattern, int slen, int plen) {
	int i, j, k;
	int found_i = -1;
	int score = 0;
	int * scores = (int*)malloc(sizeof(int)*(slen - plen + 1));
	for (i = 0; i <= slen - plen; i++) {
		//printf("--%c--\n", search[i]);
		score = 0;
		for (j = 0, k = i; 
			(j < plen); j++, k++){
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
	if((f=fopen("db/tmp.fsa","r")) == NULL){
		printf("Cannot open file.\n");
	}
	
	//search = (char*)malloc(sizeof(char)*n);
	char*tmp = (char*)malloc(1000);
	
	int i = 0, tmp_i = 0;
	int l = 0;
	l = fscanf(f, "%s\0", tmp);
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
		l = fscanf(f, "%s\0", tmp);		
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

#define WORD_LEN 3

struct word{
	//3 for neclutides
	char value[WORD_LEN+1];
};

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

int main() {
	int n = 200000000;
	
	char * gene_seq = (char*)malloc(sizeof(char)*n);
	
	
	int numOfNucleotides = readGenes(gene_seq, n);
	
	//printGenes(gene_seq, numOfNucleotides);
	
	
	printf("Number of Nucleotides: %d\n", numOfNucleotides);
	
	//char * pattern = "TATCAGGGTGGGGACCCTGTAGGATATTTGGTACCTGGCCATTACTAGAAGAAGAGAAACAATTAGTGTATTGGATTCGACAAGAGGCAAGCAAGGGAGCCAAGTTTTCCGCATGTCTGGAAGGCAGATCAAAGAGTTGTATTATAAAGTATGGAGCAACTTGCGTGAATCGAAGACAGAGGTGCTGCAGTACTTTTTGAACTGGGACGAGAAAAAGTGCCGGGAAGAATGGGAGGCAAAAGACGATACGGTCTTTGTGGAAGCGCTCGAGAAAGTTGGAGTTTTTCAGCGTTTGCGTTCCATGACGAGCGCTGGACTGCAGGGTCCGCAGTACGTCAAGCTGCAGTTTAGCAGGCATCATCGACAGTTGAGGAGCAGATATGAATTAAGTCTAGGAATGCACTTGCGAGATCAGCTTGCGCTGGGAGTTACCCCATCTAAAGTGCCGCATTGGACGGCATTCCTGTCGATGCTGATAGGGCTGTTCTACAATAAAACATTTCGGCAGAAACTGGAATATCTTTTGGAGCAGATTTCGGAGGTGTGGTTGTTACCACATTGGCTTGATTTGGCAAACGTTGAAGTTCTCGCTGCAGATAACACGAGGGTACCGCTGTACATGCTGATGGTAGCGGTTCACAAAGAGCTGGATAGCGATGATGTTCCAGACGGTAGATTTGATATAATATTACTATGTAGAGATTCGAGCAGAGAAGTTGGAGAGTGAAGGAAATTGTTGTTACGAAAGTCAGTGATTATGTATTGTGTAGTATAGTATATTGTAAGAAATTTTTTTTTCTAGGGAATATGCGTTTTGATGTAGTAGTATTTCACTGTTTTGATTTAGTGTTTGTTGCACGGCAGTAGCGAGAGACAAGTGGGAAAGAGTAGGATAAAAAGACAATCTATAAAAAGTAAACATAAAATAAAGGTAGTAAGTAGCTTTTGGTTGAACATCCGGGTAAGAGACAACAGGGCTTGGAGGAGACGTACATGAGGGTATCAGGGTGGGGACCCTGTAGGATATTTGGTACCTGGCCATTACTAGAAGAAGAGAAACAATTAGTGTATTGGATTCGACAAGAGGCAAGCAAGGGAGCCAAGTTTTCCGCATGTCTGGAAGGCAGATCAAAGAGTTGTATTATAAAGTATGGAGCAACTTGCGTGAATCGAAGACAGAGGTGCTGCAGTACTTTTTGAACTGGGACGAGAAAAAGTGCCGGGAAGAATGGGAGGCAAAAGACGATACGGTCTTTGTGGAAGCGCTCGAGAAAGTTGGAGTTTTTCAGCGTTTGCGTTCCATGACGAGCGCTGGACTGCAGGGTCCGCAGTACGTCAAGCTGCAGTTTAGCAGGCATCATCGACAGTTGAGGAGCAGATATGAATTAAGTCTAGGAATGCACTTGCGAGATCAGCTTGCGCTGGGAGTTACCCCATCTAAAGTGCCGCATTGGACGGCATTCCTGTCGATGCTGATAGGGCTGTTCTACAATAAAACATTTCGGCAGAAACTGGAATATCTTTTGGAGCAGATTTCGGAGGTGTGGTTGTTACCACATTGGCTTGATTTGGCAAACGTTGAAGTTCTCGCTGCAGATAACACGAGGGTACCGCTGTACATGCTGATGGTAGCGGTTCACAAAGAGCTGGATAGCGATGATGTTCCAGACGGTAGATTTGATATAATATTACTATGTAGAGATTCGAGCAGAGAAGTTGGAGAGTGAAGGAAATTGTTGTTACGAAAGTCAGTGATTATGTATTGTGTAGTATAGTATATTGTAAGAAATTTTTTTTTCTAGGGAATATGCGTTTTGATGTAGTAGTATTTCACTGTTTTGATTTAGTGTTTGTTGCACGGCAGTAGCGAGAGACAAGTGGGAAAGAGTAGGATAAAAAGACAATCTATAAAAAGTAAACATAAAATAAAGGTAGTAAGTAGCTTTTGGTTGAACATCCGGGTAAGAGACAACAGGGCTTGGAGGAGACGTACATGAGGG";
	//char * pattern = "CTGGCCATTACTAGAAGAAGAGAAACAATTAGTGTATTGGATTCGACAAGAGGCAAGCAAGGGAGCCAAGTTTTCCGCATGTCTGGAAGGCAGATCAAAGAGTTGTATTATAAAGTATGGAGCAACTTGCGTGAATCGAAGACAGAGGTGCTGCAGTACTTTTTGAACTGGGACGAGAAAAAGTGCCGGGAAGAATGGGAGGCAAAAGACGATACGGTCTTTGTGTGGAAGCGCTCGAGAAAGTTGGAGTTTTTCAGCGTTTGCGTTCCATGACGAGCGCTGGACTGCAGGGTCCGCAGTACGTCAAGCTGCAGTTTAGCAGGCATCATCGACAGTTGAGGAGCAGATATGAATTAAGTCTAGGAATGCACTTGCGAGATCAGCTTGCGCTGGGAGTTACCCCATCTAAAGTGCCGCATTGGACGGCATTCCTGTCGATGCTGATAGGGCTGTTCTACAATAAAACATTTCGGCAGAAACTGGAATATCTTTTGGAGCAG";
	char * pattern = "CTGGCC";
	
	printf("Len of Pattern: %d\n", strlen(pattern));
	
	
	struct word * words = getSubs(pattern, strlen(pattern));
	int words_len = strlen(pattern) - WORD_LEN + 1;
	
	//printWords(words, words_len);
	
	int j =0;
	for(j=0;j<words_len;j++){
		int r = bruteForce(gene_seq, words[j].value, numOfNucleotides, WORD_LEN);
		//printf("Word : %s is at location %d\n", words[j].value, r);
	}
	
	/*
	int r = bruteForce(gene_seq, pattern, numOfNucleotides, strlen(pattern));	
	
	if (r == -1) {
			printf("Search pattern is not available\n");
	} else {
			printf("Search pattern available at the location %d\n", r);
	}
	*/
	
	return 0;
}
