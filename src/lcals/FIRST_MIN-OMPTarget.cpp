//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-24, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "FIRST_MIN.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_TARGET_OPENMP)

#include "common/OpenMPTargetDataUtils.hpp"

#include <iostream>

namespace rajaperf
{
namespace lcals
{

  //
  // Define threads per team for target execution
  //
  const size_t threads_per_team = 256;


void FIRST_MIN::runOpenMPTargetVariant(VariantID vid, size_t RAJAPERF_UNUSED_ARG(tune_idx))
{
  const Index_type run_reps = getRunReps();
  const Index_type ibegin = 0;
  const Index_type iend = getActualProblemSize();

  FIRST_MIN_DATA_SETUP;

  if ( vid == Base_OpenMPTarget ) {

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      #pragma omp declare reduction(minloc : MyMinLoc : \
                                    omp_out = MinLoc_compare(omp_out, omp_in))\
                                    initializer (omp_priv = omp_orig)

      FIRST_MIN_MINLOC_INIT;

      #pragma omp target is_device_ptr(x) device( did ) map(tofrom:mymin)
      #pragma omp teams distribute parallel for thread_limit(threads_per_team) schedule(static, 1) \
                  reduction(minloc:mymin)
      for (Index_type i = ibegin; i < iend; ++i ) {
        FIRST_MIN_BODY;
      }

      m_minloc = mymin.loc;

    }
    stopTimer();

  } else if ( vid == RAJA_OpenMPTarget ) {

    auto res{getOmpTargetResource()};

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      RAJA::expt::ValLoc<Real_type, Index_type> tminloc(m_xmin_init,
                                                        m_initloc);

      RAJA::forall<RAJA::omp_target_parallel_for_exec<threads_per_team>>( res,
        RAJA::RangeSegment(ibegin, iend),
        RAJA::expt::Reduce<RAJA::operators::minimum>(&tminloc),
        [=](Index_type i,
          RAJA::expt::ValLocOp<Real_type, Index_type,
                               RAJA::operators::minimum>& minloc) {
          FIRST_MIN_BODY_RAJA;
        }
      );

      m_minloc = static_cast<Index_type>(tminloc.getLoc());

    }
    stopTimer();

  } else {
     getCout() << "\n  FIRST_MIN : Unknown OMP Target variant id = " << vid << std::endl;
  }

}

} // end namespace lcals
} // end namespace rajaperf

#endif  // RAJA_ENABLE_TARGET_OPENMP
