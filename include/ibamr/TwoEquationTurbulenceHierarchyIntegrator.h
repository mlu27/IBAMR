// Filename: TwoEquationTurbulenceHierarchyIntegrator.h
// Created on 11 October 2019 by Ramakrishnan Thirumalaisamy
//
// Copyright (c) 2002-2017, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#ifndef included_IBAMR_TwoEquationTurbulenceHierarchyIntegrator
#define included_IBAMR_TwoEquationTurbulenceHierarchyIntegrator

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "ibamr/AdvDiffSemiImplicitHierarchyIntegrator.h"
#include "ibamr/INSVCStaggeredHierarchyIntegrator.h"
#include "ibamr/ibamr_enums.h"
#include "ibamr/ibamr_utilities.h"
//#include "TurbulenceSSTKOmegaSourceFunction.h"

#include "ibtk/LaplaceOperator.h"

#include "IntVector.h"
#include "tbox/Database.h"
#include "tbox/Pointer.h"

#include <map>
#include <string>
#include <vector>

namespace IBAMR
{
class TurbulenceSSTKOmegaSourceFunction;
} // namespace IBAMR
namespace IBTK
{
class CartGridFunction;
class LaplaceOperator;
class PoissonSolver;
} // namespace IBTK
namespace SAMRAI
{
namespace hier
{
template <int DIM>
class BasePatchHierarchy;
template <int DIM>
class PatchHierarchy;
} // namespace hier
namespace mesh
{
template <int DIM>
class GriddingAlgorithm;
} // namespace mesh
namespace pdat
{
template <int DIM, class TYPE>
class CellVariable;
template <int DIM, class TYPE>
class FaceVariable;
template <int DIM, class TYPE>
class SideVariable;
} // namespace pdat
namespace tbox
{
class Database;
} // namespace tbox
} // namespace SAMRAI
/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
class TwoEquationTurbulenceHierarchyIntegrator : public AdvDiffSemiImplicitHierarchyIntegrator
{
public:
    /*!
     * The constructor for class TwoEquationTurbulenceHierarchyIntegrator sets some
     * default values, reads in configuration information from input and restart
     * databases, and registers the integrator object with the restart manager
     * when requested.
     */
    TwoEquationTurbulenceHierarchyIntegrator(const std::string& object_name,
                                             SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
                                             bool register_for_restart = true);

    /*!
     * Initialize the variables, basic communications algorithms, solvers, and
     * other data structures used by this time integrator object.
     *
     * This method is called automatically by initializePatchHierarchy() prior
     * to the construction of the patch hierarchy.  It is also possible for
     * users to make an explicit call to initializeHierarchyIntegrator() prior
     * to calling initializePatchHierarchy().
     */
    void
    initializeHierarchyIntegrator(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                  SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg) override;
    /*!
     * Register a cell-centered turbulent kinetic energy, \f$ k \f$.
     */
    void registerKVariable(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var);

    /*!
     * Register a cell-centered turbulent specific dissipation rate, \f$ w \f$.
     */
    void registerWVariable(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var);

    /*!
     * Get the \f$ k \f$ variable registered with the hierarchy integrator.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > getKVariable() const;
    /*!
     * Get the \f$ w \f$ variable registered with the hierarchy integrator.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > getWVariable() const;

    /*!
     * Prepare to advance the data from current_time to new_time.
     */
    void preprocessIntegrateHierarchy(double current_time, double new_time, int num_cycles = 1) override;

    /*!
     * Synchronously advance each level in the hierarchy over the given time
     * increment.
     */
    void integrateHierarchy(double current_time, double new_time, int cycle_num = 0) override;

