#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include "hwfarm.h" 

#include "../hwfarm/hwfarm.h"

struct Poly * Scene;

struct Coord {
   double x,y,z;
   struct Coord * next;
};

struct Vect {
   double A,B,C;
   struct Vect * next;
};

struct Ray {
   struct Coord * c;
   struct Vect * v;
   struct Ray * next;
};

struct Poly {
   int i;
   struct Vect * N;
   struct Coord * Vs;
   struct Poly * next;
};

struct Impact {
   double r;
   int i;
};

struct Impacts{
   struct Impact * head;
   struct Impacts * tail;
};
            
struct MappedRays {
	double cx,cy,cz,va,vb,vc;
};

struct MappedImpacts {
	double r;
	int i;
};              

struct iprvals {
   int xbig,xsmall,ybig,ysmall,zbig,zsmall;
};

struct ripval {
   int b;
   int s;
};

void printcoord(struct Coord * c){
   printf("Coord %lf %lf %lf\n",c->x,c->y,c->z);  
}

void printcoords(struct Coord * c){
   printf("Coords\n");
   while(c!=NULL){  
      printcoord(c);
      c=c->next;
   }
}

void printvect(struct Vect * v){
   printf("Vect %lf %lf %lf\n",v->A,v->B,v->C);  
}

void printvects(struct Vect * v){
   printf("Vects\n");
   while(v!=NULL){  
      printvect(v);
      v=v->next;
   }
}

void printray(struct Ray * r){
   printf("Ray\n");	
   printcoords(r->c);
   printvects(r->v);
}

void printrays(struct Ray * r){
   int i=0;
   while(r!=NULL){  
      printf("i:%d - ",i++);
      printray(r);
      r=r->next;
   }
}

void printpoly(struct Poly * p){
   printf("Poly %d\n",p->i);
   printcoords(p->Vs);
   printvects(p->N);
}

void printpolys(struct Poly * p){
   printf("Polys\n");
   while(p!=NULL){
      printpoly(p);
      p=p->next;
   }
}

void printimpact(struct Impact * i){
   if(i==NULL)
      printf("No impact\n");
   else
      printf("Impact %lf %d\n",i->r,i->i);   
}

void printimpacts(struct Impacts * i){
   printf("Impacts\n");
   while(i!=NULL){  
      printimpact(i->head);
      i=i->tail;
   }
}

void printripval(struct ripval * r){
   printf("%d %d\n",r->b,r->s);  
}

struct MappedRays * mapRays(struct Ray * rays, int raysCount){	
   struct MappedRays * m_rays = (struct MappedRays *)malloc(raysCount * sizeof(struct MappedRays));	
   int i=0;
   struct Ray * r = rays;
   while(r != NULL && i < raysCount){  
      m_rays[i].cx = r->c->x;
      m_rays[i].cy = r->c->y;
      m_rays[i].cz = r->c->z;
      m_rays[i].va = r->v->A;
      m_rays[i].vb = r->v->B;
      m_rays[i].vc = r->v->C;
      r=r->next;
      i++;
   }
	
   return m_rays;
}

void mapImpactsItem(struct MappedImpacts * m_imps, struct Impacts * imp, int impsIndex){
   if(imp->head != NULL){
      m_imps[impsIndex].r = imp->head->r;
      m_imps[impsIndex].i = imp->head->i;
   }else{
      m_imps[impsIndex].r = -1;
      m_imps[impsIndex].i = -1;
   }
}

void mapImpacts(struct MappedImpacts * m_imps, struct Impacts * imps, int impsCount){
   int j=0;
   struct Impacts * imp = imps;	
   while(imp != NULL && j < impsCount){
      mapImpactsItem(m_imps, imp, j);
      imp = imp->tail;
      j++;
   }
}

