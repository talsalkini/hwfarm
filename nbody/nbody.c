#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
//MPI & PThread
#include <mpi.h>
#include <pthread.h>

#include "../hwfarm/hwfarm.h" 

#define N 100000
#define G 6.673e-11
#define TIMESTAMP 1e11

struct Particle{
    double rx, ry;//position components
    double vx, vy;//velocity components
    double fx, fy;//force components
    double mass;//mass of the particle
};

struct Particle Update(struct Particle p, double timestamp)
{
    p.vx += timestamp*p.fx / p.mass;
    p.vy += timestamp*p.fy / p.mass;
    p.rx += timestamp*p.vx;
    p.ry += timestamp*p.vy;
    return p;
}
void PrintParticle(struct Particle p)
{
    //printf("rx == %f \n ry == %f \n vx == %f \n vy == %f \n fx == %f\n fy == %f\n mass == %f\n\n", p.rx,p.ry,p.vx,p.vy,p.fx, p.fy, p.mass);
     printf("[%d]. %f\n%f\n%f\n%f\n%f\n%f\n%f\n\n", rank, p.rx,p.ry,p.vx,p.vy,p.fx, p.fy, p.mass);
}

//Reset the forces on particle
struct Particle CopyParticle(struct Particle p_to, struct Particle p_from)
{
    p_to.fx = p_from.fx;    
    p_to.fy = p_from.fy;
    p_to.rx = p_from.rx;
    p_to.ry = p_from.ry;
    p_to.vx = p_from.vx;
    p_to.vy = p_from.vy;
    p_to.mass = p_from.mass;
    return p_to;
}
//Reset the forces on particle
struct Particle ResetForce(struct Particle p)
{
    p.fx = 0.0;
    p.fy = 0.0;
    return p;
}
//Add force to particle a by particle b
struct Particle AddForce(struct Particle a,struct Particle b)
{
	//printf("[%d]. START PRINTING IN FORCE------***************\n", rank);
	//PrintParticle(a);
	//PrintParticle(b);
	//printf("[%d]. MID PRINTING IN FORCE------***************\n", rank);
    double EPS = 3E4;      // softening parameter (just to avoid infinities)
    double dx = b.rx - a.rx;
    double dy = b.ry - a.ry;
    double dist = sqrt(dx*dx + dy*dy);
    double F = (G * a.mass * b.mass) / (dist*dist + EPS*EPS);
    a.fx += F * dx / dist;
    a.fy += F * dy / dist;
    //PrintParticle(a);
    //printf("[%d]. END PRINTING IN FORCE------***************\n", rank);
    return a;

}


void hwfarm_nbody( hwfarm_task_data* t_data, chFM checkForMobility){
	
	/*
	printf("[%d]. *******-----------************------***************\n", rank);
	printf("[%d]. task_id: %d\n", rank, t_data->task_id);
	printf("[%d]. counter: %d\n", rank, *(t_data->counter));
	printf("[%d]. shared_len: %d\n", rank, t_data->shared_len);
	*/
	
	int *i = t_data->counter;
	int *i_max = t_data->counter_max;
	int shared_len = t_data->shared_len;
	int cur_i = 0;
	struct Particle *shared_p = (struct Particle *)t_data->shared_data;
	int j = 0;
	//for (j = 0; j < shared_len; j++)
	//{
		//PrintParticle(shared_p[j]);
	//}
	
	struct Particle *output_p = (struct Particle *)t_data->output_data;
	while(*i < *i_max){
		
		
		cur_i = (*i) + (*(t_data->counter_max) * t_data->task_id);
		
		//printf("[%d]. i: %d, cur_i: %d\n", rank, *i, cur_i);		
		
		output_p[*i] = CopyParticle(output_p[*i], shared_p[cur_i]);
		
		//printf("[%d]. CYPIED --------************------***************\n", rank);
		
		//PrintParticle(output_p[*i]);
		
		output_p[*i] = ResetForce(output_p[*i]);
		
		for (j = 0; j < shared_len; j++)
		{
			//if (*i != j)
			if (cur_i != j)
			{
				//printf("[%d]. LOOP**** (*i: %d, j: %d[cur_i: %d])\n", rank, *i, j, cur_i);
				output_p[*i] = AddForce(output_p[*i], shared_p[j]);
			}

		}
		
		//PrintParticle(output_p[*i]);
		
		//printf("[%d]. FORCED --------************------***************\n", rank);
		
		//PrintParticle(output_p[*i]);
		
		output_p[*i] = Update(output_p[*i], TIMESTAMP);
		
		//printf("[%d]. UPDATED --------************------***************\n", rank);
		
		//PrintParticle(output_p[*i]);

		(*i)++;
		
		checkForMobility();
	}
	/*
	printf("[%d]. ALL OUTPUT--------************------***************\n", rank);
	
	for (j = 0; j < *i_max; j++)
	{
		PrintParticle(output_p[j]);
	}
	
	printf("[%d]. *******-----------************------***************\n", rank);
	* */
}

