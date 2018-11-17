#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<math.h>
#include"mpi.h"

#define MAX_WORDS_IN_CORPUS 32
#define MAX_FILEPATH_LENGTH 16
#define MAX_WORD_LENGTH 16
#define MAX_DOCUMENT_NAME_LENGTH 8
#define MAX_STRING_LENGTH 64

typedef char word_document_str[MAX_STRING_LENGTH];

typedef struct o {
	char word[32];
	char document[8];
	int wordCount;
	int docSize;
	int numDocs;
	int numDocsWithWord;
	double TFIDF_value;
} obj;

typedef struct w {
	char word[32];
	int numDocsWithWord;
	int currDoc;
} u_w;

static int myCompare (const void * a, const void * b)
{
    return strcmp (a, b);
}

int main(int argc , char *argv[]){
	DIR* files;
	struct dirent* file;
	int i,j;
	int numDocs = 0, docSize, contains;
	//MPI
	int my_rank,number_of_nodes,tag=50;
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&number_of_nodes);

	char filename[MAX_FILEPATH_LENGTH], word[MAX_WORD_LENGTH], document[MAX_DOCUMENT_NAME_LENGTH];
	// Will hold all TFIDF objects for all documents
	obj TFIDF[MAX_WORDS_IN_CORPUS];
	obj ROOT_TFIDF[MAX_WORDS_IN_CORPUS*number_of_nodes];
	int TF_idx = 0;
	
	// Will hold all unique words in the corpus and the number of documents with that word
	u_w unique_words[MAX_WORDS_IN_CORPUS];

	u_w ROOT_unique_words[MAX_WORDS_IN_CORPUS*number_of_nodes];
	memset(unique_words,0,sizeof(unique_words));
	memset(ROOT_unique_words,0,sizeof(ROOT_unique_words));
	memset(TFIDF,0,sizeof(TFIDF));
	memset(ROOT_TFIDF,0,sizeof(ROOT_TFIDF));

	int uw_idx = 0;
	
	// Will hold the final strings that will be printed out
	word_document_str strings[MAX_WORDS_IN_CORPUS];
	
	if(my_rank==0){
		//Count numDocs
		if((files = opendir("input")) == NULL){
			printf("Directory failed to open\n");
			exit(1);
		}
		while((file = readdir(files))!= NULL){
			// On linux/Unix we don't want current and parent directories
			if(!strcmp(file->d_name, "."))	 continue;
			if(!strcmp(file->d_name, "..")) continue;
			numDocs++;
		}
	}
	//Send this data to all Worker Nodes
	MPI_Bcast(&numDocs,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if(my_rank!=0){
	// Loop through each document and gather TFIDF variables for each word
		for(i=my_rank; i<=numDocs; i +=(number_of_nodes-1)){
			sprintf(document, "doc%d", i);
			sprintf(filename,"input/%s",document);
			FILE* fp = fopen(filename, "r");
			if(fp == NULL){
				printf("Error Opening File: %s\n", filename);
				exit(0);
			}
		
			// Get the document size
			docSize = 0;
			while((fscanf(fp,"%s",word))!= EOF)
				docSize++;
		
			// For each word in the document
			fseek(fp, 0, SEEK_SET);
			while((fscanf(fp,"%s",word))!= EOF){
				contains = 0;
			
				// If TFIDF array already contains the word@document, just increment wordCount and break
				for(j=0; j<TF_idx; j++) {
					if(!strcmp(TFIDF[j].word, word) && !strcmp(TFIDF[j].document, document)){
						contains = 1;
						TFIDF[j].wordCount++;
						break;
					}
				}
			
				//If TFIDF array does not contain it, make a new one with wordCount=1
				if(!contains) {
					strcpy(TFIDF[TF_idx].word, word);
					strcpy(TFIDF[TF_idx].document, document);
					TFIDF[TF_idx].wordCount = 1;
					TFIDF[TF_idx].docSize = docSize;
					TFIDF[TF_idx].numDocs = numDocs;
					TF_idx++;
				}
			
				contains = 0;
				// If unique_words array already contains the word, just increment numDocsWithWord
				for(j=0; j<uw_idx; j++) {
					if(!strcmp(unique_words[j].word, word)){
						contains = 1;
						if(unique_words[j].currDoc != i) {
							unique_words[j].numDocsWithWord++;
							unique_words[j].currDoc = i;
						}
						break;
					}
				}
			
				// If unique_words array does not contain it, make a new one with numDocsWithWord=1 
				if(!contains) {
					strcpy(unique_words[uw_idx].word, word);
					unique_words[uw_idx].numDocsWithWord = 1;
					unique_words[uw_idx].currDoc = i;
					uw_idx++;
				}
			}
			fclose(fp);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	//Get all the unique words at root and merge their count and send back
	MPI_Gather(&(unique_words),sizeof(u_w)*MAX_WORDS_IN_CORPUS,MPI_BYTE,&(ROOT_unique_words),sizeof(u_w)*MAX_WORDS_IN_CORPUS,MPI_BYTE,0,MPI_COMM_WORLD);

	//Compine the result in unique_words
	int index=0;
	if(my_rank==0){
		for(int i=0;i<MAX_WORDS_IN_CORPUS*number_of_nodes;i++){
			int contain=0;
			//printf("Index= %d, word=%s,numDocsWithWord=%d,currentDoc=%d\n",i,ROOT_unique_words[i].word,
			//	ROOT_unique_words[i].numDocsWithWord,ROOT_unique_words[i].currDoc);
			if(ROOT_unique_words[i].numDocsWithWord>0){
				for(int j=0;j<MAX_WORDS_IN_CORPUS;j++){
					if(strcmp(unique_words[j].word,ROOT_unique_words[i].word)==0){
						contain=1;
						unique_words[j].numDocsWithWord +=ROOT_unique_words[i].numDocsWithWord;
						break;
					}
				}
				if(!contain){
					strcpy(unique_words[index].word,ROOT_unique_words[i].word);
					unique_words[index].numDocsWithWord = ROOT_unique_words[i].numDocsWithWord;
					unique_words[index].currDoc = ROOT_unique_words[i].currDoc;
					index++;
				}
			}
		}
		//print
		/*for(int i=0;i<MAX_WORDS_IN_CORPUS;i++){
			printf("Index= %d, word=%s,numDocsWithWord=%d,currentDoc=%d\n",i,unique_words[i].word,
				unique_words[i].numDocsWithWord,unique_words[i].currDoc);
		}*/
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&(unique_words),MAX_WORDS_IN_CORPUS*sizeof(u_w),MPI_BYTE,0,MPI_COMM_WORLD);

	// Print TF job similar to HW4/HW5 (For debugging purposes)
	//printf("-------------TF Job-------------\n");
	//for(j=0; j<TF_idx; j++)
	//	printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].wordCount, TFIDF[j].docSize);
		
	// Use unique_words array to populate TFIDF objects with: numDocsWithWord
	if(my_rank!=0){
		for(i=0; i<TF_idx; i++) {
			for(j=0; j<MAX_WORDS_IN_CORPUS; j++) {
				if(!strcmp(TFIDF[i].word, unique_words[j].word)) {
					TFIDF[i].numDocsWithWord = unique_words[j].numDocsWithWord;	
					break;
				}
			}
		}

	// Print IDF job similar to HW4/HW5 (For debugging purposes)
	//printf("------------IDF Job-------------\n");
	//for(j=0; j<TF_idx; j++)
	//	printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].numDocs, TFIDF[j].numDocsWithWord);
		
	// Calculates TFIDF value and puts: "document@word\tTFIDF" into strings array
		for(j=0; j<TF_idx; j++) {
			double TF = 1.0 * TFIDF[j].wordCount / TFIDF[j].docSize;
			double IDF = log(1.0 * TFIDF[j].numDocs / TFIDF[j].numDocsWithWord);
			TFIDF[j].TFIDF_value = TF*IDF;
		}
			//double TFIDF_value = TF * IDF;
			//sprintf(strings[j], "%s@%s\t%.16f", TFIDF[j].document, TFIDF[j].word, TFIDF_value);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Gather(&TFIDF,sizeof(obj)*MAX_WORDS_IN_CORPUS,MPI_BYTE,&ROOT_TFIDF,sizeof(obj)*MAX_WORDS_IN_CORPUS,MPI_BYTE,0,MPI_COMM_WORLD);

	if(my_rank==0){
		int scount =0;
		for(int i=0;i<MAX_WORDS_IN_CORPUS*number_of_nodes;i++){
			if(ROOT_TFIDF[i].numDocsWithWord>0){
				sprintf(strings[scount++], "%s@%s\t%.16f", ROOT_TFIDF[i].document, ROOT_TFIDF[i].word, ROOT_TFIDF[i].TFIDF_value);
			}
		}
		TF_idx = scount;
		// Sort strings and print to file
		qsort(strings, TF_idx, sizeof(char)*MAX_STRING_LENGTH, myCompare);
		FILE* fp = fopen("output.txt", "w");
		if(fp == NULL){
			printf("Error Opening File: output.txt\n");
			exit(0);
		}
		for(i=0; i<TF_idx; i++)
			fprintf(fp, "%s\n", strings[i]);
		fclose(fp);
	}
	MPI_Finalize();
	
	return 0;	
}