struct Ray * unmapRays(struct MappedRays * m_rays, int raysCount){	
   int i=0;
   struct Ray * rays = NULL;
   struct Ray * r = rays;
   while(i < raysCount){  
      if(r != NULL){
         r->next = (struct Ray *)malloc(sizeof(struct Ray));
         r = r->next;
      }else{
         rays = (struct Ray *)malloc(sizeof(struct Ray));
         r = rays;
      }
		
      r->c = (struct Coord *)malloc(sizeof(struct Coord));
      r->c->x = m_rays[i].cx;
      r->c->y = m_rays[i].cy;
      r->c->z = m_rays[i].cz;
      r->c->next = NULL;
      r->v = (struct Vect *)malloc(sizeof(struct Vect));
      r->v->A = m_rays[i].va;
      r->v->B = m_rays[i].vb;
      r->v->C = m_rays[i].vc;
      r->v->next = NULL;		
      r->next = NULL;		
      i++;
   }
	
   return rays;
}


struct Impacts * unmapImpacts(struct MappedImpacts * m_imps, int impsCount){
   int j=0;
   struct Impacts * imps = NULL;
   struct Impacts * imp = imps;
   while(j < impsCount){  
      if(imps != NULL){
         imp->tail = (struct Impacts *)malloc(sizeof(struct Impacts));
         imp = imp->tail;
      }else{
         imps = (struct Impacts *)malloc(sizeof(struct Impacts));
         imp = imps;
      }		
      if(m_imps[j].r != -1 && m_imps[j].i != -1){
         imp->head = (struct Impact *)malloc(sizeof(struct Impact));
         imp->head->r = m_imps[j].r;
         imp->head->i = m_imps[j].i;
      }else
         imp->head = NULL;
		
         imp->tail = NULL;		
         j++;
   }
	
   return imps;
}

void printMappedRays(struct MappedRays * m_rays, int raysCount){	
   int i=0;
   while(i < raysCount){
      printf("Ray %d:\n", (i+1));
      printf("CX: %f, CY: %f. CZ: %f, VA: %f, VB: %f, VC: %f\n",  
         m_rays[i].cx, m_rays[i].cy, m_rays[i].cz, m_rays[i].va, m_rays[i].vb, m_rays[i].vc);
      i++;
   }
}

struct Coord * copyCoords(struct Coord * c){  
   if(c == NULL)
      return NULL;
   struct Coord * new_c = NULL;
   struct Coord * new_c_h = new_c;
   while(c != NULL){  
      if(new_c == NULL){
         new_c = (struct Coord *) malloc(sizeof(struct Coord));
         new_c_h = new_c;
      }else{
         new_c_h->next = (struct Coord *) malloc(sizeof(struct Coord));
         new_c_h = new_c_h->next;
      }
      new_c_h->x = c->x;
      new_c_h->y = c->y;
      new_c_h->z = c->z;
      new_c_h->next = NULL;
      c=c->next;
   }
   
   return new_c;
}

struct Vect * copyVects(struct Vect * v){  
   if(v == NULL)
      return NULL;
   struct Vect * new_v = NULL;
   struct Vect * new_v_h = new_v;
   while(v != NULL){  
      if(new_v == NULL){
         new_v = (struct Vect *) malloc(sizeof(struct Vect));
         new_v_h = new_v;
      }else{
         new_v_h->next = (struct Vect *) malloc(sizeof(struct Vect));
         new_v_h = new_v_h->next;
      }
      new_v_h->A = v->A;
      new_v_h->B = v->B;
      new_v_h->C = v->C;
      new_v_h->next = NULL;
      v=v->next;
   }
   
   return new_v;
}

struct Poly * copyPolys(struct Poly * p){
   if(p == NULL)
      return NULL;
   struct Poly * new_p = NULL;
   struct Poly * new_p_h = NULL;
	