void* initOutputData(void* output_data_in, int len){
	
	printf("[%d]. *******-----------************------*************** (%d)\n", rank, len);
	
	void* output_data = (void*)malloc(sizeof(void)*(len));
	
	return output_data;
}

void readParticles(struct Particle* particles, int n){
	FILE* f;
	if((f=fopen("test_data_50000","r")) == NULL){
		printf("Cannot open file.\n");
	}
	double d = 0.0;
	
	int l = 0;
	int i = 0;
	while(i<n){
		l = fscanf(f, "%lf\n", &d);
		particles[i].rx = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].ry = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].vx = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].vy = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].fx = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].fx = d;	
		l = fscanf(f, "%lf\n", &d);
		particles[i].mass = d;			
		
		i++;
		
	}
	
	fclose(f);
	
	
}

void generateParticles(struct Particle* particles, int n){
	srand(time(NULL));
    //randomly generating N Particles
    int i=0;
    for (i = 0; i < N; i++){
        double rx = 1e18*exp(-1.8)*(.5 - rand());
        particles[i].rx = rx;
        double ry = 1e18*exp(-1.8)*(.5 - rand());
        particles[i].ry = ry;
        double vx = 1e18*exp(-1.8)*(.5 - rand());
        particles[i].vx = vx;
        double vy = 1e18*exp(-1.8)*(.5 - rand());
        particles[i].vy = vy;
        double mass = 1.98892e30*rand()*10 + 1e20;
        particles[i].mass = mass;	
	}
}

