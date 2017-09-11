/*
//@HEADER
// ************************************************************************
//
//               KokkosKernels 0.9: Linear Algebra and Graph Kernels
//                 Copyright 2017 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/


#include "KokkosSparse_spgemm.hpp"
#include "KokkosKernels_TestParameters.hpp"


#define TRANPOSEFIRST false
#define TRANPOSESECOND false

namespace KokkosKernels{

namespace Experiment{
template <typename crsMat_t, typename device>
bool is_same_matrix(crsMat_t output_mat1, crsMat_t output_mat2){

  typedef typename crsMat_t::StaticCrsGraphType graph_t;
  typedef typename graph_t::row_map_type::non_const_type lno_view_t;
  typedef typename graph_t::entries_type::non_const_type   lno_nnz_view_t;
  typedef typename crsMat_t::values_type::non_const_type scalar_view_t;

  size_t nrows1 = output_mat1.graph.row_map.dimension_0();
  size_t nentries1 = output_mat1.graph.entries.dimension_0() ;
  size_t nvals1 = output_mat1.values.dimension_0();

  size_t nrows2 = output_mat2.graph.row_map.dimension_0();
  size_t nentries2 = output_mat2.graph.entries.dimension_0() ;
  size_t nvals2 = output_mat2.values.dimension_0();


  lno_nnz_view_t h_ent1 (Kokkos::ViewAllocateWithoutInitializing("e1"), nentries1);
  scalar_view_t h_vals1 (Kokkos::ViewAllocateWithoutInitializing("v1"), nvals1);


  KokkosKernels::Impl::kk_sort_graph<typename graph_t::row_map_type,
    typename graph_t::entries_type,
    typename crsMat_t::values_type,
    lno_nnz_view_t,
    scalar_view_t,
    typename device::execution_space
    >(
    output_mat1.graph.row_map, output_mat1.graph.entries, output_mat1.values,
    h_ent1, h_vals1
  );

  lno_nnz_view_t h_ent2 (Kokkos::ViewAllocateWithoutInitializing("e1"), nentries2);
  scalar_view_t h_vals2 (Kokkos::ViewAllocateWithoutInitializing("v1"), nvals2);

  if (nrows1 != nrows2) return false;
  if (nentries1 != nentries2) return false;
  if (nvals1 != nvals2) return false;

  KokkosKernels::Impl::kk_sort_graph
      <typename graph_t::row_map_type,
      typename graph_t::entries_type,
      typename crsMat_t::values_type,
      lno_nnz_view_t,
      scalar_view_t,
      typename device::execution_space
      >(
      output_mat2.graph.row_map, output_mat2.graph.entries, output_mat2.values,
      h_ent2, h_vals2
    );

  bool is_identical = true;
  is_identical = KokkosKernels::Impl::kk_is_identical_view
      <typename graph_t::row_map_type, typename graph_t::row_map_type, typename lno_view_t::value_type,
      typename device::execution_space>(output_mat1.graph.row_map, output_mat2.graph.row_map, 0);
  if (!is_identical) return false;

  is_identical = KokkosKernels::Impl::kk_is_identical_view
      <lno_nnz_view_t, lno_nnz_view_t, typename lno_nnz_view_t::value_type,
      typename device::execution_space>(h_ent1, h_ent2, 0 );
  if (!is_identical) return false;

  is_identical = KokkosKernels::Impl::kk_is_identical_view
      <scalar_view_t, scalar_view_t, typename scalar_view_t::value_type,
      typename device::execution_space>(h_vals1, h_vals2, 0.000001);
  if (!is_identical) {
    std::cout << "Incorret values" << std::endl;
  }
  return true;
}


template <typename ExecSpace, typename crsMat_t, typename crsMat_t2 , typename crsMat_t3 , typename TempMemSpace , typename PersistentMemSpace >
crsMat_t3 run_experiment(
    crsMat_t crsMat, crsMat_t2 crsMat2, Parameters params){
    //int algorithm, int repeat, int chunk_size ,int multi_color_scale, int shmemsize, int team_size, int use_dynamic_scheduling, int verbose){

  using namespace KokkosSparse;
  using namespace KokkosSparse::Experimental;
  int algorithm = params.algorithm;
  int repeat = params.repeat;
  int chunk_size = params.chunk_size;
  int multi_color_scale = params.multi_color_scale;
  int shmemsize = params.shmemsize;
  int team_size = params.team_size;
  int use_dynamic_scheduling = params.use_dynamic_scheduling;
  int verbose = params.verbose;
  int calculate_read_write_cost = params.calculate_read_write_cost;
  char *coloring_input_file = params.coloring_input_file;
  char *coloring_output_file = params.coloring_output_file;
  //char spgemm_step = params.spgemm_step;
  int vector_size = params.vector_size;
  int check_output = params.check_output;
  int mkl_sort_option = params.mkl_sort_option;
  int mkl_keep_output = params.mkl_keep_output;
  //spgemm_step++;
  typedef typename crsMat_t3::values_type::non_const_type scalar_view_t;
  typedef typename crsMat_t3::StaticCrsGraphType::row_map_type::non_const_type lno_view_t;
  typedef typename crsMat_t3::StaticCrsGraphType::entries_type::non_const_type lno_nnz_view_t;

  lno_view_t row_mapC;
  lno_nnz_view_t entriesC;
  scalar_view_t valuesC;

  typedef KokkosKernels::Experimental::KokkosKernelsHandle
      <lno_view_t,lno_nnz_view_t, scalar_view_t,
      ExecSpace, TempMemSpace,PersistentMemSpace > KernelHandle;

  typedef typename lno_nnz_view_t::value_type idx;
  typedef typename lno_view_t::value_type size_type;

  KernelHandle kh;
  kh.set_team_work_size(chunk_size);
  kh.set_shmem_size(shmemsize);
  kh.set_suggested_team_size(team_size);
  kh.set_suggested_vector_size(vector_size);

  if (use_dynamic_scheduling){
    kh.set_dynamic_scheduling(true);
  }
  if (verbose){
    kh.set_verbose(true);
  }

  const idx m = crsMat.numRows();
  const idx n = crsMat2.numRows();
  const idx k = crsMat2.numCols();

  std::cout << "m:" << m << " n:" << n << " k:" << k << std::endl;
  if (n < crsMat.numCols()){
    std::cout << "left.numCols():" << crsMat.numCols() << " right.numRows():" << crsMat2.numRows() << std::endl;
    exit(1);
  }

  lno_view_t row_mapC_ref;
  lno_nnz_view_t entriesC_ref;
  scalar_view_t valuesC_ref;
  crsMat_t3 Ccrsmat_ref;
  if (check_output)
  {
    std::cout << "Running a reference algorithm" << std::endl;
    row_mapC_ref = lno_view_t ("non_const_lnow_row", m + 1);
    entriesC_ref = lno_nnz_view_t ("");
    valuesC_ref = scalar_view_t ("");
    KernelHandle sequential_kh;
    sequential_kh.set_team_work_size(chunk_size);
    sequential_kh.set_shmem_size(shmemsize);
    sequential_kh.set_suggested_team_size(team_size);
    sequential_kh.create_spgemm_handle(KokkosSparse::SPGEMM_KK_MEMORY);

    if (use_dynamic_scheduling){
      sequential_kh.set_dynamic_scheduling(true);
    }


    spgemm_symbolic (
        &sequential_kh,
        m,
        n,
        k,
        crsMat.graph.row_map,
        crsMat.graph.entries,
        TRANPOSEFIRST,
        crsMat2.graph.row_map,
        crsMat2.graph.entries,
        TRANPOSESECOND,
        row_mapC_ref
    );

    ExecSpace::fence();


    size_type c_nnz_size = sequential_kh.get_spgemm_handle()->get_c_nnz();
    if (c_nnz_size){
      entriesC_ref = lno_nnz_view_t (Kokkos::ViewAllocateWithoutInitializing("entriesC"), c_nnz_size);
      valuesC_ref = scalar_view_t (Kokkos::ViewAllocateWithoutInitializing("valuesC"), c_nnz_size);
    }

    spgemm_numeric(
        &sequential_kh,
        m,
        n,
        k,
        crsMat.graph.row_map,
        crsMat.graph.entries,
        crsMat.values,
        TRANPOSEFIRST,

        crsMat2.graph.row_map,
        crsMat2.graph.entries,
        crsMat2.values,
        TRANPOSESECOND,
        row_mapC_ref,
        entriesC_ref,
        valuesC_ref
    );
    ExecSpace::fence();

    typename crsMat_t3::StaticCrsGraphType static_graph (entriesC_ref, row_mapC_ref);
    crsMat_t3 Ccrsmat("CrsMatrixC", k, valuesC_ref, static_graph);
    Ccrsmat_ref = Ccrsmat;
  }

  for (int i = 0; i < repeat; ++i){
  switch (algorithm){
  case 1:
    kh.create_spgemm_handle(SPGEMM_MKL);
    break;
  case 2:
    kh.create_spgemm_handle(SPGEMM_CUSPARSE);
    break;
  case 3:
    kh.create_spgemm_handle(SPGEMM_CUSP);
    break;
  case 4:
    kh.create_spgemm_handle(SPGEMM_DEBUG);
    break;
  case 5:
    kh.create_spgemm_handle(SPGEMM_MKL2PHASE);
    kh.get_spgemm_handle()->set_mkl_sort_option(mkl_sort_option);
    break;
  case 6:
    kh.create_spgemm_handle(SPGEMM_KK_MEMORY2);
    break;
  case 7:
      kh.create_spgemm_handle(SPGEMM_KK_MEMORY);
      break;
  case 8:
      kh.create_spgemm_handle(SPGEMM_KK_SPEED);
      break;
  case 9:
    kh.create_spgemm_handle(SPGEMM_KK_COLOR);
    break;

  case 10:
    kh.create_spgemm_handle(SPGEMM_KK_MULTICOLOR);
    break;

  case 11:
      kh.create_spgemm_handle(SPGEMM_KK_MULTICOLOR2);
      break;
  case 12:
    kh.create_spgemm_handle(SPGEMM_VIENNA);
    break;
  case 13:
    kh.create_spgemm_handle(SPGEMM_KK_MEMSPEED);
    break;
  case 14:
    kh.create_spgemm_handle(SPGEMM_KK_MULTIMEM);
    break;
  case 15:
    kh.create_spgemm_handle(SPGEMM_KK_OUTERMULTIMEM);
    break;


  default:
    kh.create_spgemm_handle(SPGEMM_KK_MEMORY);
    break;
  }

  kh.get_spgemm_handle()->set_multi_color_scale(multi_color_scale);
  kh.get_spgemm_handle()->mkl_keep_output = mkl_keep_output;
  kh.get_spgemm_handle()->mkl_convert_to_1base = false;
  kh.get_spgemm_handle()->set_read_write_cost_calc (calculate_read_write_cost);
  kh.get_spgemm_handle()->set_compression_steps(!params.compression2step);

  if (coloring_input_file){
    kh.get_spgemm_handle()->coloring_input_file = /*std::string(&spgemm_step) + "_" +*/ std::string(coloring_input_file);
  }
  if (coloring_output_file){
    kh.get_spgemm_handle()->coloring_output_file = /*std::string(&spgemm_step) + "_" +*/ std::string(coloring_output_file);
  }


    row_mapC = lno_view_t
              ("non_const_lnow_row",
                  m + 1);
    entriesC = lno_nnz_view_t ("");
    valuesC = scalar_view_t ("");

    Kokkos::Impl::Timer timer1;
    spgemm_symbolic (
        &kh,
        m,
        n,
        k,
        crsMat.graph.row_map,
        crsMat.graph.entries,
        TRANPOSEFIRST,
        crsMat2.graph.row_map,
        crsMat2.graph.entries,
        TRANPOSESECOND,
        row_mapC
    );

    ExecSpace::fence();
    double symbolic_time = timer1.seconds();

    Kokkos::Impl::Timer timer3;
    size_type c_nnz_size = kh.get_spgemm_handle()->get_c_nnz();
    if (c_nnz_size){
      entriesC = lno_nnz_view_t (Kokkos::ViewAllocateWithoutInitializing("entriesC"), c_nnz_size);
      valuesC = scalar_view_t (Kokkos::ViewAllocateWithoutInitializing("valuesC"), c_nnz_size);
    }

    spgemm_numeric(
        &kh,
        m,
        n,
        k,
        crsMat.graph.row_map,
        crsMat.graph.entries,
        crsMat.values,
        TRANPOSEFIRST,

        crsMat2.graph.row_map,
        crsMat2.graph.entries,
        crsMat2.values,
        TRANPOSESECOND,
        row_mapC,
        entriesC,
        valuesC
    );
    ExecSpace::fence();
    double numeric_time = timer3.seconds();

    std::cout
    << "mm_time:" << symbolic_time + numeric_time
    << " symbolic_time:" << symbolic_time
    << " numeric_time:" << numeric_time << std::endl;



  }

  std::cout << "row_mapC:" << row_mapC.dimension_0() << std::endl;
  std::cout << "entriesC:" << entriesC.dimension_0() << std::endl;
  std::cout << "valuesC:" << valuesC.dimension_0() << std::endl;
  KokkosKernels::Impl::print_1Dview(valuesC);
  KokkosKernels::Impl::print_1Dview(entriesC);


  typename crsMat_t3::StaticCrsGraphType static_graph (entriesC, row_mapC);
  crsMat_t3 Ccrsmat("CrsMatrixC", k, valuesC, static_graph);
  if (check_output){
    bool is_identical = is_same_matrix<crsMat_t3, typename crsMat_t3::device_type>(Ccrsmat_ref, Ccrsmat);
    if (!is_identical){
      std::cout << "Result is wrong." << std::endl;
      exit(1);
    }
  }


  return Ccrsmat;

}


}
}