   while(p != NULL){
      if(new_p == NULL){
         new_p = (struct Poly *) malloc(sizeof(struct Poly));
         new_p_h = new_p;
      }else{
         new_p_h->next = (struct Poly *) malloc(sizeof(struct Poly));
         new_p_h = new_p_h->next;
      }
      new_p_h->i = p->i;
      new_p_h->Vs = copyCoords(p->Vs);
      new_p_h->N = copyVects(p->N);
      p=p->next;
   }
	
   return new_p;
}

void printiprvals(struct iprvals * i){   
   printf("%d %d %d %d %d %d\n",i->xbig,i->xsmall,i->ybig,
                                 i->ysmall,i->zbig,i->zsmall);  
}

struct iprvals * in_poly_range(double p, double q,
                               double r, struct Coord *Vs){  
   struct iprvals * results;
   results=(struct iprvals *)malloc(sizeof(struct iprvals));
   results->xbig=1;
   results->xsmall=1;
   results->ybig=1;
   results->ysmall=1;
   results->zbig=1;
   results->zsmall=1;
   while(Vs!=NULL){
      results->xbig=results->xbig && p>Vs->x+1E-8;
      results->xsmall=results->xsmall && p<Vs->x-1E-8;
      results->ybig=results->ybig && q>Vs->y+1E-8;
      results->ysmall=results->ysmall && q<Vs->y-1E-8;
      results->zbig=results->zbig && r>Vs->z+1E-8;
      results->zsmall=results->zsmall && r<Vs->z-1E-8;
      Vs=Vs->next;
   }
   return results;
}

int cross_dot_sign (double a, double b, double c, double d, double e, 
                    double f, double A, double B, double C){
   double P,Q,R,cd;
   P=b*f-e*c;
   Q=d*c-a*f;
   R=a*e-d*b;
   cd=P*A+Q*B+R*C;
   if(cd<0.0)
      return -1;
   else
      return 1;
}

struct ripval * really_in_poly(double p, double q, double r, double A, 
                               double B, double C, struct Coord * Vs){
   struct ripval * results;
   int s1;
   if(Vs->next->next==NULL){  
      results=(struct ripval *)malloc(sizeof(struct ripval));
      results->b=1;
      results->s=cross_dot_sign (Vs->next->x-p,Vs->next->y-q,Vs->next->z-r,
                                 Vs->next->x-Vs->x,Vs->next->y-Vs->y,
                                 Vs->next->z-Vs->z,A,B,C);
      return results;
   }
   results=really_in_poly(p,q,r,A,B,C,Vs->next);
   if(results->b){  
      s1=cross_dot_sign (Vs->next->x-p,Vs->next->y-q,Vs->next->z-r,
                         Vs->next->x-Vs->x,Vs->next->y-Vs->y,
                         Vs->next->z-Vs->z,A,B,C);
      if(s1==results->s){  
         results->b=1;
         results->s=s1;  
      }else{
         results->b=0;
         results->s=0;  
      }
   }
   return results;
}

int in_poly_test(double p, double q, double r, double A, 
                  double B, double C, struct Coord * Vs){  
   struct iprvals * iprcheck;
   struct ripval * ripcheck;
   iprcheck = in_poly_range (p,q,r,Vs);
   if(iprcheck->xbig || iprcheck->xsmall ||
    iprcheck->ybig || iprcheck->ysmall ||
    iprcheck->zbig || iprcheck->zsmall){
      free(iprcheck);
      return 0;
   }
   ripcheck=really_in_poly(p,q,r,A,B,C,Vs);
   int b = ripcheck->b;
   free(ripcheck);
   return b;
}

