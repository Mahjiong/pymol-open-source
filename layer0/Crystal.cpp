

/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* Copyright (c) Schrodinger, LLC. 
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
#include"os_python.h"
#include"os_predef.h"
#include"os_std.h"
#include"os_gl.h"
#include"MemoryDebug.h"
#include"CGO.h"
#include"Err.h"
#include"Base.h"
#include"OOMac.h"
#include"Crystal.h"
#include"Feedback.h"
#include"Util.h"
#include"PConv.h"

PyObject *CrystalAsPyList(CCrystal * I)
{
  PyObject *result = NULL;

  if(I) {
    result = PyList_New(2);
    PyList_SetItem(result, 0, PConvFloatArrayToPyList(I->Dim, 3));
    PyList_SetItem(result, 1, PConvFloatArrayToPyList(I->Angle, 3));
  }
  return (PConvAutoNone(result));
}

int CrystalFromPyList(CCrystal * I, PyObject * list)
{
  int ok = true, rok = true;
  int ll = 0;
  if(ok)
    ok = (I != NULL);
  if(ok)
    ok = PyList_Check(list);
  if(ok)
    ll = PyList_Size(list);
  rok = ok;
  if(ok && (ll > 0))
    ok = PConvPyListToFloatArrayInPlace(PyList_GetItem(list, 0), I->Dim, 3);
  if(ok && (ll > 1))
    ok = PConvPyListToFloatArrayInPlace(PyList_GetItem(list, 1), I->Angle, 3);
  if(ok)
    CrystalUpdate(I);

  /* TO SUPPORT BACKWARDS COMPATIBILITY...
     Always check ll when adding new PyList_GetItem's */

  return (rok);
}

CCrystal::CCrystal(PyMOLGlobals* GParam) : G(GParam)
{
  identity33f(RealToFrac);
  identity33f(FracToReal);

}


void CrystalUpdate(CCrystal * I)
{
  float cabg[3]; /* Cosine of axis angle */
  float sabg[3]; /* Singe of axis angle */
  float cabgs[3];
  float sabgs1;
  int i;

  /* if we just cleared out the memory, but didn't init
   * then init the crystal and return */
  if (!I->Dim[0] || !I->Dim[1] || !I->Dim[2]) {
    *I = CCrystal(I->G);
    return;
  }

  if (!I->Angle[0] || !I->Angle[1] || !I->Angle[2]) {
    I->Angle[0] = I->Angle[1] = I->Angle[2] = 90.0F;
  }

  for(i = 0; i < 9; i++) {
    I->RealToFrac[i] = 0.0;
    I->FracToReal[i] = 0.0;
  }

  for(i = 0; i < 3; i++) {
    cabg[i] = (float) cos(I->Angle[i] * PI / 180.0);
    sabg[i] = (float) sin(I->Angle[i] * PI / 180.0);
  }

  cabgs[0] = (cabg[1] * cabg[2] - cabg[0]) / (sabg[1] * sabg[2]);
  cabgs[1] = (cabg[2] * cabg[0] - cabg[1]) / (sabg[2] * sabg[0]);
  cabgs[2] = (cabg[0] * cabg[1] - cabg[2]) / (sabg[0] * sabg[1]);

  I->UnitCellVolume = (float) (I->Dim[0] * I->Dim[1] * I->Dim[2] *
                               sqrt1d(1.0 + (double) 2.0 * cabg[0] * cabg[1] * cabg[2] -
                                      (double) (cabg[0] * cabg[0] +
                                                (double) cabg[1] * cabg[1] +
                                                (double) cabg[2] * cabg[2])));

  I->RecipDim[0] = I->Dim[1] * I->Dim[2] * sabg[0] / I->UnitCellVolume;
  I->RecipDim[1] = I->Dim[0] * I->Dim[2] * sabg[1] / I->UnitCellVolume;
  I->RecipDim[2] = I->Dim[0] * I->Dim[1] * sabg[2] / I->UnitCellVolume;

  sabgs1 = (float) sqrt1d(1.0 - cabgs[0] * cabgs[0]);

  I->RealToFrac[0] = 1.0F / I->Dim[0];
  I->RealToFrac[1] = -cabg[2] / (sabg[2] * I->Dim[0]);
  I->RealToFrac[2] = -(cabg[2] * sabg[1] * cabgs[0] + cabg[1] * sabg[2]) /
    (sabg[1] * sabgs1 * sabg[2] * I->Dim[0]);
  I->RealToFrac[4] = 1.0F / (sabg[2] * I->Dim[1]);
  I->RealToFrac[5] = cabgs[0] / (sabgs1 * sabg[2] * I->Dim[1]);
  I->RealToFrac[8] = 1.0F / (sabg[1] * sabgs1 * I->Dim[2]);

  I->FracToReal[0] = I->Dim[0];
  I->FracToReal[1] = cabg[2] * I->Dim[1];
  I->FracToReal[2] = cabg[1] * I->Dim[2];
  I->FracToReal[4] = sabg[2] * I->Dim[1];
  I->FracToReal[5] = -sabg[1] * cabgs[0] * I->Dim[2];
  I->FracToReal[8] = sabg[1] * sabgs1 * I->Dim[2];

  I->Norm[0] = (float) sqrt1d(I->RealToFrac[0] * I->RealToFrac[0] +
                              I->RealToFrac[1] * I->RealToFrac[1] +
                              I->RealToFrac[2] * I->RealToFrac[2]);
  I->Norm[1] = (float) sqrt1d(I->RealToFrac[3] * I->RealToFrac[3] +
                              I->RealToFrac[4] * I->RealToFrac[4] +
                              I->RealToFrac[5] * I->RealToFrac[5]);
  I->Norm[2] = (float) sqrt1d(I->RealToFrac[6] * I->RealToFrac[6] +
                              I->RealToFrac[7] * I->RealToFrac[7] +
                              I->RealToFrac[8] * I->RealToFrac[8]);

}

