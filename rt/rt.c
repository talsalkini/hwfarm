#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//MPI & PThread
#include <mpi.h>
#include <pthread.h>

#include "../hwfarm/hwfarm.h" 




/*
 * *********************************************************************
 * ************************ Ray Tracer *********************************
 * *********************************************************************
 * */
 
struct Poly * Scene;

struct Coord {double x,y,z;
              struct Coord * next;};

void printcoord(c)
struct Coord * c;
{  printf("Coord %lf %lf %lf\n",c->x,c->y,c->z);  }

void printcoords(c)
struct Coord * c;
{  printf("Coords\n");
   while(c!=NULL)
   {  printcoord(c);
      c=c->next;
   }
}

struct Vect {double A,B,C;
             struct Vect * next;};

void printvect(v)
struct Vect * v;
{  printf("Vect %lf %lf %lf\n",v->A,v->B,v->C);  }

void printvects(v)
struct Vect * v;
{  printf("Vects\n");
   while(v!=NULL)
   {  printvect(v);
      v=v->next;
   }
}

struct Ray {struct Coord * c;
            struct Vect * v;
            struct Ray * next;};

struct Poly {int i;
             struct Vect * N;
             struct Coord * Vs;
             struct Poly * next;};

struct Impact {double r;int i;};
struct Impacts{struct Impact * head;
               struct Impacts * tail;};


            
struct MappedRays {
	double cx,cy,cz,va,vb,vc;
};

struct MappedImpacts {
	double r;
	int i;
};              

void printray(r)
struct Ray * r;
{  printf("Ray\n");
	//printf("Ray[%x][%x]\n",*(r->c),*(r->v));
   printcoords(r->c);
   printvects(r->v);
}

void printrays(r, source)
struct Ray * r;
char source;
{  
	printf("[%d]. Rays @ %c\n", rank, source);
	int i=0;
	while(r!=NULL){  
		//if(source == 'w')
		printf("i:%d - ",i++);
		printray(r);
		r=r->next;
	}
}

struct MappedRays * mapRays(struct Ray * rays, int raysCount){
	
	struct MappedRays * m_rays = (struct MappedRays *)malloc(raysCount * sizeof(struct MappedRays));
	
	printf("[%d]. Mapping Rays @ M\n", rank);
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
	
	//struct MappedImpacts * m_imps = (struct MappedImpacts *)malloc(impsCount * sizeof(struct MappedImpacts));
	
	printf("[%d]. Mapping Impacts @ M\n", rank);
	int j=0;
	struct Impacts * imp = imps;
	
	while(imp != NULL && j < impsCount){
		mapImpactsItem(m_imps, imp, j);
		imp = imp->tail;
		j++;
	}
}

struct Ray * unmapRays(struct MappedRays * m_rays, int raysCount){
	
	//struct Ray * m_rays = (struct Ray *)malloc(raysCount * sizeof(struct Ray));
	
	printf("[%d]. Unmapping Rays @ M\n", rank);
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
	
	printf("[%d]. Unmapping Impacts @ M\n", rank);
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
	printf("[%d]. Mapped Rays @ M\n", rank);
	int i=0;
	while(i < raysCount){
		printf("Ray %d:\n", (i+1));
		printf("CX: %f, CY: %f. CZ: %f, VA: %f, VB: %f, VC: %f\n",  
			m_rays[i].cx, m_rays[i].cy, m_rays[i].cz, m_rays[i].va, m_rays[i].vb, m_rays[i].vc);
		i++;
	}
}

