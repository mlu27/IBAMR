// ---------------------------------------------------------------------
//
// Copyright (c) 2020 - 2020 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

#ifndef included_IBTK_FEValues
#define included_IBTK_FEValues

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <ibtk/FECache.h>
#include <ibtk/JacobianCalculator.h>

#include <tbox/Utilities.h>

#include "libmesh/enum_fe_family.h"
#include "libmesh/enum_order.h"
#include "libmesh/enum_quadrature_type.h"
#include "libmesh/type_vector.h"
#include <libmesh/enum_elem_type.h>
#include <libmesh/fe.h>
#include <libmesh/point.h>
#include <libmesh/quadrature.h>

#include <map>
#include <vector>

namespace IBTK
{
/**
 * Class like libMesh::FE for element shape function calculations, but
 * optimized for isoparametric Lagrange finite elements.
 */
template <int dim>
class FEValues
{
public:
    FEValues(libMesh::QBase* qrule, const FEUpdateFlags update_flags);

    virtual void reinit(const libMesh::Elem* elem);

    inline const std::vector<double>& getJxW() const
    {
        return d_JxW;
    }

    inline const std::vector<libMesh::Point>& getQuadraturePoints() const
    {
        return d_quadrature_points;
    }

    inline const std::vector<std::vector<double> >& getShapeValues() const
    {
        return d_shape_values;
    }

    inline const std::vector<std::vector<libMesh::VectorValue<double> > >& getShapeGradients() const
    {
        return d_shape_gradients;
    }

protected:
    libMesh::QBase* d_qrule;

    std::vector<double> d_JxW;

    std::vector<libMesh::Point> d_quadrature_points;

    std::vector<std::vector<double> > d_shape_values;

    std::vector<std::vector<libMesh::VectorValue<double> > > d_shape_gradients;

    /**
     * Reference values, extracted from libMesh.
     */
    struct ReferenceValues
    {
        ReferenceValues(const libMesh::QBase& quadrature);

        const libMesh::ElemType d_elem_type;

        /**
         * shape values, indexed by quadrature point and then by shape function
         * index
         */
        boost::multi_array<double, 2> d_reference_shape_values;

        /**
         * shape gradients, indexed by quadrature point and then by shape function
         * index.
         */
        boost::multi_array<libMesh::VectorValue<double>, 2> d_reference_shape_gradients;
    };

    /*
     * Mappings, indexed by element type.
     */
    std::map<libMesh::ElemType, std::unique_ptr<Mapping<dim> > > d_mappings;

    /*
     * Reference values, indexed by element type.
     */
    std::map<libMesh::ElemType, ReferenceValues> d_reference_values;

    /*
     * Things to actually recompute.
     */
    FEUpdateFlags d_update_flags;

    /**
     * Last element type. We can avoid reinitializing some things if this
     * matches the current element type.
     */
    libMesh::ElemType d_last_elem_type = libMesh::ElemType::INVALID_ELEM;
};
} // namespace IBTK

#endif //#ifndef included_IBTK_FEValues