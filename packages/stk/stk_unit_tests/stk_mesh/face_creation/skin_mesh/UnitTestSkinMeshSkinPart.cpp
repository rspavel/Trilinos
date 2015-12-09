#include <gtest/gtest.h>

#include <vector>
#include <algorithm>
#include <stdlib.h>

#include <stk_topology/topology.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/Comm.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/FEMHelpers.hpp>
#include <stk_mesh/baseImpl/MeshImplUtils.hpp>
#include <stk_mesh/base/FieldTraits.hpp>
#include <stk_mesh/base/Field.hpp>

#include <stk_mesh/base/SkinMesh.hpp>
#include <stk_mesh/base/CreateFaces.hpp>
#include <stk_mesh/base/ElemElemGraph.hpp>
#include <stk_mesh/base/ElemElemGraphImpl.hpp>

#include <stk_util/parallel/Parallel.hpp>
#include <stk_util/parallel/ParallelVectorConcat.hpp>
#include <stk_util/parallel/ParallelComm.hpp>
#include <stk_util/environment/WallTime.hpp>
#include <stk_util/environment/memory_util.hpp>
#include <stk_util/parallel/CommSparse.hpp>
#include <stk_util/environment/ReportHandler.hpp>

#include <stk_io/IossBridge.hpp>
#include <stk_io/StkMeshIoBroker.hpp>   // for StkMeshIoBroker

#include <stk_unit_test_utils/ioUtils.hpp>
#include <stk_unit_test_utils/getOption.h>

#include "stk_unit_test_utils/unittestMeshUtils.hpp"
#include <stk_unit_test_utils/MeshFixture.hpp>

#include <stk_unit_tests/stk_mesh/SetupKeyholeMesh.hpp>

#include <stk_mesh/fixtures/QuadFixture.hpp>  // for QuadFixture
#include <stk_mesh/fixtures/heterogeneous_mesh.hpp>
#include <stk_mesh/fixtures/degenerate_mesh.hpp>

#include "UnitTestSkinMeshUseCaseUtils.hpp"

namespace {

using namespace stk::mesh::impl;
using namespace stk::mesh;


void add_element_to_block(stk::mesh::BulkData& bulkData, stk::mesh::Entity element, stk::mesh::Part& block)
{
    bulkData.change_entity_parts(element, {&block}, {});
}

stk::mesh::PartVector create_element_blocks(stk::mesh::MetaData& meta, stk::topology top)
{
    stk::mesh::PartVector blocks;
    blocks.push_back(&meta.declare_part_with_topology("block_2", top));
    blocks.push_back(&meta.declare_part_with_topology("block_3", top));
    blocks.push_back(&meta.declare_part_with_topology("block_4", top));
    return blocks;
}

stk::mesh::EntityVector get_elements(stk::mesh::BulkData& bulkData)
{
    stk::mesh::EntityVector elements;
    stk::mesh::EntityRank rank = stk::topology::ELEM_RANK;
    elements.push_back(bulkData.get_entity(rank, 2));
    elements.push_back(bulkData.get_entity(rank, 3));
    elements.push_back(bulkData.get_entity(rank, 4));
    return elements;
}

void add_elements_to_various_blocks(stk::mesh::BulkData& bulkData, stk::mesh::PartVector& blocks, stk::mesh::EntityVector& elements)
{
    bulkData.modification_begin();
    for(size_t i=0;i<elements.size();++i)
    {
        if (bulkData.is_valid(elements[i]) && bulkData.bucket(elements[i]).owned())
        {
            add_element_to_block(bulkData, elements[i], *blocks[i]);
        }
    }
    bulkData.modification_end();
}


std::vector<size_t> get_num_faces_per_elements(const stk::mesh::BulkData& bulkData, const stk::mesh::EntityVector& elements)
{
    std::vector<size_t> num_sides_on_element(elements.size(),0);
    for(size_t i=0;i<elements.size();++i)
    {
        if (bulkData.is_valid(elements[i]))
        {
            num_sides_on_element[i] = bulkData.num_sides(elements[i]);
        }
    }
    return num_sides_on_element;
}

void skin_block_and_test(stk::mesh::BulkData& bulkData, const std::vector<size_t>& num_gold_faces_per_element, const size_t gold_num_faces, const stk::mesh::Part& block, const stk::mesh::PartVector &skin_parts, const stk::mesh::EntityVector& elements)
{
    ElemGraphTestUtils::skin_part(bulkData, block, skin_parts);

    std::vector<size_t> num_faces_on_element = get_num_faces_per_elements(bulkData, elements);
    for(size_t i=0; i<elements.size(); ++i)
    {
        if (bulkData.is_valid(elements[i]))
        {
            EXPECT_EQ(num_faces_on_element[i], num_gold_faces_per_element[i]);
        }
    }

    std::vector<size_t> mesh_counts;
    stk::mesh::comm_mesh_counts(bulkData, mesh_counts);
    EXPECT_EQ(gold_num_faces, mesh_counts[bulkData.mesh_meta_data().side_rank()]);
}


//////////////////////////// 3D

void test_skinning_by_adding_skin_of_block_4(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_4, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 6, 6, 6 };
    size_t gold_num_total_faces = 16u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_4, skin_parts, elements);
}

void test_skinning_by_adding_skin_of_block_1(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_1, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 6, 6, 6, 6 };
    size_t gold_num_total_faces = 21u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_1, skin_parts, elements);
}

