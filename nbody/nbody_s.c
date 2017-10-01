#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
//MPI & PThread
//#include <mpi.h>
//#include <pthread.h>

//#include "../hwfarm/hwfarm.h" 

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
    printf("%f\n%f\n%f\n%f\n%f\n%f\n%f\n\n", p.rx,p.ry,p.vx,p.vy,p.fx, p.fy, p.mass);
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
    double EPS = 3E4;      // softening parameter (just to avoid infinities)
    double dx = b.rx - a.rx;
    double dy = b.ry - a.ry;
    double dist = sqrt(dx*dx + dy*dy); 
    double F = (G * a.mass * b.mass) / (dist*dist + EPS*EPS);
    a.fx += F * dx / dist;
    a.fy += F * dy / dist;
    return a;
    
}

void readParticles(struct Particle* particles, int n){
	FILE* f;
	if((f=fopen("test_data_10000","r")) == NULL){
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
    int i=0,j=0;
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
	
	struct Particle particles[N];
	
	readParticles(particles, N);
	/*
	generateParticles(particles, N);
	
	int k;
	for (k = 0; k < N; k++) 
		PrintParticle(particles[k]);
	
	return;
	*/
	
			
	int i=0,j=0;       
	/*
	for (i = 0; i < N; i++)
	{
		PrintParticle(particles[i]);
	}	
*/
	struct timeval now;
	
	//gettimeofday(&now, NULL);	
	//double t1 = now.tv_sec + (now.tv_usec*1.0/1000000);
	//printf ( "%.5f\n", t1);
	
	//gettimeofday(&now, NULL);	
	//double t2 = now.tv_sec + (now.tv_usec*1.0/1000000);
	//printf ( "%.5f\n", t2);
    
    double t1, t2;
    
	int numberofiterations = 50;
	int count = 0;
	while (count < numberofiterations){
		//
		gettimeofday(&now, NULL);	
		t1 = now.tv_sec + (now.tv_usec*1.0/1000000);		
		//
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
		
		gettimeofday(&now, NULL);	
		t2 = now.tv_sec + (now.tv_usec*1.0/1000000);		
		
		printf("%.5f\n", t2-t1);	
		
		count++;
	}

	printf("\n");

	PrintParticle(particles[0]);
	  
	  

	/*
	for (i = 0; i < N; i++)
	{
		PrintParticle(particles[i]);
	}
	* */

	
	return 1;   
}