struct Coord * copyCoords(c)
struct Coord * c;
{  
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

struct Vect * copyVects(v)
struct Vect * v;
{  
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

struct Poly * copyPolys(p)
struct Poly * p;
{  	
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

void printpoly(p)
struct Poly * p;
{  printf("Poly %d\n",p->i);
   printcoords(p->N);
   printvects(p->Vs);
}

void printpolys(p)
struct Poly * p;
{  printf("Polys\n");
   while(p!=NULL)
   {  printpoly(p);
      p=p->next;
   }
}

void printimpact(i)
struct Impact * i;
{  if(i==NULL)
    printf("No impact\n");
   else
    printf("Impact %lf %d\n",i->r,i->i);   }

void printimpacts(i)
struct Impacts * i;
{  printf("Impacts\n");
   while(i!=NULL)
   {  printimpact(i->head);
      i=i->tail;
   }
}

struct iprvals {int xbig,xsmall,ybig,ysmall,zbig,zsmall;};

void printiprvals(i)
struct iprvals * i;
{   printf("%d %d %d %d %d %d\n",i->xbig,i->xsmall,i->ybig,
                                 i->ysmall,i->zbig,i->zsmall);  }

struct iprvals * in_poly_range(p,q,r,Vs)
double p,q,r;
struct Coord * Vs;
{  struct iprvals * results;
   results=(struct iprvals *)malloc(sizeof(struct iprvals));
   results->xbig=1;
   results->xsmall=1;
   results->ybig=1;
   results->ysmall=1;
   results->zbig=1;
   results->zsmall=1;
   while(Vs!=NULL)
   {  results->xbig=results->xbig && p>Vs->x+1E-8;
      results->xsmall=results->xsmall && p<Vs->x-1E-8;
      results->ybig=results->ybig && q>Vs->y+1E-8;
      results->ysmall=results->ysmall && q<Vs->y-1E-8;
      results->zbig=results->zbig && r>Vs->z+1E-8;
      results->zsmall=results->zsmall && r<Vs->z-1E-8;
      Vs=Vs->next;
   }
   return results;
}

int cross_dot_sign (a,b,c,d,e,f,A,B,C)
double a,b,c,d,e,f,A,B,C;
{ double P,Q,R,cd;
  P=b*f-e*c;
  Q=d*c-a*f;
  R=a*e-d*b;
  cd=P*A+Q*B+R*C;
  if(cd<0.0)
   return -1;
  else
   return 1;
}

struct ripval {int b,s;};

void printripval(r)
struct ripval * r;
{  printf("%d %d\n",r->b,r->s);  }

struct ripval * really_in_poly(p,q,r,A,B,C,Vs)
double p,q,r,A,B,C;
struct Coord * Vs;
{  struct ripval * results;
   int s1;
   if(Vs->next->next==NULL)
   {  results=(struct ripval *)malloc(sizeof(struct ripval));
      results->b=1;
      results->s=cross_dot_sign (Vs->next->x-p,Vs->next->y-q,Vs->next->z-r,
                                 Vs->next->x-Vs->x,Vs->next->y-Vs->y,
                                 Vs->next->z-Vs->z,A,B,C);
      return results;
   }
   results=really_in_poly(p,q,r,A,B,C,Vs->next);
   if(results->b)
   {  s1=cross_dot_sign (Vs->next->x-p,Vs->next->y-q,Vs->next->z-r,
                         Vs->next->x-Vs->x,Vs->next->y-Vs->y,
                         Vs->next->z-Vs->z,A,B,C);
      if(s1==results->s)
      {  results->b=1;
         results->s=s1;  }
      else
      {  results->b=0;
         results->s=0;  }
   }
   return results;
}

int in_poly_test (p,q,r,A,B,C,Vs)
double p,q,r,A,B,C;
struct Coord * Vs;
{  struct iprvals * iprcheck;
   struct ripval * ripcheck;
   iprcheck = in_poly_range (p,q,r,Vs);
   if(iprcheck->xbig || iprcheck->xsmall ||
      iprcheck->ybig || iprcheck->ysmall ||
      iprcheck->zbig || iprcheck->zsmall){
		return 0;
   }
   ripcheck=really_in_poly(p,q,r,A,B,C,Vs);
   int b = ripcheck->b;
   return b;
}

int in_poly_test2(p,q,r,A,B,C,Vs)
double p,q,r,A,B,C;
struct Coord * Vs;
{  struct iprvals * iprcheck;
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

struct Impact * TestForImpact(ray,poly)
struct Ray * ray;
struct Poly * poly;
{  double px,py,pz; 
   double u,v,w,l,m,n;
   double distance;
   double p,q,r;
   struct Impact * imp;
   //printf("		*****\n");
   u=ray->c->x;v=ray->c->y;w=ray->c->z;
   l=ray->v->A;m=ray->v->B;n=ray->v->C;
   //printf("		*****\n");
   px=poly->Vs->x;
   py=poly->Vs->y;
   pz=poly->Vs->z;
   //printf("		*****\n");
   distance=(poly->N->A*(px-u)+poly->N->B*(py-v)+poly->N->C*(pz-w))/
            (poly->N->A*l+poly->N->B*m+poly->N->C*n);
   p=u+distance*l;
   //printf("		*****\n");
   q=v+distance*m;
   r=w+distance*n;
   //printf("		*****\n");
   if(!in_poly_test(p,q,r,poly->N->A,poly->N->B,poly->N->C,poly->Vs))
    return NULL;
   