    /*!
     * Clean up data following call(s) to integrateHierarchy().
     */
    void postprocessIntegrateHierarchy(double current_time,
                                       double new_time,
                                       bool skip_synchronize_new_state_data,
                                       int num_cycles = 1) override;
    /*!
     * Set the face-centered advection velocity to be used with a cell-centered turbulent kinetic energy,\f$ k \f$.
     *
     * \note The specified advection velocity must have been already registered
     * with the hierarchy integrator.
     */
    void setAdvectionVelocityKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                       SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM, double> > u_var);
    /*!
     * Set the face-centered advection velocity to be used with a cell-centered turbulent specific dissipation rate,\f$
     * w \f$.
     *
     * \note The specified advection velocity must have been already registered
     * with the hierarchy integrator.
     */
    void setAdvectionVelocityWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                       SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM, double> > u_var);
    /*!
     * Supply an IBTK::CartGridFunction object to compute the source terms appears in
     * \f$ k \f$ equation
     */
    void setSourceTermFunctionKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> k_F_fcn);
    /*!
     * Supply an IBTK::CartGridFunction object to compute the source terms appears in
     * \f$ w \f$ equation
     */
    void setSourceTermFunctionWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> w_F_fcn);

    /*!
     * Set the diffusion time integration scheme for the turbulence kinetic energy, \f$ k \f$, that has been
     * registered with the hierarchy integrator.
     */
    void setDiffusionTimeSteppingTypeKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                               TimeSteppingType time_stepping_type);
    /*!
     * Set the diffusion time integration scheme for the turbulence specific dissipation rate, \f$ w \f$, that has
     * been registered with the hierarchy integrator.
     */
    void setDiffusionTimeSteppingTypeWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                               TimeSteppingType time_stepping_type);
    /*!
     * Set the convective time stepping method to use for turbulent kinetic energy, \f$ k \f$.
     */
    void setConvectiveTimeSteppingTypeKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > K_var,
                                                TimeSteppingType convective_time_stepping_type);
    /*!
     * Set the convective time stepping method to use for turbulent specific dissipation rate, \f$ w \f$.
     */
    void setConvectiveTimeSteppingTypeWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > W_var,
                                                TimeSteppingType convective_time_stepping_type);

    /*!
     * Set the convective time stepping method used during the initial time step
     * for the turbulent kinetic energy, \f$ k \f$.
     *
     * \note This is used \em only when the basic convective time stepping
     * scheme uses a multi-step method such as Adams-Bashforth.
     */
    void setInitialConvectiveTimeSteppingTypeKEquation(
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
        TimeSteppingType init_convective_time_stepping_type);
    /*!
     * Set the convective time stepping method used during the initial time step
     * for the turbulent specific dissipation rate, \f$ w \f$.
     *
     * \note This is used \em only when the basic convective time stepping
     * scheme uses a multi-step method such as Adams-Bashforth.
     */
    void setInitialConvectiveTimeSteppingTypeWEquation(
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
        TimeSteppingType init_convective_time_stepping_type);

    /*!
     * Set a grid function to provide initial conditions for turbulent kinetic energy,  \f$ k \f$, that has
     * been registered with the hierarchy integrator.
     */
    void setInitialConditionsKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                       SAMRAI::tbox::Pointer<IBTK::CartGridFunction> k_init);
    /*!
     * Set a grid function to provide initial conditions for turbulent specific dissipation rate,  \f$ w \f$,
     * that has been registered with the hierarchy integrator.
     */
    void setInitialConditionsWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                       SAMRAI::tbox::Pointer<IBTK::CartGridFunction> w_init);
    /*!
     * Set an object to provide boundary conditions for turbulent kinetic energy,  \f$ k \f$,
     * that has been registered with the hierarchy integrator.
     */
    void setPhysicalBcCoefKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* k_bc_coef);
    /*!
     * Set objects to provide boundary conditions for a vector-valued quantity
     * that has been registered with the hierarchy integrator.
     */
    void setPhysicalBcCoefsKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var,
                                     const std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*>& k_bc_coef);

    /*!
     * Return a  \f$ k \f$ boundary condition object
     */
    std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*> getPhysicalBcCoefsKEquation();
    /*!
     * Set an object to provide boundary conditions for turbulent specific dissipation rate,  \f$ w \f$,
     * that has been registered with the hierarchy integrator.
     */
    void setPhysicalBcCoefWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* w_bc_coef);

    /*!
     * Set objects to provide boundary conditions for a vector-valued quantity
     * that has been registered with the hierarchy integrator.
     */
    void setPhysicalBcCoefsWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var,
                                     const std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*>& w_bc_coef);
    /*!
     * Return a  \f$ w \f$ boundary condition object
     */
    std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*> getPhysicalBcCoefsWEquation();

    /*!
     * Get the solver for the Helmholtz equation (time-discretized diffusion
     * of  \f$ k \f$ equation)
     */
    SAMRAI::tbox::Pointer<IBTK::PoissonSolver>
    getHelmholtzSolverKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var);

    /*!
     * Get the solver for the Helmholtz equation (time-discretized diffusion
     * of  \f$ w \f$ equation)
     */
    SAMRAI::tbox::Pointer<IBTK::PoissonSolver>
    getHelmholtzSolverWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var);

    /*!
     * Get the operator to use to evaluate the right-hand side for the Helmholtz
     * solver (time-discretized diffusion equation of  \f$ k \f$ equation).
     */
    SAMRAI::tbox::Pointer<IBTK::LaplaceOperator>
    getHelmholtzRHSOperatorKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var);

    /*!
     * Get the operator to use to evaluate the right-hand side for the Helmholtz
     * solver (time-discretized diffusion equation of  \f$ w \f$ equation).
     */
    SAMRAI::tbox::Pointer<IBTK::LaplaceOperator>
    getHelmholtzRHSOperatorWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var);

    /*!
     * Get the convective operator being used by this solver class for
     * turbulent kinetic energy,  \f$ k \f$.
     *
     * If the convective operator has not already been constructed, then this
     * function will initialize a default convective operator.
     */
    SAMRAI::tbox::Pointer<ConvectiveOperator>
    getConvectiveOperatorKEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > k_var);

    /*!
     * Get the convective operator being used by this solver class for a
     * turbulent specific dissipation rate,  \f$ w \f$.
     *
     * If the convective operator has not already been constructed, then this
     * function will initialize a default convective operator.
     */
    SAMRAI::tbox::Pointer<ConvectiveOperator>
    getConvectiveOperatorWEquation(SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > w_var);

    /*!
     * Reset cached hierarchy dependent data.
     */
    void
    resetHierarchyConfigurationSpecialized(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                                           int coarsest_level,
                                           int finest_level) override;

    /*!
     * Register INSVCStaggeredHierarchyIntegrator class which will be
     * used to get the variables maintained by this hierarchy integrator.
     */
    void registerINSVCStaggeredHierarchyIntegrator(
        SAMRAI::tbox::Pointer<INSVCStaggeredHierarchyIntegrator> ins_hier_integrator);

    /*!
     * Compute the turbulent viscosity.
     *
     * /note The turbulent viscosity is maintained by the INSVCStaggeredHierarchyIntegrator.
     */
    void calculateTurbulentViscosity();

