// @(#)root/matrix:$Name:  $:$Id: TMatrixTSym.cxx,v 1.3 2005/12/23 19:55:50 brun Exp $
// Authors: Fons Rademakers, Eddy Offermann  Nov 2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMatrixTSym                                                          //
//                                                                      //
// Template class of a symmetric matrix in the linear algebra package   //
//                                                                      //
// Note that in this implementation both matrix element m[i][j] and     //
// m[j][i] are updated and stored in memory . However, when making the  //
// object persistent only the upper right triangle is stored .          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TMatrixTSym.h"
#include "TMatrixTLazy.h"
#include "TMatrixTSymCramerInv.h"
#include "TDecompLU.h"
#include "TDecompBK.h"
#include "TMatrixDSymEigen.h"

#ifndef R__ALPHA
templateClassImp(TMatrixTSym)
#endif

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(Int_t no_rows)
{
  Allocate(no_rows,no_rows,0,0,1);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(Int_t row_lwb,Int_t row_upb)
{
  const Int_t no_rows = row_upb-row_lwb+1;
  Allocate(no_rows,no_rows,row_lwb,row_lwb,1);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(Int_t no_rows,const Element *elements,Option_t *option)
{
  // option="F": array elements contains the matrix stored column-wise
  //             like in Fortran, so a[i,j] = elements[i+no_rows*j],
  // else        it is supposed that array elements are stored row-wise
  //             a[i,j] = elements[i*no_cols+j]
  //
  // array elements are copied

  Allocate(no_rows,no_rows);
  SetMatrixArray(elements,option);
  if (!this->IsSymmetric()) {
    Error("TMatrixTSym(Int_t,Element*,Option_t*)","matrix not symmetric");
    this->Invalidate();
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(Int_t row_lwb,Int_t row_upb,const Element *elements,Option_t *option)
{
  // array elements are copied

  const Int_t no_rows = row_upb-row_lwb+1;
  Allocate(no_rows,no_rows,row_lwb,row_lwb);
  SetMatrixArray(elements,option);
  if (!this->IsSymmetric()) {
    Error("TMatrixTSym(Int_t,Int_t,Element*,Option_t*)","matrix not symmetric");
    this->Invalidate();
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(const TMatrixTSym<Element> &another) : TMatrixTBase<Element>(another)
{
  Assert(another.IsValid());
  Allocate(another.GetNrows(),another.GetNcols(),another.GetRowLwb(),another.GetColLwb());
  *this = another;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(EMatrixCreatorsOp1 op,const TMatrixTSym<Element> &prototype)
{
  // Create a matrix applying a specific operation to the prototype.
  // Example: TMatrixTSym<Element> a(10,12); ...; TMatrixTSym<Element> b(TMatrixT::kTransposed, a);
  // Supported operations are: kZero, kUnit, kTransposed, kInverted and kAtA.

  Assert(this != &prototype);
  this->Invalidate();
                   
  Assert(prototype.IsValid());
  
  switch(op) {
    case kZero:
      Allocate(prototype.GetNrows(),prototype.GetNcols(),
               prototype.GetRowLwb(),prototype.GetColLwb(),1);
      break;

    case kUnit:
      Allocate(prototype.GetNrows(),prototype.GetNcols(),
               prototype.GetRowLwb(),prototype.GetColLwb(),1);
      this->UnitMatrix();
      break;

    case kTransposed:
      Allocate(prototype.GetNcols(), prototype.GetNrows(),
               prototype.GetColLwb(),prototype.GetRowLwb());
      Transpose(prototype);
      break;

    case kInverted:
    { 
      Allocate(prototype.GetNrows(),prototype.GetNcols(),
               prototype.GetRowLwb(),prototype.GetColLwb(),1);
      *this = prototype;
      // Since the user can not control the tolerance of this newly created matrix
      // we put it to the smallest possible number 
      const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
      this->Invert();
      this->SetTol(oldTol);
      break;
    }

    case kAtA:
      AtMultA(prototype);
      break;

    default:
      Error("TMatrixTSym(EMatrixCreatorOp1,const TMatrixTSym)",
             "operation %d not yet implemented", op);
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(EMatrixCreatorsOp1 op,const TMatrixT<Element> &prototype)
{
  Assert(dynamic_cast<TMatrixT<Element> *>(this) != &prototype);
  this->Invalidate();
                   
  Assert(prototype.IsValid());

  switch(op) {
    case kAtA:
      AtMultA(prototype);
      break;

    default:
      Error("TMatrixTSym(EMatrixCreatorOp1,const TMatrixT)",
             "operation %d not yet implemented", op);
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(const TMatrixTSym<Element> &a,EMatrixCreatorsOp2 op,const TMatrixTSym<Element> &b)
{     
  this->Invalidate();
    
  Assert(a.IsValid());
  Assert(b.IsValid());
    
  switch(op) {
    case kPlus:
    {
      Allocate(a.GetNrows(),a.GetRowLwb(),1);
      *this = a;
      *this += b;
      break;
    }

    case kMinus:
    {
      Allocate(a.GetNrows(),a.GetRowLwb(),1);
      *this = a;
      *this -= b;
      break;
    }

    default:
      Error("TMatrixTSym(EMatrixCreatorOp2)", "operation %d not yet implemented", op);
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element>::TMatrixTSym(const TMatrixTSymLazy<Element> &lazy_constructor)
{
  Allocate(lazy_constructor.GetRowUpb()-lazy_constructor.GetRowLwb()+1,
           lazy_constructor.GetRowUpb()-lazy_constructor.GetRowLwb()+1,
           lazy_constructor.GetRowLwb(),lazy_constructor.GetRowLwb(),1);
  lazy_constructor.FillIn(*this);
  if (!this->IsSymmetric()) {
    Error("TMatrixTSym(TMatrixTSymLazy)","matrix not symmetric");
    this->Invalidate();
  }
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::Delete_m(Int_t size,Element *&m)
{ 
  // delete data pointer m, if it was assigned on the heap

  if (m) {
    if (size > this->kSizeMax)
      delete [] m;
    m = 0;
  }       
}

//______________________________________________________________________________
template<class Element> 
Element* TMatrixTSym<Element>::New_m(Int_t size)
{
  // return data pointer . if requested size <= kSizeMax, assign pointer
  // to the stack space

  if (size == 0) return 0;
  else {
    if ( size <= this->kSizeMax )
      return fDataStack;
    else {
      Element *heap = new Element[size];
      return heap;
    }
  }
}

//______________________________________________________________________________
template<class Element> 
Int_t TMatrixTSym<Element>::Memcpy_m(Element *newp,const Element *oldp,Int_t copySize,
                                     Int_t newSize,Int_t oldSize)
{
  // copy copySize doubles from *oldp to *newp . However take care of the
  // situation where both pointers are assigned to the same stack space

  if (copySize == 0 || oldp == newp)
    return 0;
  else {
    if ( newSize <= this->kSizeMax && oldSize <= this->kSizeMax ) {
      // both pointers are inside fDataStack, be careful with copy direction !
      if (newp > oldp) {
        for (Int_t i = copySize-1; i >= 0; i--)
          newp[i] = oldp[i];
      } else {
        for (Int_t i = 0; i < copySize; i++)
          newp[i] = oldp[i];
      }
    }
    else
      memcpy(newp,oldp,copySize*sizeof(Element));
  }
  return 0;
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::Allocate(Int_t no_rows,Int_t no_cols,Int_t row_lwb,Int_t col_lwb,
                                    Int_t init,Int_t /*nr_nonzeros*/)
{
  // Allocate new matrix. Arguments are number of rows, columns, row
  // lowerbound (0 default) and column lowerbound (0 default).

  if (no_rows < 0 || no_cols < 0)
  {
    Error("Allocate","no_rows=%d no_cols=%d",no_rows,no_cols);
    this->Invalidate();
    return;
  }

  this->MakeValid();
  this->fNrows   = no_rows;
  this->fNcols   = no_cols;
  this->fRowLwb  = row_lwb;
  this->fColLwb  = col_lwb;
  this->fNelems  = this->fNrows*this->fNcols;
  this->fIsOwner = kTRUE;
  this->fTol     = std::numeric_limits<Element>::epsilon();

  if (this->fNelems > 0) {
    fElements = New_m(this->fNelems);
    if (init)
      memset(fElements,0,this->fNelems*sizeof(Element));
  } else
    fElements = 0;
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::AtMultA(const TMatrixT<Element> &a,Int_t constr)
{
  // Create a matrix C such that C = A' * A. In other words,
  // c[i,j] = SUM{ a[k,i] * a[k,j] }. Note, matrix C is allocated for constr=1.

  Assert(a.IsValid());

  if (constr)
    Allocate(a.GetNcols(),a.GetNcols(),a.GetColLwb(),a.GetColLwb(),1);

#ifdef CBLAS
  const Element *ap = a.GetMatrixArray();
        Element *cp = this->GetMatrixArray();
  if (typeid(Element) == typeid(Double_t))
    cblas_dgemm (CblasRowMajor,CblasTrans,CblasNoTrans,this->fNrows,this->fNcols,a.GetNrows(),
                 1.0,ap,a.GetNcols(),ap,a.GetNcols(),1.0,cp,this->fNcols);
  else if (typeid(Element) != typeid(Float_t))
    cblas_sgemm (CblasRowMajor,CblasTrans,CblasNoTrans,fNrows,fNcols,a.GetNrows(),
                 1.0,ap,a.GetNcols(),ap,a.GetNcols(),1.0,cp,fNcols); 
  else
    Error("AtMultA","type %s not implemented in BLAS library",typeid(Element));
#else
  const Int_t nb     = a.GetNoElements();
  const Int_t ncolsa = a.GetNcols();
  const Int_t ncolsb = ncolsa;
  const Element * const ap = a.GetMatrixArray();
  const Element * const bp = ap;
        Element *       cp = this->GetMatrixArray();

  const Element *acp0 = ap;           // Pointer to  A[i,0];
  while (acp0 < ap+a.GetNcols()) {
    for (const Element *bcp = bp; bcp < bp+ncolsb; ) { // Pointer to the j-th column of A, Start bcp = A[0,0]
      const Element *acp = acp0;                       // Pointer to the i-th column of A, reset to A[0,i]
      Element cij = 0;
      while (bcp < bp+nb) {           // Scan the i-th column of A and
        cij += *acp * *bcp;           // the j-th col of A
        acp += ncolsa;
        bcp += ncolsb;
      }
      *cp++ = cij;
      bcp -= nb-1;                    // Set bcp to the (j+1)-th col
    }
    acp0++;                           // Set acp0 to the (i+1)-th col
  }

  Assert(cp == this->GetMatrixArray()+this->fNelems && acp0 == ap+ncolsa);
#endif
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::AtMultA(const TMatrixTSym<Element> &a,Int_t constr)
{
  // Matrix multiplication, with A symmetric
  // Create a matrix C such that C = A' * A = A * A = A * A'
  // Note, matrix C is allocated for constr=1.

  Assert(a.IsValid());

  if (constr)
    Allocate(a.GetNcols(),a.GetNcols(),a.GetColLwb(),a.GetColLwb(),1);

#ifdef CBLAS
  const Element *ap = a.GetMatrixArray();
        Element *cp = this->GetMatrixArray();
  if (typeid(Element) == typeid(Double_t))
    cblas_dsymm (CblasRowMajor,CblasLeft,CblasUpper,this->fNrows,this->fNcols,1.0,
                 ap,a.GetNcols(),ap,a.GetNcols(),0.0,cp,this->fNcols);
  else if (typeid(Element) != typeid(Float_t))
    cblas_ssymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                 ap1,a.GetNcols(),ap,a.GetNcols(),0.0,cp,fNcols);
  else
    Error("AtMultA","type %s not implemented in BLAS library",typeid(Element));
#else
  const Int_t nb     = a.GetNoElements();
  const Int_t ncolsa = a.GetNcols();
  const Int_t ncolsb = ncolsa;
  const Element * const ap = a.GetMatrixArray();
  const Element * const bp = ap;
        Element *       cp = this->GetMatrixArray();

  const Element *acp0 = ap;           // Pointer to  A[i,0];
  while (acp0 < ap+a.GetNcols()) {
    for (const Element *bcp = bp; bcp < bp+ncolsb; ) { // Pointer to the j-th column of A, Start bcp = A[0,0]
      const Element *acp = acp0;                       // Pointer to the i-th column of A, reset to A[0,i]
      Element cij = 0;
      while (bcp < bp+nb) {           // Scan the i-th column of A and
        cij += *acp * *bcp;           // the j-th col of A
        acp += ncolsa;
        bcp += ncolsb;
      }
      *cp++ = cij;
      bcp -= nb-1;                    // Set bcp to the (j+1)-th col
    }
    acp0++;                           // Set acp0 to the (i+1)-th col
  }

  Assert(cp == this->GetMatrixArray()+this->fNelems && acp0 == ap+ncolsa);
#endif
}

//______________________________________________________________________________ 
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Use(Int_t row_lwb,Int_t row_upb,Element *data)
{
  if (row_upb < row_lwb)
  {
    Error("Use","row_upb=%d < row_lwb=%d",row_upb,row_lwb);
    this->Invalidate();
    return *this;
  }

  this->Clear();
  this->fNrows    = row_upb-row_lwb+1;
  this->fNcols    = this->fNrows;
  this->fRowLwb   = row_lwb;
  this->fColLwb   = row_lwb;
  this->fNelems   = this->fNrows*this->fNcols;
  fElements = data;
  this->fIsOwner  = kFALSE;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::GetSub(Int_t row_lwb,Int_t row_upb,
                                                   TMatrixTSym<Element> &target,Option_t *option) const
{
  // Get submatrix [row_lwb..row_upb][row_lwb..row_upb]; The indexing range of the
  // returned matrix depends on the argument option:
  //
  // option == "S" : return [0..row_upb-row_lwb+1][0..row_upb-row_lwb+1] (default)
  // else          : return [row_lwb..row_upb][row_lwb..row_upb]

  Assert(this->IsValid());

  if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
    Error("GetSub","row_lwb out of bounds");
    target.Invalidate();
    return target;
  }
  if (row_upb < this->fRowLwb || row_upb > this->fRowLwb+this->fNrows-1) {
    Error("GetSub","row_upb out of bounds");
    target.Invalidate();
    return target;
  }
  if (row_upb < row_lwb) {
    Error("GetSub","row_upb < row_lwb");
    target.Invalidate();
    return target;
  }

  TString opt(option);
  opt.ToUpper();
  const Int_t shift = (opt.Contains("S")) ? 1 : 0;

  Int_t row_lwb_sub;
  Int_t row_upb_sub;
  if (shift) {
    row_lwb_sub = 0;
    row_upb_sub = row_upb-row_lwb;
  } else {
    row_lwb_sub = row_lwb;
    row_upb_sub = row_upb;
  }

  target.ResizeTo(row_lwb_sub,row_upb_sub,row_lwb_sub,row_upb_sub);
  const Int_t nrows_sub = row_upb_sub-row_lwb_sub+1;

  if (target.GetRowIndexArray() && target.GetColIndexArray()) {
    for (Int_t irow = 0; irow < nrows_sub; irow++) {
      for (Int_t icol = 0; icol < nrows_sub; icol++) {
        target(irow+row_lwb_sub,icol+row_lwb_sub) = (*this)(row_lwb+irow,row_lwb+icol);
      }
    }
  } else {
    const Element *ap = this->GetMatrixArray()+(row_lwb-this->fRowLwb)*this->fNrows+(row_lwb-this->fRowLwb);
          Element *bp = target.GetMatrixArray();

    for (Int_t irow = 0; irow < nrows_sub; irow++) {
      const Element *ap_sub = ap;
      for (Int_t icol = 0; icol < nrows_sub; icol++) {
        *bp++ = *ap_sub++;
      }
      ap += this->fNrows;
    }
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::GetSub(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb,
                                                    TMatrixTBase<Element> &target,Option_t *option) const
{
  // Get submatrix [row_lwb..row_upb][col_lwb..col_upb]; The indexing range of the
  // returned matrix depends on the argument option:
  //
  // option == "S" : return [0..row_upb-row_lwb+1][0..col_upb-col_lwb+1] (default)
  // else          : return [row_lwb..row_upb][col_lwb..col_upb]

  Assert(this->IsValid());
  if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
    Error("GetSub","row_lwb out of bounds");
    target.Invalidate();
    return target;
  }
  if (col_lwb < this->fColLwb || col_lwb > this->fColLwb+this->fNcols-1) {
    Error("GetSub","col_lwb out of bounds");
    target.Invalidate();
    return target;
  }
  if (row_upb < this->fRowLwb || row_upb > this->fRowLwb+this->fNrows-1) {
    Error("GetSub","row_upb out of bounds");
    target.Invalidate();
    return target;
  }
  if (col_upb < this->fColLwb || col_upb > this->fColLwb+this->fNcols-1) {
    Error("GetSub","col_upb out of bounds");
    target.Invalidate();
    return target;
  }
  if (row_upb < row_lwb || col_upb < col_lwb) {
    Error("GetSub","row_upb < row_lwb || col_upb < col_lwb");
    target.Invalidate();
    return target;
  }

  TString opt(option);
  opt.ToUpper();
  const Int_t shift = (opt.Contains("S")) ? 1 : 0;

  const Int_t row_lwb_sub = (shift) ? 0               : row_lwb;
  const Int_t row_upb_sub = (shift) ? row_upb-row_lwb : row_upb;
  const Int_t col_lwb_sub = (shift) ? 0               : col_lwb;
  const Int_t col_upb_sub = (shift) ? col_upb-col_lwb : col_upb;

  target.ResizeTo(row_lwb_sub,row_upb_sub,col_lwb_sub,col_upb_sub);
  const Int_t nrows_sub = row_upb_sub-row_lwb_sub+1;
  const Int_t ncols_sub = col_upb_sub-col_lwb_sub+1;

  if (target.GetRowIndexArray() && target.GetColIndexArray()) {
    for (Int_t irow = 0; irow < nrows_sub; irow++) {
      for (Int_t icol = 0; icol < ncols_sub; icol++) {
        target(irow+row_lwb_sub,icol+col_lwb_sub) = (*this)(row_lwb+irow,col_lwb+icol);
      }
    }
  } else {
    const Element *ap = this->GetMatrixArray()+(row_lwb-this->fRowLwb)*this->fNcols+(col_lwb-this->fColLwb);
          Element *bp = target.GetMatrixArray();

    for (Int_t irow = 0; irow < nrows_sub; irow++) {
      const Element *ap_sub = ap;
      for (Int_t icol = 0; icol < ncols_sub; icol++) {
        *bp++ = *ap_sub++;
      }
      ap += this->fNcols;
    }
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::SetSub(Int_t row_lwb,const TMatrixTBase<Element> &source)
{ 
  // Insert matrix source starting at [row_lwb][row_lwb], thereby overwriting the part
  // [row_lwb..row_lwb+nrows_source][row_lwb..row_lwb+nrows_source];
    
  Assert(this->IsValid());
  Assert(source.IsValid());
    
  if (!source.IsSymmetric()) {
    Error("SetSub","source matrix is not symmetric");
    this->Invalidate();
    return *this;
  }
  if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
    Error("SetSub","row_lwb outof bounds");
    this->Invalidate();
    return *this;
  }
  const Int_t nRows_source = source.GetNrows();
  if (row_lwb+nRows_source > this->fRowLwb+this->fNrows) {
    Error("SetSub","source matrix too large");
    this->Invalidate();
    return *this;
  }
  
  if (source.GetRowIndexArray() && source.GetColIndexArray()) {
    const Int_t rowlwb_s = source.GetRowLwb();
    for (Int_t irow = 0; irow < nRows_source; irow++) {
      for (Int_t icol = 0; icol < nRows_source; icol++) {
        (*this)(row_lwb+irow,row_lwb+icol) = source(rowlwb_s+irow,rowlwb_s+icol);
      }
    }
  } else {
    const Element *bp = source.GetMatrixArray();
          Element *ap = this->GetMatrixArray()+(row_lwb-this->fRowLwb)*this->fNrows+(row_lwb-this->fRowLwb);
      
    for (Int_t irow = 0; irow < nRows_source; irow++) {
      Element *ap_sub = ap;
      for (Int_t icol = 0; icol < nRows_source; icol++) {
        *ap_sub++ = *bp++;
      }
      ap += this->fNrows;
    }
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::SetSub(Int_t row_lwb,Int_t col_lwb,const TMatrixTBase<Element> &source)
{
  // Insert matrix source starting at [row_lwb][col_lwb] in a symmetric fashion, thereby overwriting the part
  // [row_lwb..row_lwb+nrows_source][row_lwb..row_lwb+nrows_source];

  Assert(this->IsValid());
  Assert(source.IsValid());

  if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
    Error("SetSub","row_lwb out of bounds");
    this->Invalidate();
    return *this;
  }
  if (col_lwb < this->fColLwb || col_lwb > this->fColLwb+this->fNcols-1) {
    Error("SetSub","col_lwb out of bounds");
    this->Invalidate();
    return *this;
  }
  const Int_t nRows_source = source.GetNrows();
  const Int_t nCols_source = source.GetNcols();

  if (row_lwb+nRows_source > this->fRowLwb+this->fNrows || col_lwb+nCols_source > this->fRowLwb+this->fNrows) {
    Error("SetSub","source matrix too large");
    this->Invalidate();
    return *this;
  }
  if (col_lwb+nCols_source > this->fRowLwb+this->fNrows || row_lwb+nRows_source > this->fRowLwb+this->fNrows) {
    Error("SetSub","source matrix too large");
    this->Invalidate();
    return *this;
  }
  
  const Int_t rowlwb_s = source.GetRowLwb();
  const Int_t collwb_s = source.GetColLwb();
  if (row_lwb >= col_lwb) {
    // lower triangle
    Int_t irow;
    for (irow = 0; irow < nRows_source; irow++) {
      for (Int_t icol = 0; col_lwb+icol <= row_lwb+irow &&
                             icol < nCols_source; icol++) {
        (*this)(row_lwb+irow,col_lwb+icol) = source(irow+rowlwb_s,icol+collwb_s);
      }
    }

    // upper triangle
    for (irow = 0; irow < nCols_source; irow++) {
      for (Int_t icol = nRows_source-1; row_lwb+icol > irow+col_lwb &&
                              icol >= 0; icol--) {
        (*this)(col_lwb+irow,row_lwb+icol) = source(icol+rowlwb_s,irow+collwb_s);
      }
    }
  } else {

  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::SetMatrixArray(const Element *data,Option_t *option)
{
  TMatrixTBase<Element>::SetMatrixArray(data,option);
  if (!this->IsSymmetric()) {
    Error("SetMatrixArray","Matrix is not symmetric after Set");
    this->Invalidate(); 
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::Shift(Int_t row_shift,Int_t col_shift)
{
  if (row_shift != col_shift) {
    Error("Shift","row_shift != col_shift");
    this->Invalidate(); 
    return *this;
  }
  return TMatrixTBase<Element>::Shift(row_shift,col_shift);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::ResizeTo(Int_t nrows,Int_t ncols,Int_t /*nr_nonzeros*/)
{
  // Set size of the matrix to nrows x ncols
  // New dynamic elements are created, the overlapping part of the old ones are
  // copied to the new structures, then the old elements are deleted.

  Assert(this->IsValid());
  if (!this->fIsOwner) {
    Error("ResizeTo(Int_t,Int_t)","Not owner of data array,cannot resize");
    this->Invalidate();
    return *this;
  }

  if (nrows != ncols) {
    Error("ResizeTo(Int_t,Int_t)","nrows != ncols");
    this->Invalidate(); 
    return *this;
  }

  if (this->fNelems > 0) {
    if (this->fNrows == nrows && this->fNcols == ncols)
      return *this;
    else if (nrows == 0 || ncols == 0) {
      this->fNrows = nrows; this->fNcols = ncols;
      this->Clear();
      return *this;
    }

    Element     *elements_old = GetMatrixArray();
    const Int_t  nelems_old   = this->fNelems;
    const Int_t  nrows_old    = this->fNrows;
    const Int_t  ncols_old    = this->fNcols;

    Allocate(nrows,ncols);
    Assert(this->IsValid());

    Element *elements_new = GetMatrixArray();
    // new memory should be initialized but be careful ot to wipe out the stack
    // storage. Initialize all when old or new storage was on the heap
    if (this->fNelems > this->kSizeMax || nelems_old > this->kSizeMax)
      memset(elements_new,0,this->fNelems*sizeof(Element));
    else if (this->fNelems > nelems_old)
      memset(elements_new+nelems_old,0,(this->fNelems-nelems_old)*sizeof(Element));

    // Copy overlap
    const Int_t ncols_copy = TMath::Min(this->fNcols,ncols_old); 
    const Int_t nrows_copy = TMath::Min(this->fNrows,nrows_old); 

    const Int_t nelems_new = this->fNelems;
    if (ncols_old < this->fNcols) {
      for (Int_t i = nrows_copy-1; i >= 0; i--)
        Memcpy_m(elements_new+i*this->fNcols,elements_old+i*ncols_old,ncols_copy,
                 nelems_new,nelems_old);
    } else {
      for (Int_t i = 0; i < nrows_copy; i++)
        Memcpy_m(elements_new+i*this->fNcols,elements_old+i*ncols_old,ncols_copy,
                 nelems_new,nelems_old);
    }

    Delete_m(nelems_old,elements_old);
  } else {
    Allocate(nrows,ncols,0,0,1);
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::ResizeTo(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb,
                                                      Int_t /*nr_nonzeros*/)
{
  // Set size of the matrix to [row_lwb:row_upb] x [col_lwb:col_upb]
  // New dynamic elemenst are created, the overlapping part of the old ones are
  // copied to the new structures, then the old elements are deleted.

  Assert(this->IsValid());
  if (!this->fIsOwner) {
    Error("ResizeTo(Int_t,Int_t,Int_t,Int_t)","Not owner of data array,cannot resize");
    this->Invalidate();
    return *this;
  }

  if (row_lwb != col_lwb) {
    Error("ResizeTo(Int_t,Int_t,Int_t,Int_t)","row_lwb != col_lwb");
    this->Invalidate(); 
    return *this;
  }
  if (row_upb != col_upb) {
    Error("ResizeTo(Int_t,Int_t,Int_t,Int_t)","row_upb != col_upb");
    this->Invalidate(); 
    return *this;
  }

  const Int_t new_nrows = row_upb-row_lwb+1;
  const Int_t new_ncols = col_upb-col_lwb+1;

  if (this->fNelems > 0) {

    if (this->fNrows  == new_nrows  && this->fNcols  == new_ncols &&
        this->fRowLwb == row_lwb    && this->fColLwb == col_lwb)
       return *this;
    else if (new_nrows == 0 || new_ncols == 0) {
      this->fNrows = new_nrows; this->fNcols = new_ncols;
      this->fRowLwb = row_lwb; this->fColLwb = col_lwb;
      this->Clear();
      return *this;
    }

    Element    *elements_old = GetMatrixArray();
    const Int_t  nelems_old   = this->fNelems;
    const Int_t  nrows_old    = this->fNrows;
    const Int_t  ncols_old    = this->fNcols;
    const Int_t  rowLwb_old   = this->fRowLwb;
    const Int_t  colLwb_old   = this->fColLwb;

    Allocate(new_nrows,new_ncols,row_lwb,col_lwb);
    Assert(this->IsValid());

    Element *elements_new = GetMatrixArray();
    // new memory should be initialized but be careful ot to wipe out the stack
    // storage. Initialize all when old or new storag ewas on the heap
    if (this->fNelems > this->kSizeMax || nelems_old > this->kSizeMax)
      memset(elements_new,0,this->fNelems*sizeof(Element));
    else if (this->fNelems > nelems_old)
      memset(elements_new+nelems_old,0,(this->fNelems-nelems_old)*sizeof(Element));

    // Copy overlap
    const Int_t rowLwb_copy = TMath::Max(this->fRowLwb,rowLwb_old); 
    const Int_t colLwb_copy = TMath::Max(this->fColLwb,colLwb_old); 
    const Int_t rowUpb_copy = TMath::Min(this->fRowLwb+this->fNrows-1,rowLwb_old+nrows_old-1); 
    const Int_t colUpb_copy = TMath::Min(this->fColLwb+this->fNcols-1,colLwb_old+ncols_old-1); 

    const Int_t nrows_copy = rowUpb_copy-rowLwb_copy+1;
    const Int_t ncols_copy = colUpb_copy-colLwb_copy+1;

    if (nrows_copy > 0 && ncols_copy > 0) {
      const Int_t colOldOff = colLwb_copy-colLwb_old;
      const Int_t colNewOff = colLwb_copy-this->fColLwb;
      if (ncols_old < this->fNcols) {
        for (Int_t i = nrows_copy-1; i >= 0; i--) {
          const Int_t iRowOld = rowLwb_copy+i-rowLwb_old;
          const Int_t iRowNew = rowLwb_copy+i-this->fRowLwb;
          Memcpy_m(elements_new+iRowNew*this->fNcols+colNewOff,
                   elements_old+iRowOld*ncols_old+colOldOff,ncols_copy,this->fNelems,nelems_old);
        }
      } else {
        for (Int_t i = 0; i < nrows_copy; i++) {
          const Int_t iRowOld = rowLwb_copy+i-rowLwb_old;
          const Int_t iRowNew = rowLwb_copy+i-this->fRowLwb;
          Memcpy_m(elements_new+iRowNew*this->fNcols+colNewOff,
                   elements_old+iRowOld*ncols_old+colOldOff,ncols_copy,this->fNelems,nelems_old);
        }
      }
    }

    Delete_m(nelems_old,elements_old);
  } else {
    Allocate(new_nrows,new_ncols,row_lwb,col_lwb,1);
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
Double_t TMatrixTSym<Element>::Determinant() const
{
  const TMatrixT<Element> &tmp = *this;
  TDecompLU lu(tmp,this->fTol);
  Double_t d1,d2;
  lu.Det(d1,d2);
  return d1*TMath::Power(2.0,d2);
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::Determinant(Double_t &d1,Double_t &d2) const
{
  const TMatrixT<Element> &tmp = *this;
  TDecompLU lu(tmp,this->fTol);
  lu.Det(d1,d2);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Invert(Double_t *det)
{     
  // Invert the matrix and calculate its determinant
  // Notice that we need to invoke an additional LU decomposition in order to
  // calculate the determinant beacuse the Bunch-Kaufman does not result in a
  // convenient triagularr matrix .
    
  if (det)
    *det = this->Determinant();
  TMatrixDSym tmp = *this;
  TDecompBK bk(tmp,this->fTol);
  bk.Invert(tmp);
  *this = tmp;
  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::InvertFast(Double_t *det)
{
  // Invert the matrix and calculate its determinant

  Assert(this->IsValid());

  const Char_t nRows = Char_t(this->GetNrows());
  switch (nRows) {
    case 1:
    {
      Element *pM = this->GetMatrixArray();
      if (*pM == 0.) {
        Error("InvertFast","matrix is singular");
        this->Invalidate();
        *det = 0;
      } 
      else {
        *det = *pM;
        *pM = 1.0/(*pM);
      } 
      return *this;
    }
    case 2:
    {
      TMatrixTSymCramerInv::Inv2x2<Element>(*this,det);
      return *this;
    }
    case 3:
    {
      TMatrixTSymCramerInv::Inv3x3<Element>(*this,det);
      return *this;
    }
    case 4:
    {
      TMatrixTSymCramerInv::Inv4x4<Element>(*this,det);
      return *this;
    }
    case 5:
    {
      TMatrixTSymCramerInv::Inv5x5<Element>(*this,det);
      return *this;
    }
    case 6:
    {
      TMatrixTSymCramerInv::Inv6x6<Element>(*this,det);
      return *this;
    }

    default:
    {
      if (det)
        *det = this->Determinant();
      TMatrixDSym tmp = *this;
      TDecompBK bk(tmp,this->fTol);
      bk.Invert(tmp);
      *this = tmp;
      return *this;
    }
  }
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Transpose(const TMatrixTSym<Element> &source)
{
  // Transpose a matrix.

  Assert(this->IsValid());
  Assert(source.IsValid());

  if (this->fNrows != source.GetNcols() || this->fRowLwb != source.GetColLwb())
  {
    Error("Transpose","matrix has wrong shape");
    this->Invalidate();
    return *this;
  }

  *this = source;
  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Rank1Update(const TVectorT<Element> &v,Element alpha)
{
  // Perform a rank 1 operation on the matrix:                          
  //     A += alpha * v * v^T

  Assert(this->IsValid());
  Assert(v.IsValid());

  if (v.GetNoElements() < this->fNrows) {
    Error("Rank1Update","vector too short");
    this->Invalidate();
    return *this;
  }

  const Element * const pv = v.GetMatrixArray();
        Element *trp = this->GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
        Element *tcp = trp;                    // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < this->fNrows; i++) {
    trp += i;               // point to [i,i]
    tcp += i*this->fNcols;  // point to [i,i]
    const Element tmp = alpha*pv[i];
    for (Int_t j = i; j < this->fNcols; j++) {
      if (j > i) *tcp += tmp*pv[j];
      *trp++ += tmp*pv[j];
      tcp += this->fNcols;
    }
    tcp -= this->fNelems-1; // point to [0,i]
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Similarity(const TMatrixT<Element> &b)
{
// Calculate B * (*this) * B^T , final matrix will be (nrowsb x nrowsb)
// This is a similarity transform when B is orthogonal . It is more
// efficient than applying the actual multiplication because this
// routine realizes that  the final matrix is symmetric . 

  const TMatrixT<Element> ba(b,TMatrixT<Element>::kMult,*this);

  const Int_t nrowsb  = b.GetNrows();
  if (nrowsb != this->fNrows)
    this->ResizeTo(nrowsb,nrowsb);

#ifdef CBLAS
  const Element *bap = ba.GetMatrixArray();
  const Element *bp  = b.GetMatrixArray();
        Element *cp  = this->GetMatrixArray();
  if (typeid(Element) == typeid(Double_t))
    cblas_dgemm (CblasRowMajor,CblasNoTrans,CblasTrans,this->fNrows,this->fNcols,ba.GetNcols(),
                 1.0,bap,ba.GetNcols(),bp,b.GetNcols(),1.0,cp,this->fNcols);
  else if (typeid(Element) != typeid(Float_t))
    cblas_sgemm (CblasRowMajor,CblasNoTrans,CblasTrans,fNrows,fNcols,ba.GetNcols(),
                 1.0,bap,ba.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
  else
    Error("Similarity","type %s not implemented in BLAS library",typeid(Element));
#else
  const Int_t nba     = ba.GetNoElements();
  const Int_t nb      = b.GetNoElements();
  const Int_t ncolsba = ba.GetNcols();
  const Int_t ncolsb  = b.GetNcols();
  const Element * const bap  = ba.GetMatrixArray();
  const Element * const bp   = b.GetMatrixArray();
  const Element *       bi1p = bp;
        Element *       cp   = this->GetMatrixArray();
        Element * const cp0  = cp;

  Int_t ishift = 0;
  const Element *barp0 = bap;
  while (barp0 < bap+nba) {
    const Element *brp0 = bi1p;
    while (brp0 < bp+nb) {
      const Element *barp = barp0;
      const Element *brp  = brp0;
      Element cij = 0;
      while (brp < brp0+ncolsb)
        cij += *barp++ * *brp++;
      *cp++ = cij;
      brp0 += ncolsb;
    }
    barp0 += ncolsba;
    bi1p += ncolsb;
    cp += ++ishift;
  }

  Assert(cp == cp0+this->fNelems+ishift && barp0 == bap+nba);

  cp = cp0;
  for (Int_t irow = 0; irow < this->fNrows; irow++) {
    const Int_t rowOff1 = irow*this->fNrows;
    for (Int_t icol = 0; icol < irow; icol++) {
      const Int_t rowOff2 = icol*this->fNrows;
      cp[rowOff1+icol] = cp[rowOff2+irow];
    }
  }
#endif

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::Similarity(const TMatrixTSym<Element> &b)
{
// Calculate B * (*this) * B^T , final matrix will be (nrowsb x nrowsb)
// This is a similarity transform when B is orthogonal . It is more
// efficient than applying the actual multiplication because this
// routine realizes that  the final matrix is symmetric .

#ifdef CBLAS
  const TMatrixT<Element> abt(*this,TMatrixT<Element>::kMultTranspose,b);

  const Element *abtp = abt.GetMatrixArray();
  const Element *bp   = b.GetMatrixArray();
        Element *cp   = this->GetMatrixArray();
  if (typeid(Element) == typeid(Double_t))
    cblas_dsymm (CblasRowMajor,CblasLeft,CblasUpper,this->fNrows,this->fNcols,1.0,
                 bp,b.GetNcols(),abtp,abt.GetNcols(),0.0,cp,this->fNcols);
  else if (typeid(Element) != typeid(Float_t))
    cblas_ssymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                 bp,b.GetNcols(),abtp,abt.GetNcols(),0.0,cp,fNcols);
  else
    Error("Similarity","type %s not implemented in BLAS library",typeid(Element));
#else
  const TMatrixT<Element> ba(b,TMatrixT<Element>::kMult,*this);

  const Int_t nba     = ba.GetNoElements();
  const Int_t nb      = b.GetNoElements();
  const Int_t ncolsba = ba.GetNcols();
  const Int_t ncolsb  = b.GetNcols();
  const Element * const bap  = ba.GetMatrixArray();
  const Element * const bp   = b.GetMatrixArray();
  const Element *       bi1p = bp;
        Element *       cp   = this->GetMatrixArray();
        Element * const cp0  = cp;

  Int_t ishift = 0;
  const Element *barp0 = bap;
  while (barp0 < bap+nba) {
    const Element *brp0 = bi1p;
    while (brp0 < bp+nb) {
      const Element *barp = barp0;
      const Element *brp  = brp0;
      Element cij = 0;
      while (brp < brp0+ncolsb)
        cij += *barp++ * *brp++;
      *cp++ = cij;
      brp0 += ncolsb;
    }
    barp0 += ncolsba;
    bi1p += ncolsb;
    cp += ++ishift;
  }

  Assert(cp == cp0+this->fNelems+ishift && barp0 == bap+nba);

  cp = cp0;
  for (Int_t irow = 0; irow < this->fNrows; irow++) {
    const Int_t rowOff1 = irow*this->fNrows;
    for (Int_t icol = 0; icol < irow; icol++) {
      const Int_t rowOff2 = icol*this->fNrows;
      cp[rowOff1+icol] = cp[rowOff2+irow];
    }
  }
#endif

  return *this;
}

//______________________________________________________________________________
template<class Element> 
Element TMatrixTSym<Element>::Similarity(const TVectorT<Element> &v)
{
// Calculate scalar v * (*this) * v^T

  Assert(this->IsValid());
  Assert(v.IsValid());

  if (this->fNcols != v.GetNrows() || this->fColLwb != v.GetLwb()) {
    Error("Similarity(const TVectorT &)","vector and matrix incompatible");
    this->Invalidate();
    return -1.;
  }

  const Element *mp = this->GetMatrixArray(); // Matrix row ptr
  const Element *vp = v.GetMatrixArray();     // vector ptr

  Element sum1 = 0;
  const Element * const vp_first = vp;
  const Element * const vp_last  = vp+v.GetNrows();
  while (vp < vp_last) {
    Element sum2 = 0;
    for (const Element *sp = vp_first; sp < vp_last; )
      sum2 += *mp++ * *sp++;
    sum1 += sum2 * *vp++;
  }

  Assert(mp == this->GetMatrixArray()+this->GetNoElements());

  return sum1;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::SimilarityT(const TMatrixT<Element> &b)
{
// Calculate B^T * (*this) * B , final matrix will be (ncolsb x ncolsb)
// It is more efficient than applying the actual multiplication because this
// routine realizes that  the final matrix is symmetric .

  const TMatrixT<Element> bta(b,TMatrixT<Element>::kTransposeMult,*this);

  const Int_t ncolsb = b.GetNcols();
  if (ncolsb != this->fNcols)
    this->ResizeTo(ncolsb,ncolsb);

#ifdef CBLAS
  const Element *btap = bta.GetMatrixArray();
  const Element *bp   = b.GetMatrixArray();
        Element *cp   = this->GetMatrixArray();
  if (typeid(Element) == typeid(Double_t))
    cblas_dgemm (CblasRowMajor,CblasNoTrans,CblasNoTrans,this->fNrows,this->fNcols,bta.GetNcols(),
                 1.0,btap,bta.GetNcols(),bp,b.GetNcols(),1.0,cp,this->fNcols);
  else if (typeid(Element) != typeid(Float_t))
    cblas_sgemm (CblasRowMajor,CblasNoTrans,CblasNoTrans,fNrows,fNcols,bta.GetNcols(),
                 1.0,btap,bta.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
  else
    Error("similarityT","type %s not implemented in BLAS library",typeid(Element));
#else
  const Int_t nbta     = bta.GetNoElements();
  const Int_t nb       = b.GetNoElements();
  const Int_t ncolsbta = bta.GetNcols();
  const Element * const btap = bta.GetMatrixArray();
  const Element * const bp   = b.GetMatrixArray();
        Element *       cp   = this->GetMatrixArray();
        Element * const cp0  = cp;

  Int_t ishift = 0;
  const Element *btarp0 = btap;                     // Pointer to  A[i,0];
  const Element *bcp0   = bp;
  while (btarp0 < btap+nbta) {
    for (const Element *bcp = bcp0; bcp < bp+ncolsb; ) { // Pointer to the j-th column of B, Start bcp = B[0,0]
      const Element *btarp = btarp0;                   // Pointer to the i-th row of A, reset to A[i,0]
      Element cij = 0;
      while (bcp < bp+nb) {                         // Scan the i-th row of A and
        cij += *btarp++ * *bcp;                     // the j-th col of B
        bcp += ncolsb;
      }
      *cp++ = cij;
      bcp -= nb-1;                                  // Set bcp to the (j+1)-th col
    }
    btarp0 += ncolsbta;                             // Set ap to the (i+1)-th row
    bcp0++;
    cp += ++ishift;
  }

  Assert(cp == cp0+this->fNelems+ishift && btarp0 == btap+nbta);

  cp = cp0;
  for (Int_t irow = 0; irow < this->fNrows; irow++) {
    const Int_t rowOff1 = irow*this->fNrows;
    for (Int_t icol = 0; icol < irow; icol++) {
      const Int_t rowOff2 = icol*this->fNrows;
      cp[rowOff1+icol] = cp[rowOff2+irow];
    }
  }
#endif

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator=(const TMatrixTSym<Element> &source)
{
  if (!AreCompatible(*this,source)) {
    Error("operator=","matrices not compatible");
    this->Invalidate();
    return *this;
  }

  if (this != &source) {
    TObject::operator=(source);
    memcpy(this->GetMatrixArray(),source.fElements,this->fNelems*sizeof(Element));
  }
  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator=(const TMatrixTSymLazy<Element> &lazy_constructor)
{
  Assert(this->IsValid());

  if (lazy_constructor.fRowUpb != this->GetRowUpb() ||
      lazy_constructor.fRowLwb != this->GetRowLwb()) {
     Error("operator=(const TMatrixTSymLazy&)", "matrix is incompatible with "
           "the assigned Lazy matrix");
    this->Invalidate();
    return *this;
  }

  lazy_constructor.FillIn(*this);
  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator=(Element val)
{
  // Assign val to every element of the matrix.

  Assert(this->IsValid());

  Element *ep = fElements;
  const Element * const ep_last = ep+this->fNelems;
  while (ep < ep_last)
    *ep++ = val;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator+=(Element val)
{
  // Add val to every element of the matrix.

  Assert(this->IsValid());

  Element *ep = fElements;
  const Element * const ep_last = ep+this->fNelems;
  while (ep < ep_last)
    *ep++ += val;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator-=(Element val)
{
  // Subtract val from every element of the matrix.

  Assert(this->IsValid());

  Element *ep = fElements;
  const Element * const ep_last = ep+this->fNelems;
  while (ep < ep_last)
    *ep++ -= val;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator*=(Element val)
{
  // Multiply every element of the matrix with val.

  Assert(this->IsValid());

  Element *ep = fElements;
  const Element * const ep_last = ep+this->fNelems;
  while (ep < ep_last)
    *ep++ *= val;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator+=(const TMatrixTSym<Element> &source)
{
  // Add the source matrix.

  if (!AreCompatible(*this,source)) {
    Error("operator+=","matrices not compatible");
    this->Invalidate();
    return *this;
  }

  const Element *sp = source.GetMatrixArray();
  Element *tp = this->GetMatrixArray();
  const Element * const tp_last = tp+this->fNelems;
  while (tp < tp_last)
    *tp++ += *sp++;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::operator-=(const TMatrixTSym<Element> &source)
{
  // Subtract the source matrix.

  if (!AreCompatible(*this,source)) {
    Error("operator-=","matrices not compatible");
    this->Invalidate();
    return *this;
  }

  const Element *sp = source.GetMatrixArray();
  Element *tp = this->GetMatrixArray();
  const Element * const tp_last = tp+this->fNelems;
  while (tp < tp_last)
    *tp++ -= *sp++;

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::Apply(const TElementActionT<Element> &action)
{ 
  Assert(this->IsValid());
  
  Element val = 0;
  Element *trp = this->GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
  Element *tcp = trp;                    // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < this->fNrows; i++) {
    trp += i;               // point to [i,i]
    tcp += i*this->fNcols;  // point to [i,i]
    for (Int_t j = i; j < this->fNcols; j++) {
      action.Operation(val);
      if (j > i) *tcp = val;
      *trp++ = val;
      tcp += this->fNcols;
    }
    tcp -= this->fNelems-1; // point to [0,i]
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::Apply(const TElementPosActionT<Element> &action)
{ 
  // Apply action to each element of the matrix. To action the location
  // of the current element is passed.
  
  Assert(this->IsValid());

  Element val = 0;
  Element *trp = this->GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
  Element *tcp = trp;                    // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < this->fNrows; i++) {
    action.fI = i+this->fRowLwb;
    trp += i;               // point to [i,i]
    tcp += i*this->fNcols;  // point to [i,i]
    for (Int_t j = i; j < this->fNcols; j++) {
      action.fJ = j+this->fColLwb;
      action.Operation(val);
      if (j > i) *tcp = val;
      *trp++ = val;
      tcp += this->fNcols;
    }
    tcp -= this->fNelems-1; // point to [0,i]
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTBase<Element> &TMatrixTSym<Element>::Randomize(Element alpha,Element beta,Double_t &seed)
{
  // randomize matrix element values but keep matrix symmetric

  Assert(this->IsValid());

  if (this->fNrows != this->fNcols || this->fRowLwb != this->fColLwb) {
    Error("Randomize(Element,Element,Element &","matrix should be square");
    this->Invalidate();
    return *this;
  }

  const Element scale = beta-alpha;
  const Element shift = alpha/scale;

  Element *ep = GetMatrixArray();
  for (Int_t i = 0; i < this->fNrows; i++) {
    const Int_t off = i*this->fNcols;
    for (Int_t j = 0; j <= i; j++) {
      ep[off+j] = scale*(Drand(seed)+shift);
      if (i != j) {
        ep[j*this->fNcols+i] = ep[off+j];
      }
    }
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &TMatrixTSym<Element>::RandomizePD(Element alpha,Element beta,Double_t &seed)
{
  // randomize matrix element values but keep matrix symmetric positive definite

  Assert(this->IsValid());

  if (this->fNrows != this->fNcols || this->fRowLwb != this->fColLwb) {
    Error("RandomizeSym(Element,Element,Element &","matrix should be square");
    this->Invalidate();
    return *this;
  }

  const Element scale = beta-alpha;
  const Element shift = alpha/scale;

  Element *ep = GetMatrixArray();
  Int_t i;
  for (i = 0; i < this->fNrows; i++) {
    const Int_t off = i*this->fNcols;
    for (Int_t j = 0; j <= i; j++)
      ep[off+j] = scale*(Drand(seed)+shift);
  }

  for (i = this->fNrows-1; i >= 0; i--) {
    const Int_t off1 = i*this->fNcols;
    for (Int_t j = i; j >= 0; j--) {
      const Int_t off2 = j*this->fNcols;
      ep[off1+j] *= ep[off2+j];
      for (Int_t k = j-1; k >= 0; k--) {
        ep[off1+j] += ep[off1+k]*ep[off2+k];
      }
      if (i != j)
        ep[off2+i] = ep[off1+j];
    }
  }

  return *this;
}

//______________________________________________________________________________
template<class Element> 
const TMatrixT<Element> TMatrixTSym<Element>::EigenVectors(TVectorT<Element> &eigenValues) const
{
  // Return a matrix containing the eigen-vectors ordered by descending eigen-values.
  // For full functionality use TMatrixDSymEigen .
  
  TMatrixDSym tmp = *this;
  TMatrixDSymEigen eigen(tmp);
  eigenValues.ResizeTo(this->fNrows);
  eigenValues = eigen.GetEigenValues();
  return eigen.GetEigenVectors();
} 

//______________________________________________________________________________
template<class Element> 
Bool_t operator==(const TMatrixTSym<Element> &m1,const TMatrixTSym<Element> &m2)
{
  // Check to see if two matrices are identical.

  if (!AreCompatible(m1,m2)) return kFALSE;
  return (memcmp(m1.GetMatrixArray(),m2.GetMatrixArray(),               
                 m1.GetNoElements()*sizeof(Element)) == 0);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator+(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  TMatrixTSym<Element> target(source1);
  target += source2;
  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator+(const TMatrixTSym<Element> &source1,Element val)
{
  TMatrixTSym<Element> target(source1);
  target += val;
  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator+(Element val,const TMatrixTSym<Element> &source1)
{
  return operator+(source1,val);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator-(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  TMatrixTSym<Element> target(source1);
  target -= source2;
  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator-(const TMatrixTSym<Element> &source1,Element val)
{
  TMatrixTSym<Element> target(source1);
  target -= val;
  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator-(Element val,const TMatrixTSym<Element> &source1)
{
  return Element(-1.0)*operator-(source1,val);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator*(const TMatrixTSym<Element> &source1,Element val)
{
  TMatrixTSym<Element> target(source1);
  target *= val;
  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator*(Element val,const TMatrixTSym<Element> &source1)
{
  return operator*(source1,val);
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator&&(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // Logical AND

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator&&(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last)
    *tp++ = (*sp1++ != 0.0 && *sp2++ != 0.0);

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator||(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // Logical Or

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator||(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last)
    *tp++ = (*sp1++ != 0.0 || *sp2++ != 0.0);

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator>(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 > source2

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator>(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last) {
    *tp++ = (*sp1) > (*sp2); sp1++; sp2++;
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator>=(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 >= source2

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator>=(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last) {
    *tp++ = (*sp1) >= (*sp2); sp1++; sp2++;
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator<=(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 <= source2

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator<=(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last) {
    *tp++ = (*sp1) <= (*sp2); sp1++; sp2++;
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> operator<(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 < source2

  TMatrixTSym<Element> target;

  if (!AreCompatible(source1,source2)) {
    Error("operator<(const TMatrixTSym&,const TMatrixTSym&)","matrices not compatible");
    target.Invalidate();
    return target;
  }

  target.ResizeTo(source1);

  const Element *sp1 = source1.GetMatrixArray();
  const Element *sp2 = source2.GetMatrixArray();
        Element *tp  = target.GetMatrixArray();
  const Element * const tp_last = tp+target.GetNoElements();
  while (tp < tp_last) {
    *tp++ = (*sp1) < (*sp2); sp1++; sp2++;
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &Add(TMatrixTSym<Element> &target,Element scalar,const TMatrixTSym<Element> &source)
{
  // Modify addition: target += scalar * source.

  if (!AreCompatible(target,source)) {
    ::Error("Add","matrices not compatible");
    target.Invalidate();
    return target;
  }

  const Int_t nrows  = target.GetNrows();
  const Int_t ncols  = target.GetNcols();
  const Int_t nelems = target.GetNoElements();
  const Element *sp  = source.GetMatrixArray();
        Element *trp = target.GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
        Element *tcp = target.GetMatrixArray(); // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < nrows; i++) {
    sp  += i;
    trp += i;        // point to [i,i]
    tcp += i*ncols;  // point to [i,i]
    for (Int_t j = i; j < ncols; j++) {
      const Element tmp = scalar * *sp++;
      if (j > i) *tcp += tmp;
      *trp++ += tmp;
      tcp += ncols;
    }
    tcp -= nelems-1; // point to [0,i]
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &ElementMult(TMatrixTSym<Element> &target,const TMatrixTSym<Element> &source)
{
  // Multiply target by the source, element-by-element.

  if (!AreCompatible(target,source)) {
    ::Error("ElementMult","matrices not compatible");
    target.Invalidate();
    return target;
  }

  const Int_t nrows  = target.GetNrows();
  const Int_t ncols  = target.GetNcols();
  const Int_t nelems = target.GetNoElements();
  const Element *sp  = source.GetMatrixArray();
        Element *trp = target.GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
        Element *tcp = target.GetMatrixArray(); // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < nrows; i++) {
    sp  += i;
    trp += i;        // point to [i,i]
    tcp += i*ncols;  // point to [i,i]
    for (Int_t j = i; j < ncols; j++) {
      if (j > i) *tcp *= *sp;
      *trp++ *= *sp++;
      tcp += ncols;
    }
    tcp -= nelems-1; // point to [0,i]
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
TMatrixTSym<Element> &ElementDiv(TMatrixTSym<Element> &target,const TMatrixTSym<Element> &source)
{
  // Multiply target by the source, element-by-element.

  if (!AreCompatible(target,source)) {
    ::Error("ElementDiv","matrices not compatible");
    target.Invalidate();
    return target;
  }

  const Int_t nrows  = target.GetNrows();
  const Int_t ncols  = target.GetNcols();
  const Int_t nelems = target.GetNoElements();
  const Element *sp  = source.GetMatrixArray();
        Element *trp = target.GetMatrixArray(); // pointer to UR part and diagonal, traverse row-wise
        Element *tcp = target.GetMatrixArray(); // pointer to LL part,              traverse col-wise
  for (Int_t i = 0; i < nrows; i++) {
    sp  += i;
    trp += i;        // point to [i,i]
    tcp += i*ncols;  // point to [i,i]
    for (Int_t j = i; j < ncols; j++) {
      Assert(*sp != 0.0);
      if (j > i) *tcp /= *sp;
      *trp++ /= *sp++;
      tcp += ncols;
    }
    tcp -= nelems-1; // point to [0,i]
  }

  return target;
}

//______________________________________________________________________________
template<class Element> 
void TMatrixTSym<Element>::Streamer(TBuffer &R__b)
{
  // Stream an object of class TMatrixTSym.

  if (R__b.IsReading()) {
    UInt_t R__s, R__c;
    Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
    Clear();
    TMatrixTBase<Element>::Class()->ReadBuffer(R__b,this,R__v,R__s,R__c);
    fElements = new Element[this->fNelems];
    Int_t i;
    for (i = 0; i < this->fNrows; i++) {
      R__b.ReadFastArray(fElements+i*this->fNcols+i,this->fNcols-i);
    }
    // copy to Lower left triangle
    for (i = 0; i < this->fNrows; i++) {
      for (Int_t j = 0; j < i; j++) {
        fElements[i*this->fNcols+j] = fElements[j*this->fNrows+i];
      }
    }
    if (this->fNelems <= this->kSizeMax) {
      memcpy(fDataStack,fElements,this->fNelems*sizeof(Element));
      delete [] fElements;
      fElements = fDataStack;
    }
  } else {
    TMatrixTBase<Element>::Class()->WriteBuffer(R__b,this);
    // Only write the Upper right triangle
    for (Int_t i = 0; i < this->fNrows; i++) {
      R__b.WriteFastArray(fElements+i*this->fNcols+i,this->fNcols-i);
    }
  }
}

#ifndef ROOT_TMatrixFSymfwd
#include "TMatrixFSymfwd.h"
#endif

template class TMatrixTSym<Float_t>;

template Bool_t       operator== <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator+  <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator+  <Float_t>(const TMatrixFSym &source1,      Float_t       val);
template TMatrixFSym  operator+  <Float_t>(      Float_t      val    ,const TMatrixFSym  &source2);
template TMatrixFSym  operator-  <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator-  <Float_t>(const TMatrixFSym &source1,      Float_t       val);
template TMatrixFSym  operator-  <Float_t>(      Float_t      val    ,const TMatrixFSym  &source2);
template TMatrixFSym  operator*  <Float_t>(const TMatrixFSym &source,       Float_t       val    );
template TMatrixFSym  operator*  <Float_t>(      Float_t      val,    const TMatrixFSym  &source );
template TMatrixFSym  operator&& <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator|| <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator>  <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator>= <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator<= <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);
template TMatrixFSym  operator<  <Float_t>(const TMatrixFSym &source1,const TMatrixFSym  &source2);

template TMatrixFSym &Add        <Float_t>(TMatrixFSym &target,      Float_t      scalar,const TMatrixFSym &source);
template TMatrixFSym &ElementMult<Float_t>(TMatrixFSym &target,const TMatrixFSym &source);
template TMatrixFSym &ElementDiv <Float_t>(TMatrixFSym &target,const TMatrixFSym &source);

#ifndef ROOT_TMatrixDSymfwd
#include "TMatrixDSymfwd.h"
#endif

template class TMatrixTSym<Double_t>;

template Bool_t       operator== <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator+  <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator+  <Double_t>(const TMatrixDSym &source1,      Double_t      val);
template TMatrixDSym  operator+  <Double_t>(      Double_t     val    ,const TMatrixDSym  &source2);
template TMatrixDSym  operator-  <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator-  <Double_t>(const TMatrixDSym &source1,      Double_t      val);    
template TMatrixDSym  operator-  <Double_t>(      Double_t     val    ,const TMatrixDSym  &source2);
template TMatrixDSym  operator*  <Double_t>(const TMatrixDSym &source,       Double_t      val    );
template TMatrixDSym  operator*  <Double_t>(      Double_t     val,    const TMatrixDSym  &source );
template TMatrixDSym  operator&& <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator|| <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator>  <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator>= <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator<= <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);
template TMatrixDSym  operator<  <Double_t>(const TMatrixDSym &source1,const TMatrixDSym  &source2);

template TMatrixDSym &Add        <Double_t>(TMatrixDSym &target,      Double_t     scalar,const TMatrixDSym &source);
template TMatrixDSym &ElementMult<Double_t>(TMatrixDSym &target,const TMatrixDSym &source);
template TMatrixDSym &ElementDiv <Double_t>(TMatrixDSym &target,const TMatrixDSym &source);