   imp=(struct Impact *)malloc(sizeof(struct Impact));
   imp->r=distance;
   imp->i=poly->i;
   //printf("		*****\n");
   return imp;
}

void TestForImpact2(ray,poly, imp)
struct Ray * ray;
struct Poly * poly;
struct Impact * imp;
{  double px,py,pz; 
   double u,v,w,l,m,n;
   double distance;
   double p,q,r;
  // struct Impact * imp;
   //printf("		*****\n");
   u=ray->c->x;
   v=ray->c->y;
   w=ray->c->z;
   l=ray->v->A;
   m=ray->v->B;
   n=ray->v->C;
   //printf("		*****\n");
   px=poly->Vs->x;
   py=poly->Vs->y;
   pz=poly->Vs->z;
   //printf("		*****\n");
   distance=(poly->N->A*(px-u)+poly->N->B*(py-v)+poly->N->C*(pz-w))/
            (poly->N->A*l+poly->N->B*m+poly->N->C*n);
   p=u+distance*l;
   //printf("		*****\n");
   q=v+distance*m;
   r=w+distance*n;
   //printf("		*****\n");
	if(!in_poly_test2(p,q,r,poly->N->A,poly->N->B,poly->N->C,poly->Vs)){
		imp->r = -1;
		imp->i = -1;
	}else{
		imp->r=distance;
		imp->i=poly->i;   
	}
}

void earlier2(currentImpact, newImpact)
struct Impact * currentImpact, * newImpact;
{  
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

struct Impact * earlier(imp1,imp2)
struct Impact * imp1, * imp2;
{  
	if(imp1==NULL && imp2==NULL)
		return NULL;
	if(imp2==NULL)
		return imp1;
	if(imp1==NULL)
		return imp2;
	if(imp1->r<imp2->r)
		return imp1;
	return imp2;
}


struct Impact * insert(i)
struct Impacts * i;
{  
	struct Impact * e = NULL;
	while(i != NULL)
	{  
		e = earlier(i->head,e);
		i = i->tail;
	}
	///Skeleton
	if(	e == NULL) 
		return NULL;
	else{
		struct Impact * ie;
		ie=(struct Impact *)malloc(sizeof(struct Impact));
		ie->r=e->r;
		ie->i=e->i;
		///
		return ie;
	}
}

int X = 0;

void freeH(struct Impacts * h){
	if(h->tail != NULL)
		freeH(h->tail);
	if(h->head != NULL)
		free(h->head);
	free(h);
	X++;
}

struct Impact * FirstImpact(os, r)
struct Poly * os;
struct Ray * r;
{  
	//printf("*****\n");
	struct Impacts * h,* t;
	if(os==NULL)
		return NULL;
	struct Poly * o = os;
	//printf("*****\n");
	h = (struct Impacts *)malloc(sizeof(struct Impacts));
	//printf("*****\n");
	h->head = TestForImpact( r, o);
	//printf("*****\n");
	t = h;
	o = o->next;
	//printf("*****\n");

