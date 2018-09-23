#include <stdlib.h>
#include <stdio.h>
#include <cuda.h>
#include <math.h>
#include <curand_kernel.h>

#define ITER_PER_THREAD 256
#define NUMBER_OF_THREAD 256

__global__ void pi_cal(long *niter, long *a,curandState *state){
	long idx = (blockDim.x * blockIdx.x) + threadIdx.x; 
	long count = 0;
	float x,y,z;
	curand_init(110919, idx , 0, &state[idx]);
	for(long i=0;i<ITER_PER_THREAD;i++){
		x = curand_uniform(&state[idx]);
		y = curand_uniform(&state[idx]);
		z = (x*x)+(y*y);
		if((idx*ITER_PER_THREAD+i) < (*niter) && z<=1.0f){
			count+=1;
		}
	}
	a[idx] = count;
}


int main(int argc,char **argv){
	if(argc!=2){
		printf("Usage: <exefile> <number_of_iteration>\n");
		return 0;
	}
	long niter = atol(argv[1]);
	int number_of_blocks = ceil(((double)(niter)/(ITER_PER_THREAD*NUMBER_OF_THREAD)));
	double pi =0.0;
	//printf("number of_block %d\n",number_of_blocks);
	long m_size  = number_of_blocks * NUMBER_OF_THREAD;
	long *final_memory = (long *) malloc(m_size*sizeof(long));
	memset(final_memory,0,m_size*sizeof(long));

	long *d_a;
	long *d_iter;
	curandState *d_states;

	//allocate memory for current state to generate random number on device
	cudaMalloc((void **)&d_a,m_size*sizeof(long));
	cudaMalloc((void **)&d_states,m_size*sizeof(curandState));
	cudaMalloc((void **)&d_iter, sizeof(long));
	
	//copy the value of niter to device iter
	cudaMemcpy(d_iter,&niter,sizeof(long),cudaMemcpyHostToDevice);

	//invoke gpu kernal program for pi calculation
	pi_cal<<<number_of_blocks,NUMBER_OF_THREAD>>>(d_iter,d_a,d_states);

	//copy the result from d_a to hosts final_memory array
	cudaMemcpy(final_memory,d_a,m_size*sizeof(long),cudaMemcpyDeviceToHost);
	
	//Now sum all the values in host final_memory to pi	
	for(long i=0;i<m_size;i++){
		pi+=final_memory[i];
	}
	//printf("m_size %d pi %lf\n",m_size,pi);
	//divide the final pi value by size
	pi = (double)(4.0 * pi) / (double)niter;
	//pi /= m_size;

	printf("# of trials= %d , estimate of pi is %.16lf \n",niter,pi);

	//free the allocated memory
	free(final_memory);
	cudaFree(d_a);
	cudaFree(d_states);
	cudaFree(d_iter);
	return 0;
}
