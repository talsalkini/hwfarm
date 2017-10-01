#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hwfarm.h" 

#define N 100000
#define G 6.673e-11
#define TIMESTAMP 1e11

struct Particle{
   double rx, ry; //position components
   double vx, vy; //velocity components
   double fx, fy; //force components
   double mass;   //mass of the particle
};

struct Particle Update(struct Particle p, double timestamp){
   p.vx += timestamp*p.fx / p.mass;
   p.vy += timestamp*p.fy / p.mass;
   p.rx += timestamp*p.vx;
   p.ry += timestamp*p.vy;
   return p;
}

void PrintParticle(struct Particle p){
   printf("[%d]. %f\n%f\n%f\n%f\n%f\n%f\n%f\n\n", rank, p.rx,p.ry,p.vx,p.vy,p.fx, p.fy, p.mass);
}

struct Particle CopyParticle(struct Particle p_to, struct Particle p_from){
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
struct Particle ResetForce(struct Particle p){
   p.fx = 0.0;
   p.fy = 0.0;
   return p;
}

//Add force to particle a by particle b
struct Particle AddForce(struct Particle a,struct Particle b){
   //To avoid infinities
   double EPS = 3E4;
   double dx = b.rx - a.rx;
   double dy = b.ry - a.ry;
   double dist = sqrt(dx*dx + dy*dy);
   double F = (G * a.mass * b.mass) / (dist*dist + EPS*EPS);
   a.fx += F * dx / dist;
   a.fy += F * dy / dist;
   return a;
}

void hwfarm_nbody( hwfarm_task_data* t_data, chFM checkForMobility){
   int *i = t_data->counter;
   int *i_max = t_data->counter_max;
   int shared_len = t_data->shared_len;
   int cur_i = 0, j = 0;
   struct Particle *shared_p = (struct Particle *)t_data->shared_data;
   struct Particle *output_p = (struct Particle *)t_data->output_data;
   while(*i < *i_max){
      cur_i = (*i) + (*(t_data->counter_max) * t_data->task_id);		
      output_p[*i] = CopyParticle(output_p[*i], shared_p[cur_i]);
      output_p[*i] = ResetForce(output_p[*i]);		
      for (j = 0; j < shared_len; j++){
         if (cur_i != j){
            output_p[*i] = AddForce(output_p[*i], shared_p[j]);
         }
      }
      output_p[*i] = Update(output_p[*i], TIMESTAMP);
      (*i)++;		
      checkForMobility();
   }
}

void readParticles(struct Particle* particles, int n){
   FILE* f;
   if((f=fopen("test_data","r")) == NULL){
      printf("Cannot open file.\n");
      exit(0);
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

int main(int argc, char** argv){ 	
   initHWFarm(argc,argv); 
	
   int problem_size = N;
   int chunk = atoi(argv[1]);	
   int tasks = problem_size / chunk;
   int mobility = 1;
		
   //input
   void * input_data = NULL;
   int input_data_size = 0;
   int input_data_len = 0;	
		
   //shared
   struct Particle * shared_data = NULL;
   int shared_data_size = sizeof(struct Particle);
   int shared_data_len = N;
	
   //output
   struct Particle *output_data = NULL;
   int output_data_size = sizeof(struct Particle);
   int output_data_len = chunk;
	
   hwfarm_state main_state;
   main_state.counter = 0;
   main_state.max_counter = chunk;
   main_state.state_data = NULL;
   main_state.state_len = 0;	
	
   if(rank == 0){		
      struct Particle * particles = (struct Particle *)malloc(sizeof(struct Particle)*N);
				
      readParticles(particles, N);							
		
      //Shared data		
      shared_data = particles;

      //Output Data		
      output_data = malloc(problem_size * output_data_size);
   } 
	
   int numberofiterations = 10;
   int count = 0;
   while (count < numberofiterations){		
   hwfarm( hwfarm_nbody, tasks,
         input_data, input_data_size, input_data_len, 
         shared_data, shared_data_size, shared_data_len, 
         output_data, output_data_size, output_data_len, 
         main_state, mobility);		 
		    
      if(rank == 0){
         shared_data = output_data;
      }		
      count++;		
   }
   if(rank == 0){
      //print the output to a file (output_data, problem_size)
   }
	
   finalizeHWFarm();
	
   return 1;   
}
