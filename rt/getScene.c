#include <stdio.h>
#include <stdlib.h>
#include <math.h>
 
struct Poly * Scene;

struct Coord {double x,y,z;
              struct Coord * next;};

void printcoord(c)
struct Coord * c;
{  printf("Coord %.2lf %.2lf %.2lf\n",c->x,c->y,c->z);  }

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
{  printf("Vect %.2lf %.2lf %.2lf\n",v->A,v->B,v->C);  }

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
	
	//struct MappedImpacts * m_imps = (struct MappedImpacts *)malloc(impsCount * sizeof(struct MappedImpacts));
	
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

struct Poly * getSceneConsecutive(FILE * fp, int limit){  
	struct Poly * scene = (struct Poly *)malloc(sizeof(struct Poly) * limit);
	int i=0;
	for(i=0;i<limit;i++){
		
	}
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

int main(argc,argv)
int argc;
char **argv;
{ 	
	int scene_limit = 10;
	
	FILE * fp1;
	fp1=fopen("object_file","r");
	if(fp1==NULL)
	{  
		printf("can't open 'object_file'\n");
		exit(0);
	} 
	
	struct Poly * Scene = getScene(fp1, scene_limit); 
	
	int i =0;
	
	printpolys(Scene);
	
	return 1;  
}