void TestForImpact(struct Ray * ray, struct Poly * poly, struct Impact * imp){
   double px,py,pz; 
   double u,v,w,l,m,n;
   double distance;
   double p,q,r;
   u=ray->c->x;
   v=ray->c->y;
   w=ray->c->z;
   l=ray->v->A;
   m=ray->v->B;
   n=ray->v->C;
   px=poly->Vs->x;
   py=poly->Vs->y;
   pz=poly->Vs->z;
   distance=(poly->N->A*(px-u)+poly->N->B*(py-v)+poly->N->C*(pz-w))/
            (poly->N->A*l+poly->N->B*m+poly->N->C*n);
   p=u+distance*l;
   q=v+distance*m;
   r=w+distance*n;
   if(!in_poly_test(p,q,r,poly->N->A,poly->N->B,poly->N->C,poly->Vs)){
      imp->r = -1;
      imp->i = -1;
   }else{
      imp->r=distance;
      imp->i=poly->i;   
   }
}

void earlier(struct Impact *currentImpact, struct Impact * newImpact){
   if(currentImpact->r != -1 && newImpact->r != -1){
      if(currentImpact->r > newImpact->r){
         currentImpact->r = newImpact->r;	
         currentImpact->i = newImpact->i;
      }
   }
   if(currentImpact->r == -1 && newImpact->r != -1){
      currentImpact->r = newImpact->r;	
      currentImpact->i = newImpact->i;	
   }
}

struct Impact * insert(struct Impacts * i){
   struct Impact * e = NULL;
   while(i != NULL){  
      earlier(i->head,e);
      i = i->tail;
   }
   if(	e == NULL) 
      return NULL;
   else{
      struct Impact * ie;
      ie=(struct Impact *)malloc(sizeof(struct Impact));
      ie->r=e->r;
      ie->i=e->i;
      return ie;
   }
}

struct Impact * FirstImpact(struct Poly * os, struct Ray * r){  
   if(os==NULL)
      return NULL;
   struct Poly * o = os;
   struct Impact* currentImpact = (struct Impact *)malloc(sizeof(struct Impact));
   struct Impact* newImpact = (struct Impact *)malloc(sizeof(struct Impact));	
   TestForImpact( r, o, currentImpact);
   o = o->next;

   while(o != NULL){  
      TestForImpact( r, o, newImpact);	
      earlier(currentImpact, newImpact);		
      o = o->next;
   }
   return currentImpact;	
}

double root(double x, double r){  
   if(x<0.00000001 && -0.00000001<x)
      return 0.0;
   while(fabs((r*r-x)/x)>=0.0000001)
      r=(r+x/r)/2.0;
   return r;
}

struct Vect * VAdd(struct Vect * v1,struct Vect * v2){
   struct Vect * v;
   v=(struct Vect *)malloc(sizeof(struct Vect));
   v->A=v1->A+v2->A;
   v->B=v1->B+v2->B;
   v->C=v1->C+v2->C;
   return v;
}

struct Vect * VMult(double n, struct Vect * v1){
   struct Vect * v;
   v=(struct Vect *)malloc(sizeof(struct Vect));
   v->A=n*v1->A;
   v->B=n*v1->B;
   v->C=n*v1->C;      
   return v;
}

struct Vect * ray_points( int i, int j, int Detail, struct Vect * v, 
                          struct Vect * Vx, struct Vect * Vy){
   struct Vect * iVx,* jVy,* newv;
   if(j==Detail)
      return NULL;    
   if(i==Detail)
      return ray_points(0,j+1,Detail,v,Vx,Vy);
   iVx=VMult(((double)i)/((double)(Detail-1)),Vx);
   jVy=VMult(((double)j)/((double)(Detail-1)),Vy);
   newv=VAdd(VAdd(v,iVx),jVy);
   newv->next=ray_points(i+1,j,Detail,v,Vx,Vy); 
   return newv;
}