	while(o != NULL)
	{  
		//printf("[%d]***[j:%d]***\n",rank,++j);
		t->tail = (struct Impacts *)malloc(sizeof(struct Impacts));
		t = t->tail;
		//printf("*****\n");
		t->head = TestForImpact( r, o);
		o = o->next;
		// printf("[%d]---***[j:%d]***\n",rank,++j);
	}
	t->tail = NULL;
	struct Impact * ie = insert(h);
	
	freeH(h);
	
	//printf("X %d FREED.", X);

	return ie;	
}

struct Impact * FirstImpact2(os, r)
struct Poly * os;
struct Ray * r;
{  
	//printf("FirstImpact2\n");
	if(os==NULL)
		return NULL;
	struct Poly * o = os;
	struct Impact* currentImpact = (struct Impact *)malloc(sizeof(struct Impact));
	struct Impact* newImpact = (struct Impact *)malloc(sizeof(struct Impact));
	//printf("FirstImpact2\n");
	
	TestForImpact2( r, o, currentImpact);
	o = o->next;
	
	//printf("Current Impact: %f, %d\n", currentImpact->r, currentImpact->i);

	while(o != NULL)
	{  
		TestForImpact2( r, o, newImpact);
	
		//printf("New Impact: %f, %d\n", newImpact->r, newImpact->i);
		//printf("Current Impact: %f, %d\n", currentImpact->r, currentImpact->i);
	
		earlier2(currentImpact, newImpact);
		
		//printf("Current Impact: %f, %d\n", currentImpact->r, currentImpact->i);
		
		o = o->next;
	}

	return currentImpact;	
}

struct Impacts * FindImpacts(rays,objects)
struct Ray * rays;
struct Poly * objects;
{
   if(rays==NULL)
	return NULL;
   
   struct Impacts * result = NULL;
   struct Impacts * t = result;
   //printf("[%d]***[i:%d]***\n",rank,++i);
   while(rays!=NULL)
   {  
	  // printf("[%d]1***[i:%d]***\n",rank,++i);
	   if(result == NULL){
		   //printf("[%d]3***[i:%d]***\n",rank,++i);
		   result = (struct Impacts *)malloc(sizeof(struct Impacts));
		   result->head = FirstImpact(objects,rays);
		   t = result;
		   rays = rays->next;
	   }else{
		   //printf("[%d]2***[i:%d]***\n",rank,++i);
			t->tail=(struct Impacts *)malloc(sizeof(struct Impacts));
			t=t->tail;
			//printf("[%d]2***[i:%d]***\n",rank,++i);
			t->head=FirstImpact(objects,rays);
			//printf("[%d]2***[i:%d]***\n",rank,++i);
			rays=rays->next;
		}
		//printf("[%d]***[i:%d]***\n",rank,++i);
   }
   t->tail=NULL;
   
   return result;
   //printf("[%d]***[i:%d]***FINITO\n",rank,++i);
}

double root(x,r)
double x,r;
{  if(x<0.00000001 && -0.00000001<x)
    return 0.0;
   while(fabs((r*r-x)/x)>=0.0000001)
    r=(r+x/r)/2.0;
   return r;
}

struct Vect * VAdd(v1,v2)
struct Vect * v1, * v2;
{  struct Vect * v;
   v=(struct Vect *)malloc(sizeof(struct Vect));
   v->A=v1->A+v2->A;
   v->B=v1->B+v2->B;
   v->C=v1->C+v2->C;
   return v;
}

struct Vect * VMult(n,v1)
double n;
struct Vect * v1;
{
	struct Vect * v;
	//printf("v1: %x, v1->A: %f\n", v1, v1->A);
	//printf("v1->A: %f, v1->B: %f,v1->C: %f, n: %d\n",v1->A, v1->B, v1->C, n);
   v=(struct Vect *)malloc(sizeof(struct Vect));
   //printf("VM2\n");
   v->A=n*v1->A;//printf("VM3\n");
   v->B=n*v1->B;
   v->C=n*v1->C;   
   
