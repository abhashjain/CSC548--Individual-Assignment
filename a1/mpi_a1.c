#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include"mpi.h"

//function to sum the array
long sum(long *time,int count){
	long result =0;
	for(int i=0;i<count;i++){
		result+=time[i];
	}
	return result;
}

int main(int argc,char **argv){
	int my_rank;
    int p;
    int source;
    int dest;
    int tag=50;
	struct timeval start,end,final;
	MPI_Status status;
    MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&p);
	if(p<8){
		printf("Error: Less number of process!\n");
		return 0;
	}
    int msg_size[] = {32,64,128,256,512,1024,1024*2,1024*4,1024*8,1024*16,1024*32,1024*64,1024*128,1024*256,1024*512,1024*1024,1024*1024*2};
	for(int k=0;k<17;k++){
	int msg_sz = msg_size[k];
	char *message = (char *) calloc(msg_sz,sizeof(char)) ;
	memset(message,'a',sizeof(char)*msg_sz);
	if(my_rank<4){
		MPI_Send(message,msg_sz+1,MPI_CHAR,my_rank+4,tag,MPI_COMM_WORLD);
		MPI_Recv(message,msg_sz+1,MPI_CHAR,my_rank+4,tag,MPI_COMM_WORLD,&status);
	} else {
		MPI_Recv(message,msg_sz+1,MPI_CHAR,my_rank-4,tag,MPI_COMM_WORLD,&status);
		MPI_Send(message,msg_sz+1,MPI_CHAR,my_rank-4,tag,MPI_COMM_WORLD);
	}
	if(my_rank==0){
		long time[10]={0};
		long time_p[4]={0};
		
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD);
            	MPI_Recv(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD);
			MPI_Recv(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD,&status);
			//printf("message received %s and size is %d\n",message,msg_sz);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=(final.tv_sec*1000000+final.tv_usec);
		}
			long t = sum(time,10);
			t/=10;
			time_p[0] = t;
			//printf("Abhash: value at p0 is %ld\n",t);
			for(int i=1;i<4;i++){
				long temp=0;
				MPI_Recv(&time_p[i],1,MPI_LONG,i,tag,MPI_COMM_WORLD,&status);
				//printf("Received value from i %d is %ld\n",i,time_p[i]);
				//time_p[i] =temp; 
			}
			printf("%d: %ld %ld %ld %ld\n",msg_sz,time_p[0],time_p[1],time_p[2],time_p[3]);
	} else if(my_rank==1){
		long time[10]={0};
		//char buf[50];
		for(int i=0;i<=10;i++) {
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD);
	            MPI_Recv(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD);
        	MPI_Recv(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=(final.tv_sec*1000000+final.tv_usec);
		}
		long t = sum(time,10);
		t/=10;
		//send mean and std dev to p0
		MPI_Send(&t,1,MPI_LONG,0,tag,MPI_COMM_WORLD);
		//printf("%d: Time taken for %d is %ld usec\n",msg_sz,my_rank,t);
	} else if(my_rank==2){
		long time[10]={0};
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,6,tag,MPI_COMM_WORLD);
       			MPI_Recv(message,msg_sz+1,MPI_CHAR,6,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,strlen(message)+1,MPI_CHAR,6,tag,MPI_COMM_WORLD);
       		MPI_Recv(message,strlen(message)+1,MPI_CHAR,6,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=(final.tv_sec*1000000+final.tv_usec);
		}
		long t = sum(time,10);
		t/=10;
		MPI_Send(&t,1,MPI_LONG,0,tag,MPI_COMM_WORLD);
		//printf("%d: Time taken for %d is %ld usec\n",msg_sz,my_rank,t);
	} else if(my_rank == 3 ){
		long time[10]={0};
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD);
   				MPI_Recv(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD);
   			MPI_Recv(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=(final.tv_sec*1000000+final.tv_usec);
		}
		long t = sum(time,10);
		t=t/10;
		MPI_Send(&t,1,MPI_LONG,0,tag,MPI_COMM_WORLD);
		//printf("%d: Time taken for %d is %ld usec\n",msg_sz,my_rank,t);
	} else if(my_rank==4){
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,0,tag,MPI_COMM_WORLD,&status);
			//memset(message,'b',sizeof(char)*msg_sz);
			//printf("msg received at %d is %s\n",my_rank,message);
			MPI_Send(message,msg_sz+1,MPI_CHAR,0,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==5){
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,1,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,1,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==6){
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,2,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,2,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==7){
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,3,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,3,tag,MPI_COMM_WORLD);
		}
	}
	free(message);
	}//End of msg size for loop
	//printf("Final time in usec is %d.%d\n",time/1000000,time%1000000);
	MPI_Finalize();
	//printf("Exit\n");
	return 0;
}
