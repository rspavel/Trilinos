// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef SACADO_TEMPLATEMANAGER_HPP
#define SACADO_TEMPLATEMANAGER_HPP

#include "Teuchos_RefCountPtr.hpp"

#include "Sacado_ConfigDefs.h"

#include "Sacado_mpl_size.hpp"
#include "Sacado_mpl_find.hpp"
#include "Sacado_mpl_for_each.hpp"

namespace Sacado {

  // Forward declaration
  template <typename TypeSeq, typename BaseT, template<typename> class ObjectT>
  class TemplateIterator;
  template <typename TypeSeq, typename BaseT, template<typename> class ObjectT>
  class ConstTemplateIterator;

  //! Container class to manager template instantiations of a template class
  /*!
   * This class provides a generic container class for managing multiple
   * instantiations of another class ObjectT.  It assumes each class
   * ObjectT<ScalarT> is derived from a non-template base class BaseT.  It
   * stores a vector of reference counted pointers to objects of type
   * BaseT corresponding to each instantiation of ObjectT.  The instantiations
   * ObjectT for each ScalarT are provided by a builder class, passed through
   * the buildObjects() method (see DefaultBuilderOp for an example builder).
   * An iterator is provided for accessing each template instantiation, and
   * non-templated virtual methods of BaseT can be called by dereferencing
   * the iterator.  Finally, template methods are provided to access the
   * stored objects as either objects of type BaseT (getAsBase()) or objects
   * of type ObjectT<ScalarT> (getAsObject()).  A map using RTTI is used to
   * map a typename to an index into the vector corresponding to the object
   * of that type.
   *
   * Template managers for specific types should derive from this class
   * instantiated on those types.  In most cases, a builder class will also
   * need to be created to instantiate the objects for each scalar type.
   */
  template <typename TypeSeq, typename BaseT, template<typename> class ObjectT>
  class TemplateManager {

    //! Implementation of < for type_info objects
    struct type_info_less {
      bool operator() (const type_info* a, const type_info* b) {
	return a->before(*b);
      }
    };

    template <typename BuilderOpT>
    struct BuildObject {
      mutable std::vector< Teuchos::RefCountPtr<BaseT> >& objects;
      const BuilderOpT& builder;
      BuildObject(std::vector< Teuchos::RefCountPtr<BaseT> >& objects_,
		  const BuilderOpT& builder_) :
	objects(objects_), builder(builder_) {}
      template <typename T> void operator()(T) const {
	int idx = mpl::find<TypeSeq,T>::value;
	objects[idx] = builder.template build<T>();
      }
    };

    //! Declare TemplateIterator as a friend class
    friend class TemplateIterator<TypeSeq,BaseT,ObjectT>;

  public:

    //! Typedef for iterator
    typedef TemplateIterator<TypeSeq,BaseT,ObjectT> iterator;

    //! Typedef for const_iterator
    typedef ConstTemplateIterator<TypeSeq,BaseT,ObjectT> const_iterator;

    //! The default builder class for building objects for each ScalarT
    struct DefaultBuilderOp {

      //! Returns a new rcp for an object of type ObjectT<ScalarT>
      template<class ScalarT>
      Teuchos::RefCountPtr<BaseT> build() const {
	return Teuchos::rcp( dynamic_cast<BaseT*>( new ObjectT<ScalarT> ) );
      }

    };

    //! Default constructor
    TemplateManager();

    //! Destructor
    ~TemplateManager();

    //! Build objects for each ScalarT
    template <typename BuilderOpT>
    void buildObjects(const BuilderOpT& builder);

    //! Build objects for each ScalarT using default builder
    void buildObjects();

    //! Get RCP to object corrensponding to ScalarT as BaseT
    template<typename ScalarT>
    Teuchos::RefCountPtr<BaseT> getAsBase();

    //! Get RCP to object corrensponding to ScalarT as BaseT
    template<typename ScalarT>
    Teuchos::RefCountPtr<const BaseT> getAsBase() const;

    //! Get RCP to object corrensponding to ScalarT as ObjectT<ScalarT>
    template<typename ScalarT>
    Teuchos::RefCountPtr< ObjectT<ScalarT> > getAsObject();

    //! Get RCP to object corrensponding to ScalarT as ObjectT<ScalarT>
    template<typename ScalarT>
    Teuchos::RefCountPtr< const ObjectT<ScalarT> > getAsObject() const;

    //! Return an iterator that points to the first type object
    typename Sacado::TemplateManager<TypeSeq,BaseT,ObjectT>::iterator begin();

    //! Return an iterator that points to the first type object
    typename Sacado::TemplateManager<TypeSeq,BaseT,ObjectT>::const_iterator 
    begin() const;

    //! Return an iterator that points one past the last type object
    typename Sacado::TemplateManager<TypeSeq,BaseT,ObjectT>::iterator end();

    //! Return an iterator that points one past the last type object
    typename Sacado::TemplateManager<TypeSeq,BaseT,ObjectT>::const_iterator 
    end() const;

  private:

    //! Stores array of rcp's to objects of each type
    std::vector< Teuchos::RefCountPtr<BaseT> > objects;

  };

}

// Include template definitions
#include "Sacado_TemplateManagerImp.hpp"

// Include template iterator definition
#include "Sacado_TemplateIterator.hpp"

#endif