int main(argc,argv)
int argc;
char **argv;
{ 
	initHWFarm(argc,argv);
	
	if(argc != 2){  
		printf("mpirun ... <binary file> <tasks>\n");
		exit(0);
	} 
	
	int problem_size = N;
	int tasks = atoi(argv[1]);	

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
	
	int * input_data = NULL;	
	struct Particle * shared_data = NULL;
	struct Particle *output_data = NULL;

	int mobility = 1;
		
	//input
	int input_data_size = 0;
	int input_data_len = 0;	
		
	//shared
	int shared_data_size = sizeof(struct Particle);
	int shared_data_len = N;
	
	//output
	int output_data_size = sizeof(struct Particle);
	int output_data_len = chunk;
	int output_data_total = N;
	
	//int start_particle = (problem_size / tasks) + ;

	hwfarm_state main_state;
	main_state.counter = 0;
	main_state.max_counter = chunk;
	main_state.state_data = NULL;
	main_state.state_len = 0;	
	
	//int i=0;
	
	
	//struct Particle * particles = (struct Particle *) malloc(sizeof(struct Particle *) * N);
	//struct Particle particles[N];
	
	//randomly generating N Particles	
	
	
	
	//generateParticles(particles, N);
	
	if(rank == 0)
	{
		
		printf("MASTER START: %d\n", chunk);	
		
		struct Particle * particles = (struct Particle *)malloc(sizeof(struct Particle)*N);
				
		readParticles(particles, N);							
		
		//PrintParticle(particles[0]);
				
		//prepare data to be sent to the workers			
		
		//Shared data		
		shared_data = particles;
		
		///PrintParticle(shared_data[0]);

		//Output Data		
		output_data = (struct Particle*)initOutputData(output_data, output_data_total*output_data_size);		
		
		start_time = MPI_Wtime();
		printf("MASTER START: %.6f\n",MPI_Wtime());	
	} 
	
	/*
	printf("[%d]. START...\n",rank);	
	if(rank == 0)
	{
		for (i = 0; i < N; i++)
		{
			PrintParticle(particles[i]);
		}
	}
	* */
	/*
	printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	
	hwfarm( hwfarm_nbody, tasks,
		    input_data, input_data_size, input_data_len, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
	
	if(rank == 0){
		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);	    

		for (i = 0; i < N; i++)
		{
			printf("[%d]. -------------------------------------------\n",rank);
			PrintParticle(output_data[i]);
			printf("[%d]. -------------------------------------------\n",rank);
		}

		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	}
	
	if(rank == 0){
		shared_data = output_data;
	}
	
	main_state.counter = 0;
	
	hwfarm( hwfarm_nbody, tasks,
		    input_data, input_data_size, input_data_len, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
		    
	if(rank == 0){
		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);	    

		for (i = 0; i < N; i++)
		{
			printf("[%d]. -------------------------------------------\n",rank);
			PrintParticle(output_data[i]);
			printf("[%d]. -------------------------------------------\n",rank);
		}

		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	}
	
	if(rank == 0){
		shared_data = output_data;
	}
	
	main_state.counter = 0;
	
	hwfarm( hwfarm_nbody, tasks,
		    input_data, input_data_size, input_data_len, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
		    
	if(rank == 0){
		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);	    

		for (i = 0; i < N; i++)
		{
			printf("[%d]. -------------------------------------------\n",rank);
			PrintParticle(output_data[i]);
			printf("[%d]. -------------------------------------------\n",rank);
		}

		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	}
	
	if(rank == 0){
		shared_data = output_data;
	}
	
	main_state.counter = 0;
	
	hwfarm( hwfarm_nbody, tasks,
		    input_data, input_data_size, input_data_len, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);
		    
	if(rank == 0){
		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);	    

		for (i = 0; i < N; i++)
		{
			printf("[%d]. -------------------------------------------\n",rank);
			PrintParticle(output_data[i]);
			printf("[%d]. -------------------------------------------\n",rank);
		}

		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	}
	
	*/
	
	int numberofiterations = 10;
	int count = 0;
	while (count < numberofiterations){
		
		hwfarm( hwfarm_nbody, tasks,
		    input_data, input_data_size, input_data_len, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);		 
		    
		//printf("[%d]. Shared_data: %p\n",rank, shared_data);			
		    
		if(rank == 0){
			shared_data = output_data;
		}
		
		//main_state.counter = 0;		
		
		count++;
		
		printf("[%d]. count = %d of %d +++++++++++++++++++++++++++++++++++++++++++++++\n",rank, count, numberofiterations);	
	}
	if(rank == 0){
		PrintParticle(output_data[0]);
	}
	
	/*
	if(rank == 0){
		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);	    

		for (i = 0; i < N; i++)
		{
			printf("[%d]. -------------------------------------------\n",rank);
			//PrintParticle(output_data[i]);
			printf("[%d]. -------------------------------------------\n",rank);
		}

		printf("[%d]. +++++++++++++++++++++++++++++++++++++++++++++++\n",rank);
	}*/
    /*
	if(rank == 0){
		int j=0;
		int numberofiterations2 = 5;
		int count2 = 0;
		while (count2 < numberofiterations2){
			for (i = 0; i < N; i++)
			{
				particles[i] = ResetForce(particles[i]);
				for (j = 0; j < N; j++)
				{
					if (i != j)
					{
						particles[i] = AddForce(particles[i], particles[j]);
					}

				}
			}
			//loop again to update the time stamp here
			for (i = 0; i < N; i++)
			{
				particles[i] = Update(particles[i], TIMESTAMP);
			}
			
			
			count2++;
		} 
		
		for (i = 0; i < N; i++)
		{
			PrintParticle(particles[i]);
		}
	}
    */
    finalizeHWFarm();
	
	return 1;   
}