void test_skinning_of_block_2(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_2, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 6, 1, 0 };
    size_t gold_num_total_faces = 6u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_2, skin_parts, elements);
}

void test_skinning_by_adding_skin_of_block_3(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_3, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 6, 6, 1 };
    size_t gold_num_total_faces = 11u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_3, skin_parts, elements);
}

////////////////////////////// 2D

void test_skinning_by_adding_skin_of_block_4_2D(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_4, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 4, 4, 4 };
    size_t gold_num_total_faces = 10u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_4, skin_parts, elements);
}

void test_skinning_by_adding_skin_of_block_1_2D(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_1, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 4, 4, 4, 4 };
    size_t gold_num_total_faces = 13u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_1, skin_parts, elements);
}

void test_skinning_of_block_2_2D(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_2, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 4, 1, 0 };
    size_t gold_num_total_faces = 4u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_2, skin_parts, elements);
}

void test_skinning_by_adding_skin_of_block_3_2D(stk::mesh::BulkData& bulkData, const stk::mesh::Part& block_3, const stk::mesh::PartVector& skin_parts, const stk::mesh::EntityVector &elements)
{
    std::vector<size_t> gold_num_faces_per_element = { 1, 4, 4, 1 };
    size_t gold_num_total_faces = 7u;
    skin_block_and_test(bulkData, gold_num_faces_per_element, gold_num_total_faces, block_3, skin_parts, elements);
}

TEST(ElementGraph, skin_part)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

     if(stk::parallel_machine_size(comm) <= 2)
     {
         unsigned spatialDim = 3;

         stk::mesh::MetaData meta(spatialDim);
         stk::mesh::BulkData bulkData(meta, comm);
         make_2_hex_mesh_with_element1_inactive(bulkData);
         stk::mesh::PartVector skin_parts = get_skin_parts(meta);
         ElemGraphTestUtils::skin_part(bulkData, *meta.get_part("active"), skin_parts);
         ElemGraphTestUtils::test_num_faces_per_element(bulkData, {6u, 1u});
         std::vector<size_t> global_mesh_counts;
         stk::mesh::comm_mesh_counts(bulkData, global_mesh_counts);
         EXPECT_EQ(6u, global_mesh_counts[meta.side_rank()]);
     }
}

TEST(ElementGraph, skin_part_3_blocks)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

     if(stk::parallel_machine_size(comm) <= 2)
     {
         unsigned spatialDim = 3;

         stk::mesh::MetaData meta(spatialDim);
         stk::mesh::BulkData bulkData(meta, comm);
         stk::mesh::Part& skin = meta.declare_part_with_topology("skin", stk::topology::QUAD_4);

         stk::mesh::PartVector blocks = create_element_blocks(meta, stk::topology::HEX_8);

         stk::mesh::Part& active = meta.declare_part("active");
         stk::unit_test_util::fill_mesh_using_stk_io("generated:1x1x4", bulkData, comm);

         stk::mesh::EntityVector elements = get_elements(bulkData);
         add_elements_to_various_blocks(bulkData, blocks, elements);

         elements.insert(elements.begin(), bulkData.get_entity(stk::topology::ELEM_RANK, 1));
         blocks.insert(blocks.begin(), meta.get_part("block_1"));

         test_skinning_of_block_2(bulkData, *blocks[1], {&active, &skin}, elements);
         test_skinning_by_adding_skin_of_block_3(bulkData, *blocks[2], {&active, &skin}, elements);
         test_skinning_by_adding_skin_of_block_4(bulkData, *blocks[3], {&active, &skin}, elements);
         test_skinning_by_adding_skin_of_block_1(bulkData, *blocks[0], {&active, &skin}, elements);
     }
}

TEST(ElementGraph, skin_part_3_blocks_2D)
{
    stk::ParallelMachine comm = MPI_COMM_WORLD;

    if(stk::parallel_machine_size(comm) <= 4)
    {
        const unsigned X = 4, Y = 1;
        bool auraOn = true;
        stk::mesh::fixtures::QuadFixture fixture(comm, X, Y, auraOn);

        stk::mesh::MetaData & meta = fixture.m_meta;
        stk::mesh::Part& skin = meta.declare_part_with_topology("skin", stk::topology::LINE_2);

        stk::mesh::PartVector blocks = create_element_blocks(meta, stk::topology::QUAD_4);
        stk::mesh::Part& active = meta.declare_part("active");

        fixture.m_meta.commit();
        fixture.generate_mesh();

        stk::mesh::BulkData & bulkData = fixture.m_bulk_data;

        stk::mesh::EntityVector elements = get_elements(bulkData);
        add_elements_to_various_blocks(bulkData, blocks, elements);

        elements.insert(elements.begin(), bulkData.get_entity(stk::topology::ELEM_RANK, 1));
        blocks.insert(blocks.begin(), meta.get_part("quad_part")); // because quad fixture doesn't do block_1

        test_skinning_of_block_2_2D(bulkData, *blocks[1], {&active, &skin}, elements);
        test_skinning_by_adding_skin_of_block_3_2D(bulkData, *blocks[2], {&active, &skin}, elements);
        test_skinning_by_adding_skin_of_block_4_2D(bulkData, *blocks[3], {&active, &skin}, elements);
        test_skinning_by_adding_skin_of_block_1_2D(bulkData, *blocks[0], {&active, &skin}, elements);
    }
}

} // namespace
