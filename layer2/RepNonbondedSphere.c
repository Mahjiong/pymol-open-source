/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#include<GL/gl.h>
#include"Base.h"
#include"OOMac.h"
#include"RepNonbondedSphere.h"
#include"Color.h"
#include"Sphere.h"
#include"Setting.h"
#include"main.h"

typedef struct RepNonbondedSphere {
  Rep R;
  float *V;
  float *VC;
  SphereRec *SP;
  int *NT;
  int N,NC;
} RepNonbondedSphere;

#include"ObjectMolecule.h"

void RepNonbondedSphereRender(RepNonbondedSphere *I,CRay *ray,Pickable **pick);
void RepNonbondedSphereFree(RepNonbondedSphere *I);

void RepNonbondedSphereInit(void)
{
}

void RepNonbondedSphereFree(RepNonbondedSphere *I)
{
  FreeP(I->VC);
  FreeP(I->V);
  FreeP(I->NT);
  OOFreeP(I);
}

void RepNonbondedSphereRender(RepNonbondedSphere *I,CRay *ray,Pickable **pick)
{
  float *v=I->V;
  int c=I->N;
  int cc=0;
  int a;
  SphereRec *sp;

  if(ray) {
	 v=I->VC;
	 c=I->NC;
	 while(c--) {
		ray->fColor3fv(ray,v);
		v+=3;
		ray->fSphere3fv(ray,v,*(v+3));
		v+=4;
	 }
  } else if(pick&&PMGUI) {
  } else if(PMGUI) {
    sp=I->SP;
    while(c--)
      {
        glColor3fv(v);
        v+=3;
        for(a=0;a<sp->NStrip;a++) {
          glBegin(GL_TRIANGLE_STRIP);
          cc=sp->StripLen[a];
          while(cc--) {
            glNormal3fv(v);
            v+=3;
            glVertex3fv(v);
            v+=3;
          }
          glEnd();
        }
      }
  }
}

Rep *RepNonbondedSphereNew(CoordSet *cs)
{
  ObjectMolecule *obj;
  int a,c,d,a1,c1;
  float *v,*v0,*vc,vdw;
  float nonbonded_size;
  int *q, *s;
  SphereRec *sp = Sphere0; 
  int ds,*nt;
  int *active,visFlag;
  AtomInfoType *ai;
  OOAlloc(RepNonbondedSphere);
  obj = cs->Obj;

  active = Alloc(int,cs->NIndex);

  for(a=0;a<cs->NIndex;a++) {
    ai = obj->AtomInfo+cs->IdxToAtm[a];
    active[a] =(!ai->bonded) && (ai->visRep[ cRepNonbondedSphere ]);
  }

  visFlag=false;
  for(a=0;a<cs->NIndex;a++)
    if(active[a]) {
      visFlag=true;
      break;
    }
  if(!visFlag) {
    OOFreeP(I);
    FreeP(active);
    return(NULL); /* skip if no dots are visible */
  }

  nonbonded_size = SettingGet(cSetting_nonbonded_size);

  RepInit(&I->R);
 /* get current dot sampling */
  ds = (int)SettingGet(cSetting_dot_density);
  ds=1;
  if(ds<0) ds=0;
  switch(ds) {
  case 0: sp=Sphere0; break;
  case 1: sp=Sphere1; break;
  case 2: sp=Sphere2; break;
  default: sp=Sphere3; break;
  }

  I->R.fRender=(void (*)(struct Rep *, CRay *, Pickable **))RepNonbondedSphereRender;
  I->R.fFree=(void (*)(struct Rep *))RepNonbondedSphereFree;
  I->R.fRecolor=NULL;

  /* raytracing primitives */
  
  I->VC=(float*)mmalloc(sizeof(float)*cs->NIndex*7);
  ErrChkPtr(I->VC);
  I->NC=0;

  I->NT=NULL;

  v=I->VC; 
  for(a=0;a<cs->NIndex;a++)
	 {
      if(active[a])
		  {
          a1 = cs->IdxToAtm[a];

			 I->NC++;
			 c1=*(cs->Color+a);
			 vc = ColorGet(c1); /* save new color */
			 *(v++)=*(vc++);
			 *(v++)=*(vc++);
			 *(v++)=*(vc++);
			 v0 = cs->Coord+3*a;			 
			 *(v++)=*(v0++);
			 *(v++)=*(v0++);
			 *(v++)=*(v0++);
			 *(v++)=nonbonded_size;
		  }
	 }

  if(I->NC) 
	 I->VC=(float*)mrealloc(I->VC,sizeof(float)*(v-I->VC));
  else
	 I->VC=(float*)mrealloc(I->VC,1);

  I->V=(float*)mmalloc(sizeof(float)*cs->NIndex*(3+sp->NVertTot*6));
  ErrChkPtr(I->V);
  
  /* rendering primitives */

  I->N=0;
  I->SP=sp;
  v=I->V;
  nt=I->NT;

  for(a=0;a<cs->NIndex;a++)
	 {
		if(active[a])
		  {
          a1 = cs->IdxToAtm[a];

			 c1=*(cs->Color+a);
			 v0 = cs->Coord+3*a;
			 vdw = cs->Obj->AtomInfo[a1].vdw;
			 vc = ColorGet(c1);
			 *(v++)=*(vc++);
			 *(v++)=*(vc++);
			 *(v++)=*(vc++);

          q=sp->Sequence;
          s=sp->StripLen;
          
          for(d=0;d<sp->NStrip;d++)
            {
              for(c=0;c<(*s);c++)
                {
                  *(v++)=sp->dot[*q].v[0]; /* normal */
                  *(v++)=sp->dot[*q].v[1];
                  *(v++)=sp->dot[*q].v[2];
                  *(v++)=v0[0]+nonbonded_size*sp->dot[*q].v[0]; /* point */
                  *(v++)=v0[1]+nonbonded_size*sp->dot[*q].v[1];
                  *(v++)=v0[2]+nonbonded_size*sp->dot[*q].v[2];
                  q++;
                }
              s++;
            }
			 I->N++;
			 if(nt) nt++;
		  }
	 }
  
  if(I->N) 
	 {
		I->V=(float*)mrealloc(I->V,sizeof(float)*(v-I->V));
		if(I->NT) I->NT=Realloc(I->NT,int,nt-I->NT);
	 }
  else
	 {
		I->V=(float*)mrealloc(I->V,1);
		if(I->NT) I->NT=Realloc(I->NT,int,1);
	 }
  FreeP(active);
  return((void*)(struct Rep*)I);
}