struct Ray * GenerateRays(int Det, double X, double Y, double Z){  
   double d;
   double Vza,Vzb,Vzc;
   double ab;
   double ya,yb,yc;
   double ysize;
   struct Vect * v;
   struct Vect * rps,* VX,* VY;
   struct Ray * newrays,* t;
   d=root(X*X+Y*Y+Z*Z,1.0);
   Vza=(-4.0*X/d);Vzb=(-4.0*Y/d);Vzc=(-4.0*Z/d);
   ab=root(Vza*Vza+Vzb*Vzb,1.0);
   VX=(struct Vect *)malloc(sizeof(struct Vect));
   VX->A=Vzb/ab;VX->B=(-Vza/ab);VX->C=0.0;
   ya=Vzb*VX->C-VX->B*Vzc;
   yb=VX->A*Vzc-Vza*VX->C;
   yc=Vza*VX->B-VX->A*Vzb;
   ysize=root(ya*ya+yb*yb+yc*yc,1.0);
   VY=(struct Vect *)malloc(sizeof(struct Vect));
   VY->A=ya/ysize;VY->B=yb/ysize;VY->C=yc/ysize;
   if(VY->C>0.0){  
      VX->A=(-VX->A);VX->B=(-VX->B);VX->C=(-VX->C);
      VY->A=(-VY->A);VY->B=(-VY->B);VY->C=(-VY->C);  
   }
   
   v=(struct Vect *)malloc(sizeof(struct Vect));
   v->A=X+Vza-(VX->A+VY->A)/2.0;
   v->B=Y+Vzb-(VX->B+VY->B)/2.0;
   v->C=Z+Vzc-(VX->C+VY->C)/2.0;
   rps=ray_points(0,0,Det,v,VX,VY);
   
   if(rps==NULL)
      return NULL;
   newrays=(struct Ray *)malloc(sizeof(struct Ray));
   newrays->c=(struct Coord *)malloc(sizeof(struct Coord));
   newrays->c->x=X;
   newrays->c->y=Y;
   newrays->c->z=Z;
   newrays->v=(struct Vect *)malloc(sizeof(struct Vect));
   newrays->v->A=rps->A-X;
   newrays->v->B=rps->B-Y;
   newrays->v->C=rps->C-Z;
   t=newrays;
   rps=rps->next;
   
   while(rps!=NULL){  
      t->next=(struct Ray *)malloc(sizeof(struct Ray));
      t=t->next;
      t->c=(struct Coord *)malloc(sizeof(struct Coord));
      t->c->x=X;
      t->c->y=Y;
      t->c->z=Z;
      t->v=(struct Vect *)malloc(sizeof(struct Vect));
      t->v->A=rps->A-X;
      t->v->B=rps->B-Y;
      t->v->C=rps->C-Z;
      rps=rps->next;
   }
   t->next=NULL;
   
   return newrays;
}

void showimps(int dv, int i, struct Impacts * imps){  
   FILE *fres = fopen("finalResult.txt","w");
   while(imps != NULL){  
      if(i==0){  
         fprintf(fres,"\n");
         i=dv;
      }
      if(imps->head == NULL){			
         fprintf(fres,"%2c",'.');			
      }else{
         fprintf(fres,"%2d",imps->head->i);
      }
      fprintf(fres," ");
      i=i-1;
      imps=imps->tail;
   }
   fprintf(fres,"\n");
   fclose(fres);
}

struct Poly * getPoly(FILE * scene_file, int id){  
   struct Poly * p;
   struct Coord * t;
   p=(struct Poly *) malloc(sizeof(struct Poly));
   p->i=id;
   p->N=(struct Vect *)malloc(sizeof(struct Vect));
   int numOfI = -1;
   numOfI = fscanf(scene_file,"%lf %lf %lf",&(p->N->A),&(p->N->B),&(p->N->C));
   numOfI = fscanf(scene_file,"%d",&id);
   p->Vs=(struct Coord *)malloc(sizeof(struct Coord));
   t=p->Vs;
   numOfI = fscanf(scene_file,"%lf %lf %lf",&(t->x),&(t->y),&(t->z));
   t=p->Vs;
   while(--id){
      t->next=(struct Coord *)malloc(sizeof(struct Coord));
      t=t->next;
      numOfI = fscanf(scene_file,"%lf %lf %lf",&(t->x),&(t->y),&(t->z));
   }
   t->next=NULL;
   return p;
}