   return v;
}

struct Vect * ray_points(i,j,Detail,v,Vx,Vy)
int i,j,Detail;
struct Vect * v,*Vx,*Vy;
{  
//printf("Detail: %d, i: %d, j: %d\n",Detail, i, j);
   struct Vect * iVx,* jVy,* newv;
   if(j==Detail)
    return NULL;    
   if(i==Detail)
    return ray_points(0,j+1,Detail,v,Vx,Vy);
   //printf("Detail:%d\n",Detail);
    //printf("VM\n");
    //printf("1\n");
   /// printf("Vx: %x, Vx->A: %f\n", Vx, Vx->A);
   iVx=VMult(((double)i)/((double)(Detail-1)),Vx);
   //printf("VM\n");
   //printf("2\n");
   jVy=VMult(((double)j)/((double)(Detail-1)),Vy);
   //printf("VM\n");
   //printf("3\n");
   newv=VAdd(VAdd(v,iVx),jVy);
   //printf("4\n");
   //printf("Detail: %d, i: %d, j: %d\n",Detail, i, j);
   newv->next=ray_points(i+1,j,Detail,v,Vx,Vy); 
   //printf("Detail:%d\n",Detail);  
   return newv;
   
}

struct Ray * GenerateRays(Det,X,Y,Z)
int Det;
double X,Y,Z;
{  
	//printf("[%d]. GenerateRays 1\n", rank);
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
   if(VY->C>0.0)
   {  VX->A=(-VX->A);VX->B=(-VX->B);VX->C=(-VX->C);
      VY->A=(-VY->A);VY->B=(-VY->B);VY->C=(-VY->C);  
   }
   
   v=(struct Vect *)malloc(sizeof(struct Vect));
   v->A=X+Vza-(VX->A+VY->A)/2.0;
   v->B=Y+Vzb-(VX->B+VY->B)/2.0;
   v->C=Z+Vzc-(VX->C+VY->C)/2.0;
   //printf("VX(%.4f,%.4f,%.4f)\n",VX->A,VX->B,VX->C);
   //printf("VY(%.4f,%.4f,%.4f)\n",VY->A,VY->B,VY->C);
   //printf("V(%.4f,%.4f,%.4f)\n",v->A,v->B,v->C);
   rps=ray_points(0,0,Det,v,VX,VY);
   
   //printf("[%d]. GenerateRays 1\n", rank);
   
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
   
   //printf("[%d]. GenerateRays 1\n", rank);
   
   while(rps!=NULL)
   {  
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
   
   //printf("[%d]. GenerateRays 1\n", rank);
   
   return newrays;
}

void showimps(dv,i,imps)
int dv,i;
struct Impacts * imps;
{  
	FILE *fres = fopen("finalResult.txt","w");
		/*for(j=0;j<320;j++){	
			fprintf(fres,"[%d]--%d:, %x ,-:%x\n", rank, j, (imps+j), *(long*)(imps+j));
		}*/
	//printf("----------IMPACTS------------\n");	
	while(imps != NULL)
	{  
		if(i==0)
		{  
			//putchar('\n');
			fprintf(fres,"\n");
			//printf("\n");
			i=dv;
		}
		if(imps->head == NULL){
			//putchar('.');
			fprintf(fres,"%2c",'.');
			//printf("%2c",'.');
		}else{
			fprintf(fres,"%2d",imps->head->i);
			//printf("%2d",imps->head->i);
		}
		//fputchar(' ');
		fprintf(fres," ");
		//printf(" ");
		i=i-1;
		imps=imps->tail;
	}
//putchar('\n');
fprintf(fres,"\n");
//printf("\n");
fclose(fres);
}


struct Ray * initUserData(Detail, ViewX, ViewY, ViewZ)
int Detail;
double ViewX;
double ViewY;
double ViewZ;	
{ 
	struct Ray * rays;	
	
	//rays=GenerateRaysMalloc(Detail,ViewX,ViewY,ViewZ);
	//int length = Detail * Detail;
	//struct Ray * rays;
	///printf("Starting of generating\n");
	rays=GenerateRays(Detail,ViewX,ViewY,ViewZ);
	///
	//struct Impacts * imps;	
	//imps=FindImpacts(rays,Scene);
	//showimps(Detail,Detail,imps);
	
	return rays;
}

struct Poly * getPoly(FILE * fp, int id){  
	struct Poly * p;
    struct Coord * t;
    p=(struct Poly *) malloc(sizeof(struct Poly));
    p->i=id;
    p->N=(struct Vect *)malloc(sizeof(struct Vect));
    int numOfI = -1;
    numOfI = fscanf(fp,"%lf %lf %lf",&(p->N->A),&(p->N->B),&(p->N->C));
    numOfI = fscanf(fp,"%d",&id);
    p->Vs=(struct Coord *)malloc(sizeof(struct Coord));
    t=p->Vs;
    numOfI = fscanf(fp,"%lf %lf %lf",&(t->x),&(t->y),&(t->z));
    t=p->Vs;
    while(--id)
    {  
		t->next=(struct Coord *)malloc(sizeof(struct Coord));
        t=t->next;
        numOfI = fscanf(fp,"%lf %lf %lf",&(t->x),&(t->y),&(t->z));
    }
    t->next=NULL;
    return p;
}

struct Poly * getScene(FILE * fp, int limit){  
	struct Poly * s,* t;
	int id;
	if(fscanf(fp,"%d",&id)==EOF)
	{  
		fclose(fp);
		return NULL;
	}
	s = getPoly(fp,id);
	t=s;
	int i=0;
	while(fscanf(fp,"%d",&id)!=EOF)
	{  
		if(limit <= i++)	   
			break;
		t->next=getPoly(fp,id);
		t=t->next;
	}
	t->next=NULL;
	fclose(fp);
	printf("Read Done: [%d]...\n",--i);
	return s;
}

/*
 * *********************************************************************
 * ******************** End Ray Tracer *********************************
 * *********************************************************************
 * */
 
 

///This function is written by the user countains :
///inputData : the data should be processed by each worker(one task)
///inputLen : the length of data in one task
///result : the data resulting from processing the input data(result of one task)
///outputLen : the length of the result
///process the data(task)
					
void doProcessing(  hwfarm_task_data* t_data, chFM checkForMobility){					
	
	printf("inputLen: %d\n", t_data->input_len);
	//printf("*--*-*-*-*-*-*-*-*-***-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
	//printMappedRays(inputData, inputLen);
	//printf("*--*-*-*-*-*-*-*-*-***-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
	
//	pthread_t pth_id = pthread_self();
	
	//printpolys(Scene);
	//struct Poly* my_scene = copyPolys(Scene);
	//printf("The scene has been copied to %zu\n", pth_id);
	//printpolys(my_scene);
	
	struct Ray * rays = unmapRays(t_data->input_data, t_data->input_len);
		
	//struct Impacts * imps;	
	//imps = FindImpacts(rays, Scene);
	//
	if(rays==NULL)
	return;
	
	//Extract Paramters
	int *main_index = (t_data->counter);
	int details = *((int*)t_data->state_data);
	
	printf("-----------------------------\n[%d]. index: %d, details: %d\n-----------------------------\n", rank, *main_index, details);
   
	struct Impacts * imps = NULL;
	//struct Impacts * tmpResult = NULL;
	struct Impacts * t = imps;
	int i=0;
	
	//Navigate to the unprocessed ray
	while(rays!=NULL && i < *main_index){
		rays=rays->next;
		i++;
	}
	
	int malloc_size = 0;
		
	while(rays!=NULL)
	{     
		if(imps == NULL){
			imps = (struct Impacts *)malloc(sizeof(struct Impacts));
			t = imps;
		}else{
			t->tail=(struct Impacts *)malloc(sizeof(struct Impacts));
			t=t->tail;
		}
		malloc_size += sizeof(struct Impacts);
		malloc_size += sizeof(struct Impacts);
		malloc_size += sizeof(struct Impacts);
		
		t->head = FirstImpact2(Scene, rays);
		
		//printf("**************\n[%d](%zu) main_index: %d\n**************\n", rank, pth_id, *main_index);
		
		//if(t->head != NULL)
		//	printf("[%d]. (%d)[r: %.2f, i: %d]\n", i, rank, t->head->r, t->head->i);			
		//else
		//	printf("[%d]. (%d)[r: -, i: -]\n", i, rank);
		rays=rays->next;

		mapImpactsItem(t_data->output_data, t, *main_index);

		*main_index = (*main_index) + 1;
		
		//
		checkForMobility();
	}
	t->tail=NULL;
	
	printf("[%d]. malloc_size: %d\n", rank, malloc_size);

	//
	//showimps( 20, 20, imps);
	
	//struct MappedImpacts * m_i
	//m_imps[j].r = -1;
	
	//mapImpacts(result, imps, inputLen);
	
}

void* initSharedData(void* shared_data_in, int argc, void ** args){
	return shared_data_in;
}

void* initInputData(void* input_data_in, int argc, void ** args){
	
	struct Ray * rays;	
	
	//printf("MASTER START****************-----: %.6f\n",MPI_Wtime());
	
	int Details = *((int*)args[0]);
	float RAY_X = *((float*)args[1]);
	float RAY_Y = *((float*)args[2]);
	float RAY_Z = *((float*)args[3]);
	
	//printf("MASTER START****************-----: %.6f\n",MPI_Wtime());
	
	rays = initUserData(Details, RAY_X, RAY_Y, RAY_Z);
	
	//printf("MASTER START****************-----: %.6f\n",MPI_Wtime());
	
	//printrays(rays);

	struct MappedRays * mapped_rays;
	mapped_rays = mapRays(rays, Details*Details);
	
	//printf("MASTER START****************-----: %.6f\n",MPI_Wtime());
	
	//printMappedRays(mapped_rays, Details*Details);
	
	return mapped_rays;
}

void* initParData(void* par_data_in, int argc, void ** args){
	int details = *((int*)args[0]);
	
	int* par_data = (int*)malloc(sizeof(double)*(2));
	
	par_data[0] = 0;
	par_data[1] = details;
	//par_data[2] = mat_size;

	return par_data;
}



void* initOutputData(int argc, void ** args){
	int res_size = *((int*)args[4]);
	int all_output_len = *((int*)args[6]);
	
	return malloc(res_size * all_output_len);
}

//*********************************************************
//---------------------------------------------------------
//mpirun -n 2 ./ray4 test11 1 2 2 1500
//mpirun -n 
//	<NumOfProcesses> 
//	<code_file>  
//	<NumberOfRays>
//	<NumberOfRays(in the scene)>(number of task can be specified using previous two arguments)
//	<input_data_file(Scene)>
//	<numberOfObjects(in the scene)>
//  <mobility-on-off>
//---------------------------------------------------------
//mpirun -n 4 -hostfile ~/mpd.hosts ./ray_tracer 4 150 1500 test11 100000 1
//--host bwlf01,bwlf02,bwlf03
//*********************************************************
int main(argc,argv)
int argc;
char **argv;
{ 	
	if(argc!=6)
	{  
		printf("mpirun ... <binary> <rays-1D> <rays-chunk> <object-file> <num-of-object> <mobility>\n");
		exit(0);
	} 
	initHWFarm(argc,argv); 
	
	printf("Worker/Master [%d] In Machine [%s]\n",rank,processor_name);		
	
	startTime = MPI_Wtime();
	
	void * input_data = NULL;	
	void * output_data = NULL;	
	
	//initial arguments
	//void ** args = (void**)malloc(sizeof(int*) * 2);	
	//args[0] = (void*)malloc(sizeof(int));
	//args[1] = (void*)malloc(sizeof(int));
	//*((int*)args[0]) = mat_size;
	//*((int*)args[1]) = chunk;	
	int Details = atoi(argv[1]);
	int chunk_size = atoi(argv[2]);
	char* scene_file = (argv[3]);
	int scene_limit = atoi(argv[4]);
	int mobility = atoi(argv[5]);
	
		
	//prepare data to bes sent to the workers	
	
		
	//Ray Tracer
	//struct Impacts * imps;		
	//int Details = 40;
	//int chunk_size = 400;
	int problem_size = Details * Details;
	
	int tasks = problem_size / chunk_size;
	float RAY_X = 10.0;
	float RAY_Y = 10.0;
	float RAY_Z = 10.0;
	
	//input
	int taskDataSize = sizeof(struct MappedRays);
	int inputDataSize = chunk_size;
	
	//Shared data	
	//shared_data = (double*)initSharedData(NULL, 6, args);	
	
	//output
	int resultDataSize = sizeof(struct MappedImpacts);
	int outputDataSize = chunk_size;
	
	hwfarm_state main_state;
	main_state.counter = 0;
	main_state.max_counter = chunk_size;
	main_state.state_data = NULL;
	main_state.state_len = 0;	
	//
	if(rank == 0)
	{
		//struct MappedRays * mapped_rays;
		//mapped_rays = mapRays(rays, points);
		
		
		//printMappedRays(mapped_rays, points);
		
		//struct Ray * rays2 = unmapRays(mapped_rays, points);
		
		//struct Impacts * imps;	
		//imps = FindImpacts(rays2, Scene);
		//showimps(Details,Details,imps);
		
		//struct MappedImpacts *  m_imp = mapImpacts(imps, points);
		
		//struct Impacts * new_imps = unmapImpacts(m_imp, points);	
		
		//showimps(Details,Details,new_imps);
		
		//printf("MASTER START****************: %.6f\n",MPI_Wtime());
		
		void ** args = (void**)malloc(sizeof(int*) * 7);	
		args[0] = (void*)malloc(sizeof(int));
		args[1] = (void*)malloc(sizeof(float));
		args[2] = (void*)malloc(sizeof(float));
		args[3] = (void*)malloc(sizeof(float));
		args[4] = (void*)malloc(sizeof(int));
		args[5] = (void*)malloc(sizeof(int));
		args[6] = (void*)malloc(sizeof(int));

		//printf("MASTER START****************: %.6f\n",MPI_Wtime());

		//Details
		*((int*)args[0]) = Details;
		//RAY_X
		*((float*)args[1]) = RAY_X;
		//RAY_Y
		*((float*)args[2]) = RAY_Y;
		//RAY_Z
		*((float*)args[3]) = RAY_Z;
		//resultDataSize
		*((int*)args[4]) = resultDataSize;
		//outputDataSize
		*((int*)args[5]) = outputDataSize;
		//problmSize
		*((int*)args[6]) = problem_size;

//printf("MASTER START****************: %.6f\n",MPI_Wtime());
		//Input data		
		input_data = initInputData(input_data, 6, args);	
		
		//printf("MASTER START****************: %.6f\n",MPI_Wtime());


		//State data
		main_state.counter = 0;
		main_state.max_counter = chunk_size;
		main_state.state_data = (int*)initParData(main_state.state_data, 6, args);;
		main_state.state_len = 7 * sizeof(int);
		
		//Output Data		
		output_data = initOutputData(7, args);		
	
		start_time = MPI_Wtime();
		
	}else{
		FILE * fp1;
		fp1=fopen(scene_file,"r");
		if(fp1==NULL)
		{  
			printf("can't open %s\n",scene_file);
			exit(0);
		} 
		
		Scene = getScene(fp1, scene_limit); 
	}
	
	hwfarm( doProcessing, tasks,
		    input_data, taskDataSize, inputDataSize, 
		    NULL, 0, 0, 
		    output_data, resultDataSize, outputDataSize, 
		    main_state, mobility);
	
	
	if(rank == 0)
	{
		//struct MappedImpacts * mapped_imps = (struct MappedImpacts *)output_data;
		//printf("r[0]: %f, i[0]: %d\n", mapped_imps[0].r, mapped_imps[0].i);
		
		struct Impacts * new_imps = unmapImpacts(output_data, problem_size);	
		
		printf("MASTER IMPS.\n");
		showimps(Details, Details, new_imps);
		
		//printArrays(input_data, shared_data, output_data, mat_size);
	}
	
	//printf("[%d] Process finalized\n",rank);
	
	finalizeHWFarm();
	
	return 1;  
}