void CrystalDump(CCrystal * I)
{
  PyMOLGlobals *G = I->G;
  int i;

  PRINTF
    " Crystal: Unit Cell         %8.3f %8.3f %8.3f\n", I->Dim[0], I->Dim[1], I->Dim[2]
    ENDF(G);
  PRINTF
    " Crystal: Alpha Beta Gamma  %8.3f %8.3f %8.3f\n",
    I->Angle[0], I->Angle[1], I->Angle[2]
    ENDF(G);
  PRINTF " Crystal: RealToFrac Matrix\n" ENDF(G);
  for(i = 0; i < 3; i++) {
    PRINTF " Crystal: %9.4f %9.4f %9.4f\n",
      I->RealToFrac[i * 3], I->RealToFrac[i * 3 + 1], I->RealToFrac[i * 3 + 2]
      ENDF(G);
  }
  PRINTF " Crystal: FracToReal Matrix\n" ENDF(G);
  for(i = 0; i < 3; i++) {
    PRINTF
      " Crystal: %9.4f %9.4f %9.4f\n",
      I->FracToReal[i * 3], I->FracToReal[i * 3 + 1], I->FracToReal[i * 3 + 2]
      ENDF(G);
  }
  PRINTF " Crystal: Unit Cell Volume %8.0f.\n", I->UnitCellVolume ENDF(G);

}

static float unitCellVertices[][3] = { {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}, // bottom 4 vertices
				       {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} }; // top 4 vertices
static int unitCellLineIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0,   // bottom 4 lines
				     4, 5, 5, 6, 6, 7, 7, 4,   // top 4 lines
				     0, 4, 1, 5, 2, 6, 3, 7 }; // 4 connector lines

CGO *CrystalGetUnitCellCGO(CCrystal * I)
{
  PyMOLGlobals *G = I->G;
  float v[3];
  CGO *cgo = NULL;
  if(I) {
    cgo = CGONew(G);
    CGODisable(cgo, GL_LIGHTING);

    float *vertexVals = CGODrawArrays(cgo, GL_LINES, CGO_VERTEX_ARRAY, 24);
    for (int pl = 0; pl < 24 ; pl++){
      transform33f3f(I->FracToReal, unitCellVertices[unitCellLineIndices[pl]], v);
      copy3f(v, &vertexVals[pl*3]);
    }

    CGOEnable(cgo, GL_LIGHTING);
    CGOStop(cgo);
  }
  return (cgo);
}