struct Poly * getScene(char * scene_file_name, int limit){
   FILE * scene_file = fopen(scene_file_name,"r");
   if(scene_file == NULL){  
      printf("can't open %s\n",scene_file_name);
      exit(0);
   }
	  
   struct Poly * s,* t;
   int id;
   if(fscanf(scene_file,"%d",&id) == EOF){  
      fclose(scene_file);
      return NULL;
   }
   s = getPoly(scene_file,id);
   t=s;
   int i=0;
   while(fscanf(scene_file,"%d",&id) != EOF){  
      if(limit <= i++)	   
         break;
      t->next = getPoly(scene_file,id);
      t = t->next;
   }
   t->next = NULL;
   fclose(scene_file);
   return s;
}

void hwfarm_rt( hwfarm_task_data* t_data, chFM checkForMobility){					
	
	struct Ray * rays = unmapRays(t_data->input_data, t_data->input_len);
		
	if(rays==NULL)
	return;
	
	//Get the counter value
	int *main_index = (t_data->counter);
	
	//The head of the imapct array
	struct Impacts * imps = NULL;
	// An auxiliary pointer 
	struct Impacts * t = imps;
	int i=0;
	
	//Navigate to the unprocessed ray ( to consider the moved tasks)
	while(rays!=NULL && i < *main_index){
		rays=rays->next;
		i++;
	}
	
	while(rays!=NULL){  
		   
		if(imps == NULL){
			imps = (struct Impacts *)malloc(sizeof(struct Impacts));
			t = imps;
		}else{
			t->tail=(struct Impacts *)malloc(sizeof(struct Impacts));
			t=t->tail;
		}
		
		t->head = FirstImpact(Scene, rays);

		rays=rays->next;

		mapImpactsItem(t_data->output_data, t, *main_index);

		*main_index = (*main_index) + 1;

		checkForMobility();
	}
	t->tail=NULL;
	
}

int main(int argc, char** argv){ 	
	initHWFarm(argc,argv); 
	
	//Details refers to the number of rays in one dimension
	int Details = atoi(argv[1]);
	int chunk = atoi(argv[2]);
	char* scene_file = (argv[3]);
	int scene_limit = atoi(argv[4]);
	int mobility = atoi(argv[5]);
	int problem_size = Details * Details;	
	int tasks = problem_size / chunk;
	
	float ViewX = 10.0;
	float ViewY = 10.0;
	float ViewZ = 10.0;
	
	//input
	void * input_data = NULL;	
	int input_data_size = sizeof(struct MappedRays);
	int input_data_len = chunk;
		
	//output
	void * output_data = NULL;	
	int output_data_size = sizeof(struct MappedImpacts);
	int output_data_len = chunk;
	
	hwfarm_state main_state;
	main_state.counter = 0;
	main_state.max_counter = chunk;
	main_state.state_data = NULL;
	main_state.state_len = 0;	
	
	if(rank == 0)
	{
		//Input data
		struct Ray * rays;		
		rays=GenerateRays(Details,ViewX,ViewY,ViewZ);		
		struct MappedRays * mapped_rays;
		mapped_rays = mapRays(rays, Details*Details);	
		input_data = mapped_rays;		
		
		//Output Data		
		output_data = malloc(output_data_size * problem_size);
				
	}else{		
		Scene = getScene(scene_file, scene_limit); 
	}
	
	hwfarm( hwfarm_rt, tasks,
		    input_data, input_data_size, input_data_len, 
		    NULL, 0, 0, 
		    output_data, output_data_size, output_data_len, 
		    main_state, mobility);	
	
	if(rank == 0){
		struct Impacts * new_imps = unmapImpacts(output_data, problem_size);	
		//Print impact to a file
		showimps(Details, Details, new_imps);	
	}

	finalizeHWFarm();
	
	return 1;  
}
