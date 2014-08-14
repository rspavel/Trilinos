// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_OBJECTIVE_SIMOPT_H
#define ROL_OBJECTIVE_SIMOPT_H

#include "ROL_Objective.hpp"

/** \class ROL::Objective_SimOpt
    \brief Provides the interface to evaluate simulation-based objective functions.
*/


namespace ROL {

template <class Real>
class Objective_SimOpt : public Objective<Real> {
public:

  /** \brief Update objective function.  
                u is an iterate, 
                z is an iterate, 
                flag = true if the iterate has changed,
                iter is the outer algorithm iterations count.
  */
  virtual void update( const Vector<Real> &u, const Vector<Real> &z, bool flag = true, int iter = -1 ) {}
  void update( const Vector<Real> &x, bool flag = true, int iter = -1 ) {
    const ROL::Vector_SimOpt<Real> &xs = Teuchos::dyn_cast<const ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<const ROL::Vector<Real> >(x));
    this->update(*(xs.get_1()),*(xs.get_2()),flag,iter);
  }

  /** \brief Compute value.
  */
  virtual Real value( const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  Real value( const Vector<Real> &x, Real &tol ) {
    const ROL::Vector_SimOpt<Real> &xs = Teuchos::dyn_cast<const ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<const ROL::Vector<Real> >(x));
    return this->value(*(xs.get_1()),*(xs.get_2()),tol);
  }

  /** \brief Compute gradient with respect to first component.
  */
  virtual void gradient_1( Vector<Real> &g, const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  /** \brief Compute gradient with respect to second component.
  */
  virtual void gradient_2( Vector<Real> &g, const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  void gradient( Vector<Real> &g, const Vector<Real> &x, Real &tol ) {
    ROL::Vector_SimOpt<Real> &gs = Teuchos::dyn_cast<ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<ROL::Vector<Real> >(g));
    const ROL::Vector_SimOpt<Real> &xs = Teuchos::dyn_cast<const ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<const ROL::Vector<Real> >(x));
    Teuchos::RCP<Vector<Real> > g1 = gs.get_1()->clone();
    Teuchos::RCP<Vector<Real> > g2 = gs.get_2()->clone();
    this->gradient_1(*g1,*(xs.get_1()),*(xs.get_2()),tol);
    this->gradient_2(*g2,*(xs.get_1()),*(xs.get_2()),tol);
    gs.set_1(*g1);
    gs.set_2(*g2);
  }

  /** \brief Apply Hessian approximation to vector.
  */
  virtual void hessVec_11( Vector<Real> &hv, const Vector<Real> &v, 
                           const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  virtual void hessVec_12( Vector<Real> &hv, const Vector<Real> &v, 
                           const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  virtual void hessVec_21( Vector<Real> &hv, const Vector<Real> &v, 
                           const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  virtual void hessVec_22( Vector<Real> &hv, const Vector<Real> &v, 
                           const Vector<Real> &u, const Vector<Real> &z, Real &tol ) = 0;
  void hessVec( Vector<Real> &hv, const Vector<Real> &v, const Vector<Real> &x, Real &tol ) {
    ROL::Vector_SimOpt<Real> &hvs = Teuchos::dyn_cast<ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<ROL::Vector<Real> >(hv));
    const ROL::Vector_SimOpt<Real> &vs = Teuchos::dyn_cast<const ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<const ROL::Vector<Real> >(v));
    const ROL::Vector_SimOpt<Real> &xs = Teuchos::dyn_cast<const ROL::Vector_SimOpt<Real> >(
      Teuchos::dyn_cast<const ROL::Vector<Real> >(x));
    Teuchos::RCP<Vector<Real> > h11 = (xs.get_1())->clone();
    this->hessVec_11(*h11,*(vs.get_1()),*(xs.get_1()),*(xs.get_2()),tol);
    Teuchos::RCP<Vector<Real> > h12 = (xs.get_1())->clone();
    this->hessVec_12(*h12,*(vs.get_2()),*(xs.get_1()),*(xs.get_2()),tol);
    Teuchos::RCP<Vector<Real> > h21 = (xs.get_2())->clone();
    this->hessVec_21(*h21,*(vs.get_1()),*(xs.get_1()),*(xs.get_2()),tol);
    Teuchos::RCP<Vector<Real> > h22 = (xs.get_2())->clone();
    this->hessVec_22(*h22,*(vs.get_2()),*(xs.get_1()),*(xs.get_2()),tol);
    h11->plus(*h12);
    hvs.set_1(*h11);
    h22->plus(*h21);
    hvs.set_2(*h22);
  }

}; // class Step

} // namespace ROL

#endif