private:
    /*!
     * To grant a access to the blending and production variables scratch index to the
     * TurbulenceSSTKOmegaSourceFunction class.
     */
    friend class TurbulenceSSTKOmegaSourceFunction;

    /*!
     * Compute the turbulent kinetic energy production.
     */
    void calculateTurbulentKEProduction(const double data_time);

    /**
     * Compute the belending function.
     */
    void calculateBlendingFunction(const double data_time,
                                   const SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext> ctx);
    /**
     * Calculate F2
     */
    void calculateF2();

    /*!
     * Read input values from a given database.
     */
    void getFromInput(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db, bool is_from_restart);

    // Cell-centered transport quantites i.e., \f$ k \f$ and \f$ w \f$
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_var, d_k_rhs_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_w_var, d_w_rhs_var;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_N_var, d_w_N_var, d_k_N_old_var, d_w_N_old_var;

    // Source function variables.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_F_var, d_w_F_var;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_k_F_fcn, d_w_F_fcn;

    // Blending function F1.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_F1_var;

    // F2 variable.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_F2_var;

    // Turbulent kinetic energy production.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_p_var;

    // Distance to closest surface.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_distance_to_closest_surface_var;
    SAMRAI::tbox::Array<int> d_wall_location_index;

    // Molecular viscosity
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_mu_cc_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_mu_const_cc_var;

    // Turbulent viscosity.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_mu_t_var;

    // Effective viscosity
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_mu_eff_cc_var;

    // temporary variables
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_temp_var, d_w_temp_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_temp_rhs_var, d_w_temp_rhs_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_dissipation_var, d_w_dissipation_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_k_C_var, d_w_C_var;

    // Density.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_rho_cc_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::SideVariable<NDIM, double> > d_rho_sc_var;

    // Transported quantities.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM, double> > d_k_u_var, d_w_u_var;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_k_u_fcn, d_w_u_fcn;

    ConvectiveDifferencingType d_k_convective_difference_form, d_w_convective_difference_form;
    std::string d_k_convective_op_type, d_w_convective_op_type;
    SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> d_k_convective_op_input_db, d_w_convective_op_input_db;
    SAMRAI::tbox::Pointer<ConvectiveOperator> d_k_convective_op, d_w_convective_op;
    bool d_k_convective_op_needs_init, d_w_convective_op_needs_init;

    TimeSteppingType d_k_diffusion_time_stepping_type, d_k_convective_time_stepping_type,
        d_k_init_convective_time_stepping_type, d_w_diffusion_time_stepping_type, d_w_convective_time_stepping_type,
        d_w_init_convective_time_stepping_type;

    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_k_init, d_w_init;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_k_F_init, d_w_F_init;
    std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*> d_k_bc_coef, d_w_bc_coef;
    SAMRAI::tbox::Pointer<IBTK::HierarchyGhostCellInterpolation> d_k_bdry_bc_fill_op, d_w_bdry_bc_fill_op,
        d_mu_eff_bdry_bc_fill_op;

    /*!
     * Diffusion coefficient data
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::SideVariable<NDIM, double> > d_k_diffusion_coef_var, d_k_diffusion_coef_rhs_var,
        d_w_diffusion_coef_var, d_w_diffusion_coef_rhs_var;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_k_diffusion_coef_fcn, d_w_diffusion_coef_fcn;
    bool d_k_is_diffusion_coef_variable, d_w_is_diffusion_coef_variable;

    /*!
     * Objects to keep track of the resetting functions.
     */
    std::map<SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> >, std::vector<ResetPropertiesFcnPtr> >
        d_k_reset_fcns, d_w_reset_fcns;
    std::map<SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> >, std::vector<void*> > d_k_reset_fcns_ctx,
        d_w_reset_fcns_ctx;
    std::vector<int> d_k_reset_priority, d_w_reset_priority;

    /*!
     * solvers and related data.
     */
    std::string d_k_solver_type, d_k_precond_type, d_w_solver_type, d_w_precond_type;
    SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> d_k_solver_db, d_k_precond_db, d_w_solver_db, d_w_precond_db;
    SAMRAI::tbox::Pointer<IBTK::LaplaceOperator> d_k_rhs_op, d_w_rhs_op;
    SAMRAI::tbox::Pointer<IBTK::PoissonSolver> d_k_solver, d_w_solver;
    bool d_k_solver_needs_init, d_w_solver_needs_init, d_k_rhs_op_needs_init, d_w_rhs_op_needs_init;

    SAMRAI::tbox::Pointer<SAMRAI::solv::SAMRAIVectorReal<NDIM, double> > d_k_sol, d_k_rhs, d_w_sol, d_w_rhs;
    /*
     * Patch data descriptor indices for all "state" variables managed by the
     * integrator.
     *
     * State variables have three contexts: current, scratch, and new.
     */
    int d_k_current_idx, d_k_new_idx, d_k_scratch_idx;
    int d_w_current_idx, d_w_new_idx, d_w_scratch_idx;
    int d_rho_cc_new_idx, d_rho_sc_new_idx, d_rho_cc_current_idx, d_rho_sc_current_idx;
    int d_mu_cc_new_idx, d_mu_cc_current_idx;

    /*
     * Patch data descriptor indices for all "scratch" variables managed by the
     * integrator.
     *
     * Scratch variables have only one context: scratch.
     */
    int d_F1_scratch_idx, d_F2_scratch_idx, d_p_scratch_idx, d_distance_to_closest_surface_scratch_idx;
    int d_mu_eff_cc_scratch_idx;
    int d_rho_const_idx;
    int d_mu_const_idx;
    int d_k_temp_rhs_idx, d_w_temp_rhs_idx;
    int d_k_temp_idx, d_w_temp_idx;
    int d_k_dissipation_idx, d_w_dissipation_idx;
    int d_k_C_idx, d_w_C_idx;

    /*
     * Pointer INSVCStaggeredConservativeHierarchyIntegrator
     * integrator.
     */
    SAMRAI::tbox::Pointer<INSVCStaggeredHierarchyIntegrator> d_ins_hierarchy_integrator;

    double d_sigma_k = 0.85, d_sigma_w = 0.5, d_beta = 0.075;

    /*!
     * Double precision values are (optional) factors used to rescale the
     * density and viscosity for plotting.
     *
     * Boolean values indicates whether to output various quantities for
     * visualization.
     */
    double d_k_scale = 1.0, d_w_scale = 1.0;
    bool d_output_k = false, d_output_w = false;
};

} // namespace IBAMR

#endif //#ifndef included_IBAMR_TwoEquationTurbulenceHierarchyIntegrator
