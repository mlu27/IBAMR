// ---------------------------------------------------------------------
//
// Copyright (c) 2019 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "ibamr/StokesFirstOrderWaveBcCoef.h"

#include "ArrayData.h"
#include "BoundaryBox.h"
#include "Box.h"
#include "CartesianGridGeometry.h"
#include "CartesianPatchGeometry.h"
#include "Index.h"
#include "IntVector.h"
#include "Patch.h"
#include "tbox/Database.h"

#include <cmath>
#include <utility>

#include "ibamr/namespaces.h"

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

namespace
{
static const int EXTENSIONS_FILLABLE = 128;
}

/////////////////////////////// PUBLIC ///////////////////////////////////////

StokesFirstOrderWaveBcCoef::StokesFirstOrderWaveBcCoef(std::string object_name,
                                                       const int comp_idx,
                                                       Pointer<Database> input_db,
                                                       Pointer<CartesianGridGeometry<NDIM> > grid_geom)
    : d_object_name(std::move(object_name)),
      d_comp_idx(comp_idx),
      d_muparser_bcs(d_object_name + "::muParser", input_db, grid_geom),
      d_grid_geom(grid_geom)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(!d_object_name.empty());
    TBOX_ASSERT(input_db);
#endif
    // Get wave parameters.
    getFromInput(input_db);

    return;
} // StokesFirstOrderWaveBcCoef

StokesFirstOrderWaveBcCoef::~StokesFirstOrderWaveBcCoef()
{
    // intentionally left-blank.
    return;
} // ~StokesFirstOrderWaveBcCoef

void
StokesFirstOrderWaveBcCoef::setBcCoefs(Pointer<ArrayData<NDIM, double> >& acoef_data,
                                       Pointer<ArrayData<NDIM, double> >& bcoef_data,
                                       Pointer<ArrayData<NDIM, double> >& gcoef_data,
                                       const Pointer<Variable<NDIM> >& variable,
                                       const Patch<NDIM>& patch,
                                       const BoundaryBox<NDIM>& bdry_box,
                                       double fill_time) const
{
    // Get pgeom info.
    const Box<NDIM>& patch_box = patch.getBox();
    const SAMRAI::hier::Index<NDIM>& patch_lower = patch_box.lower();
    Pointer<CartesianPatchGeometry<NDIM> > pgeom = patch.getPatchGeometry();
    const double* const x_lower = pgeom->getXLower();
    const double* const dx = pgeom->getDx();

    // Compute a representative grid spacing
    double vol_cell = 1.0;
    for (int d = 0; d < NDIM; ++d) vol_cell *= dx[d];
    auto alpha = d_num_interface_cells * std::pow(vol_cell, 1.0 / static_cast<double>(NDIM));

    const int dir = (NDIM == 2) ? 1 : (NDIM == 3) ? 2 : -1;

    // We take location index = 0, i.e., x_lower to be the wave inlet.
    const unsigned int location_index = bdry_box.getLocationIndex();
    if (location_index != 0)
    {
        d_muparser_bcs.setBcCoefs(acoef_data, bcoef_data, gcoef_data, variable, patch, bdry_box, fill_time);
    }
    else
    {
        const unsigned int bdry_normal_axis = location_index / 2;
        const Box<NDIM>& bc_coef_box =
            (acoef_data ? acoef_data->getBox() :
                          bcoef_data ? bcoef_data->getBox() : gcoef_data ? gcoef_data->getBox() : Box<NDIM>());
#if !defined(NDEBUG)
        TBOX_ASSERT(!acoef_data || bc_coef_box == acoef_data->getBox());
        TBOX_ASSERT(!bcoef_data || bc_coef_box == bcoef_data->getBox());
        TBOX_ASSERT(!gcoef_data || bc_coef_box == gcoef_data->getBox());
#endif

        const double fac = (d_gravity * d_wave_number * d_amplitude) / d_omega;
        double dof_posn[NDIM];
        for (Box<NDIM>::Iterator b(bc_coef_box); b; b++)
        {
            const hier::Index<NDIM>& i = b();
            if (acoef_data) (*acoef_data)(i, 0) = 1.0;
            if (bcoef_data) (*bcoef_data)(i, 0) = 0.0;

            for (unsigned int d = 0; d < NDIM; ++d)
            {
                if (d != bdry_normal_axis)
                {
                    dof_posn[d] = x_lower[d] + dx[d] * (static_cast<double>(i(d) - patch_lower(d)) + 0.5);
                }
                else
                {
                    dof_posn[d] = x_lower[d] + dx[d] * (static_cast<double>(i(d) - patch_lower(d)));
                }
            }

            // Compute a numerical heaviside at the boundary from the analytical wave
            // elevation
            const double theta = d_wave_number * dof_posn[0] - d_omega * fill_time;
            const double kd = d_wave_number * d_depth;
            const double z_plus_d = dof_posn[dir];
            const double eta = d_amplitude * cos(theta);
            const double phi = -eta + (z_plus_d - d_depth);
            double h_phi;
            if (phi < -alpha)
                h_phi = 1.0;
            else if (std::abs(phi) <= alpha)
                h_phi = 1.0 - (0.5 + 0.5 * phi / alpha + 1.0 / (2.0 * M_PI) * std::sin(M_PI * phi / alpha));
            else
                h_phi = 0.0;

            if (d_comp_idx == 0)
            {
                if (gcoef_data)
                    (*gcoef_data)(i, 0) = h_phi * fac * cosh(d_wave_number * (z_plus_d)) * cos(theta) / cosh(kd);
            }

            if (d_comp_idx == 1)
            {
#if (NDIM == 2)
                if (gcoef_data)
                    (*gcoef_data)(i, 0) = h_phi * fac * sinh(d_wave_number * z_plus_d) * sin(theta) / cosh(kd);
#elif (NDIM == 3)
                if (gcoef_data) (*gcoef_data)(i, 0) = 0.0;
#endif
            }

#if (NDIM == 3)
            if (d_comp_idx == 2)
            {
                if (gcoef_data)
                    (*gcoef_data)(i, 0) = h_phi * fac * sinh(d_wave_number * z_plus_d) * sin(theta) / cosh(kd);
            }
#endif
        }
    }
    return;
} // setBcCoefs

IntVector<NDIM>
StokesFirstOrderWaveBcCoef::numberOfExtensionsFillable() const
{
    return EXTENSIONS_FILLABLE;
} // numberOfExtensionsFillable

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////
void
StokesFirstOrderWaveBcCoef::getFromInput(Pointer<Database> input_db)
{
    Pointer<Database> wave_db = input_db->getDatabase("wave_parameters_db");
#if !defined(NDEBUG)
    TBOX_ASSERT(input_db->isDatabase("wave_parameters_db"));
#endif

    d_depth = wave_db->getDouble("depth");
    d_omega = wave_db->getDouble("omega");
    d_gravity = wave_db->getDouble("gravitational_constant");
    d_wave_number = wave_db->getDouble("wave_number");
    d_amplitude = wave_db->getDouble("amplitude");
    d_num_interface_cells = wave_db->getDouble("num_interface_cells");

    return;
} // getFromInput

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBAMR
