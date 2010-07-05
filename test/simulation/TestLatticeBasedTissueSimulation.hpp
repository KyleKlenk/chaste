/*

Copyright (C) University of Oxford, 2005-2010

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef TESTLATTICEBASEDTISSUESIMULATION_HPP_
#define TESTLATTICEBASEDTISSUESIMULATION_HPP_

#include <cxxtest/TestSuite.h>

// Must be included before other cell_based headers
#include "TissueSimulationArchiver.hpp"

#include "LatticeBasedTissueSimulation.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "LatticeBasedTissue.hpp"
#include "DiffusionUpdateRule.hpp"
#include "AdvectionUpdateRule.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "WildTypeCellMutationState.hpp"
#include "RandomCellKiller.hpp"
#include "NumericFileComparison.hpp"
#include "CellBasedEventHandler.hpp"

class TestLatticeBasedTissueSimulation : public AbstractCellBasedTestSuite
{
private:

    /**
     * Compare two meshes to see if they are 'the same'.  Doesn't check everything,
     * but is fairly thorough. Used for testing serialization.
     */
    template<unsigned DIM>
    void CompareMeshes(TetrahedralMesh<DIM,DIM>* pMesh1,
                       TetrahedralMesh<DIM,DIM>* pMesh2)
    {
        TS_ASSERT_EQUALS(pMesh1->GetNumAllNodes(), pMesh2->GetNumAllNodes());
        TS_ASSERT_EQUALS(pMesh1->GetNumNodes(), pMesh2->GetNumNodes());
        TS_ASSERT_EQUALS(pMesh1->GetNumBoundaryNodes(), pMesh2->GetNumBoundaryNodes());

        for (unsigned i=0; i<pMesh1->GetNumAllNodes(); i++)
        {
            Node<DIM>* p_node = pMesh1->GetNode(i);
            Node<DIM>* p_node2 = pMesh2->GetNode(i);
            TS_ASSERT_EQUALS(p_node->IsDeleted(), p_node2->IsDeleted());
            TS_ASSERT_EQUALS(p_node->GetIndex(), p_node2->GetIndex());
            TS_ASSERT_EQUALS(p_node->IsBoundaryNode(), p_node2->IsBoundaryNode());
            for (unsigned j=0; j<DIM; j++)
            {
                TS_ASSERT_DELTA(p_node->rGetLocation()[j], p_node2->rGetLocation()[j], 1e-16);
            }
        }

        TS_ASSERT_EQUALS(pMesh1->GetNumElements(), pMesh2->GetNumElements());
        TS_ASSERT_EQUALS(pMesh1->GetNumAllElements(), pMesh2->GetNumAllElements());
        TS_ASSERT_EQUALS(pMesh1->GetNumBoundaryElements(), pMesh2->GetNumBoundaryElements());
        TS_ASSERT_EQUALS(pMesh1->GetNumAllBoundaryElements(), pMesh2->GetNumAllBoundaryElements());

        typename AbstractTetrahedralMesh<DIM,DIM>::ElementIterator iter2 = pMesh2->GetElementIteratorBegin();
        for (typename AbstractTetrahedralMesh<DIM,DIM>::ElementIterator iter = pMesh1->GetElementIteratorBegin();
             iter != pMesh1->GetElementIteratorEnd();
             ++iter, ++iter2)
        {
            TS_ASSERT_EQUALS(iter->GetNumNodes(), iter2->GetNumNodes());
            for (unsigned i=0; i<iter->GetNumNodes(); i++)
            {
                TS_ASSERT_EQUALS(iter->GetNodeGlobalIndex(i), iter2->GetNodeGlobalIndex(i));
            }
        }
    }

public:

    void TestCellsDiffusing() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(10, 10, true); // 11*11 nodes

        // Create cell
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);

        FixedDurationGenerationBasedCellCycleModel* p_model_1 = new FixedDurationGenerationBasedCellCycleModel();
        p_model_1->SetCellProliferativeType(DIFFERENTIATED);
        TissueCell cell_1(p_state, p_model_1);

        FixedDurationGenerationBasedCellCycleModel* p_model_2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model_2->SetCellProliferativeType(DIFFERENTIATED);
        TissueCell cell_2(p_state, p_model_2);

        std::vector<TissueCell> cells;
        cells.push_back(cell_1);
        cells.push_back(cell_2);

        std::vector<unsigned> real_node_indices;
        real_node_indices.push_back(47);
        real_node_indices.push_back(73);

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetDt(1);
        simulator.SetEndTime(20);

        TS_ASSERT_THROWS_THIS(simulator.Solve(), "OutputDirectory not set");
        CellBasedEventHandler::Reset();

        // Destroy the simulation time class because of failed solve
        SimulationTime::Destroy();
        SimulationTime::Instance()->SetStartTime(0.0);

        simulator.SetOutputDirectory("TestCellsDiffusing");

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(tissue.GetNumRealCells(), 2u);

        // Coverage of CalculateCellDivisionVector()
        c_vector<double, 2> cell_division_vector = simulator.CalculateCellDivisionVector(*(tissue.Begin()));
        TS_ASSERT_DELTA(cell_division_vector[0], 0.000, 1e-4);
        TS_ASSERT_DELTA(cell_division_vector[1], 0.000, 1e-4);
    }

    void TestCellsDividing() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(20, 20, true); // 21*21 nodes

        // Create cell which keeps dividing
        TissueConfig::Instance()->SetMaxTransitGenerations(UINT_MAX);
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(TRANSIT);
        TissueCell cell(p_state, p_model);
        cell.SetBirthTime(-13.5);

        std::vector<TissueCell> cells;
        cells.push_back(cell);

        std::vector<unsigned> real_node_indices;
        real_node_indices.push_back(220);

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create an empty UpdateRule system so only movement from cell birth
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestCellsDividing");
        simulator.SetDt(1);
        simulator.SetEndTime(50);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(tissue.GetNumRealCells(), 32u);
    }


    void TestDiffusionOfLargeNumberOfCells() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(49, 49, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        std::vector<unsigned> real_node_indices;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        unsigned num_cells = 100;
        for ( unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);
            TissueCell cell(p_state, p_model);
            cells.push_back(cell);

            real_node_indices.push_back(i);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestDiffusionOfLargeNumberOfCells");
        simulator.SetDt(0.1);
        simulator.SetEndTime(5.0);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(tissue.GetNumRealCells(), 100u);
    }

    void TestDiffusionAndDeathOfLargeNumberOfCells() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(19, 19, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        std::vector<unsigned> real_node_indices;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        unsigned num_cells = 100;
        for (unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);
            TissueCell cell(p_state, p_model);
            cells.push_back(cell);

            real_node_indices.push_back(i);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestDiffusionAndDeathOfLargeNumberOfCells");
        simulator.SetDt(0.1);
        simulator.SetEndTime(10);

        // Create cell killer and pass in to simulation
        RandomCellKiller<2> random_cell_killer(&tissue, 0.005);
        simulator.AddCellKiller(&random_cell_killer);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(tissue.GetNumRealCells(), 95u);
    }

    void TestDiffusionAndDivisionOfLargeNumberOfCells() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(19, 19, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        std::vector<unsigned> real_node_indices;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        unsigned num_cells = 50;
        for (unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);
            TissueCell cell(p_state, p_model);
            cell.SetBirthTime(-RandomNumberGenerator::Instance()->ranf());
            cells.push_back(cell);

            real_node_indices.push_back(i);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>*> update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestDiffusionAndDivisionOfLargeNumberOfCells");
        simulator.SetDt(0.1);
        simulator.SetEndTime(10);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(tissue.GetNumRealCells(), 50u);
    }

    void TestDiffusionAndAdvectionAndDivisionOfLargeNumberOfCells() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(49, 49, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        unsigned num_cells = 6;

        std::vector<unsigned> real_node_indices;
        real_node_indices.push_back(0);
        real_node_indices.push_back(1);
        real_node_indices.push_back(2);
        real_node_indices.push_back(50);
        real_node_indices.push_back(51);
        real_node_indices.push_back(100);

        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);

        for (unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);
            TissueCell cell(p_state, p_model);
            cell.SetBirthTime(-RandomNumberGenerator::Instance()->ranf());
            cells.push_back(cell);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> diffusion_update_rule;
        AdvectionUpdateRule<2> advection_update_rule(7, 2.0);
        std::vector<AbstractUpdateRule<2>*> update_rule_collection;
        update_rule_collection.push_back(&diffusion_update_rule);
        update_rule_collection.push_back(&advection_update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestDiffusionAndAdvectionAndDivision");
        simulator.SetDt(0.1);
        simulator.SetEndTime(10);

        // Run simulation
        TS_ASSERT_THROWS_NOTHING(simulator.Solve());
    }

    void TestMultipleAdvectionUpdateRules() throw(Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(6, 6, true); // 7*7 nodes

        // Create a single cell in the centre of the mesh
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);       
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(DIFFERENTIATED);
        TissueCell cell(p_state, p_model);

        std::vector<TissueCell> cells;
        cells.push_back(cell);

        std::vector<unsigned> real_node_indices;
        real_node_indices.push_back(24);

        // Create tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create multiple advection update rules, one for each firection 
        std::vector<AbstractUpdateRule<2>*> update_rule_collection;
        AdvectionUpdateRule<2> update_rule_0(0, 1.0);
        AdvectionUpdateRule<2> update_rule_1(1, 1.0);
        AdvectionUpdateRule<2> update_rule_2(2, 1.0);
        AdvectionUpdateRule<2> update_rule_3(3, 1.0);
        AdvectionUpdateRule<2> update_rule_4(4, 1.0);
        AdvectionUpdateRule<2> update_rule_5(5, 1.0);
        AdvectionUpdateRule<2> update_rule_6(6, 1.0);
        AdvectionUpdateRule<2> update_rule_7(7, 1.0);
        update_rule_collection.push_back(&update_rule_0);
        update_rule_collection.push_back(&update_rule_1);
        update_rule_collection.push_back(&update_rule_2);
        update_rule_collection.push_back(&update_rule_3);
        update_rule_collection.push_back(&update_rule_4);
        update_rule_collection.push_back(&update_rule_5);
        update_rule_collection.push_back(&update_rule_6);
        update_rule_collection.push_back(&update_rule_7);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("TestMultipleAdvectionUpdateRules");

        /*
         * Set the time step to be large enough to guarantee that the cell moves
         * according to each of the update rules at each time step (and hence 
         * remains at its original location).
         */ 
        simulator.SetDt(2.0);

        simulator.SetEndTime(10.0);

        // Run simulation
        simulator.Solve();

        // Test cell remains at centre of mesh
        AbstractTissue<2>::Iterator cell_iter = simulator.rGetTissue().Begin();
        c_vector<double, 2> cell_location = simulator.rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_location[0], 3.000, 1e-4);
        TS_ASSERT_DELTA(cell_location[1], 3.000, 1e-4);
    }

    void TestRandomIterationOverUpdateRules() throw (Exception)
    {
        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(10, 10, true); // 50*50 nodes

        // Create a single cell
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);
        TissueCell cell(p_state, p_model);

        std::vector<TissueCell> cells;
        cells.push_back(cell);

        std::vector<unsigned> real_node_indices;
        real_node_indices.push_back(0);

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create two update rules
        DiffusionUpdateRule<2> diffusion_update_rule(1.0); // unit diffusion coefficient
        AdvectionUpdateRule<2> advection_update_rule(0, 1.0); // flow upward with unit mean speed
        std::vector<AbstractUpdateRule<2>*> update_rule_collection;
        update_rule_collection.push_back(&diffusion_update_rule);
        update_rule_collection.push_back(&advection_update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection, true, true);
        simulator.SetOutputDirectory("TestRandomIterationOverUpdateRules");
        simulator.SetDt(0.1);
        simulator.SetEndTime(5.0);

        // Run simulation
        simulator.Solve();

        // Test final position of cell
        AbstractTissue<2>::Iterator cell_iter = simulator.rGetTissue().Begin();
        c_vector<double, 2> cell_location = simulator.rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_location[0], 0.000, 1e-4);
        TS_ASSERT_DELTA(cell_location[1], 3.000, 1e-4);
    }

        
    void TestStandardResultForArchivingTestsBelow() throw (Exception)
    {
        TissueConfig::Instance()->SetOutputCellCyclePhases(true);

        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(49, 49, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        std::vector<unsigned> real_node_indices;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        unsigned num_cells = 100;
        for (unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);
            TissueCell cell(p_state, p_model);
            cells.push_back(cell);

            real_node_indices.push_back(i);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("LatticeBasedStandardResult");
        simulator.SetDt(0.1);
        simulator.SetEndTime(8.0);

        // Create cell killer and pass in to simulation
        RandomCellKiller<2> random_cell_killer(&tissue, 0.005);
        simulator.AddCellKiller(&random_cell_killer);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(simulator.rGetTissue().GetNumRealCells(), 96u);

        AbstractTissue<2>::Iterator cell_iter = simulator.rGetTissue().Begin();
        for (unsigned i=0; i<28; i++)
        {
            ++cell_iter;
        }
        c_vector<double, 2> cell_28_location = simulator.rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_28_location[0], 31.000, 1e-4);
        TS_ASSERT_DELTA(cell_28_location[1], 3.000, 1e-4);

        for (unsigned i=29; i<61; i++)
        {
            ++cell_iter;
        }
        c_vector<double, 2> cell_61_location = simulator.rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_61_location[0], 11.000, 1e-4);
        TS_ASSERT_DELTA(cell_61_location[1], 5.000, 1e-4);

        // Check writing of Voronoi data
        OutputFileHandler handler("LatticeBasedStandardResult", false);
        std::string results_file = handler.GetOutputDirectoryFullPath() + "results_from_time_0/cellcyclephases.dat";

        NumericFileComparison comp(results_file, "projects/LatticeBased/test/data/CellCyclePhaseOutput/cellcyclephases.dat");
        TS_ASSERT(comp.CompareFiles());
        TS_ASSERT_EQUALS(system(("diff " + results_file + " projects/LatticeBased/test/data/CellCyclePhaseOutput/cellcyclephases.dat").c_str()), 0);
    }

    // Testing Save
    void TestSave() throw (Exception)
    {
        TissueConfig::Instance()->SetOutputCellCyclePhases(true);

        // Create mesh
        TetrahedralMesh<2,2> mesh;
        mesh.ConstructRectangularMesh(49, 49, true); // 50*50 nodes

        // Create cells
        std::vector<TissueCell> cells;
        std::vector<unsigned> real_node_indices;
        boost::shared_ptr<AbstractCellMutationState> p_state(new WildTypeCellMutationState);
        unsigned num_cells = 100;
        for (unsigned i=0; i<num_cells; i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);
            TissueCell cell(p_state, p_model);
            cells.push_back(cell);

            real_node_indices.push_back(i);
        }

        // Create a tissue
        LatticeBasedTissue<2> tissue(mesh, cells, real_node_indices);

        // Create a UpdateRule system
        DiffusionUpdateRule<2> update_rule;
        std::vector<AbstractUpdateRule<2>* > update_rule_collection;
        update_rule_collection.push_back(&update_rule);

        // Set up tissue simulation
        LatticeBasedTissueSimulation<2> simulator(tissue, update_rule_collection);
        simulator.SetOutputDirectory("LatticeBasedSaveAndLoad");
        simulator.SetDt(0.1);

        // Our full end time is 8.0, here we run until 3.0 then load and run more below
        simulator.SetEndTime(3.0);

        // Create cell killer and pass in to simulation
        RandomCellKiller<2> random_cell_killer(&tissue, 0.005);
        simulator.AddCellKiller(&random_cell_killer);

        // Run simulation
        simulator.Solve();

        TS_ASSERT_EQUALS(simulator.rGetTissue().GetNumRealCells(), 98u);

        // Save the results
        TissueSimulationArchiver<2, LatticeBasedTissueSimulation<2> >::Save(&simulator);
    }

    // Testing Load (based on previous two tests)
    void TestLoad() throw (Exception)
    {
        // Load the simulation from the TestSave method above and run it from 3.0 to 6.0
        LatticeBasedTissueSimulation<2>* p_simulator1;

        p_simulator1 = TissueSimulationArchiver<2, LatticeBasedTissueSimulation<2> >::Load("LatticeBasedSaveAndLoad", 3.0);

        TS_ASSERT_EQUALS(p_simulator1->rGetTissue().GetNumRealCells(), 98u);
        TS_ASSERT_DELTA(p_simulator1->GetDt(), 0.1, 1e-6);

        p_simulator1->SetEndTime(6.0);
        p_simulator1->Solve();

        // Get mesh
        TetrahedralMesh<2,2>& r_mesh1 = (static_cast<LatticeBasedTissue<2>*>(&(p_simulator1->rGetTissue())))->rGetMesh();

        // Save then reload, compare meshes either side
        TissueSimulationArchiver<2, LatticeBasedTissueSimulation<2> >::Save(p_simulator1);

        LatticeBasedTissueSimulation<2>* p_simulator2 = TissueSimulationArchiver<2, LatticeBasedTissueSimulation<2> >::Load("LatticeBasedSaveAndLoad", 6.0);
        TetrahedralMesh<2,2>& r_mesh2 = (static_cast<LatticeBasedTissue<2>*>(&(p_simulator2->rGetTissue())))->rGetMesh();

        CompareMeshes(&r_mesh1, &r_mesh2);

        // Run a bit further...
        p_simulator2->SetEndTime(8.0);

        // Run simulation
        p_simulator2->Solve();

        TS_ASSERT_EQUALS(p_simulator2->rGetTissue().GetNumRealCells(), 96u);

        AbstractTissue<2>::Iterator cell_iter = p_simulator2->rGetTissue().Begin();
        for (unsigned i=0; i<28; i++)
        {
            ++cell_iter;
        }
        c_vector<double, 2> cell_28_location = p_simulator2->rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_28_location[0], 31.000, 1e-4);
        TS_ASSERT_DELTA(cell_28_location[1], 3.000, 1e-4);

        for (unsigned i=29; i<61; i++)
        {
            ++cell_iter;
        }
        c_vector<double, 2> cell_61_location = p_simulator2->rGetTissue().GetLocationOfCellCentre(*cell_iter);
        TS_ASSERT_DELTA(cell_61_location[0], 11.000, 1e-4);
        TS_ASSERT_DELTA(cell_61_location[1], 5.000, 1e-4);

        // Tidy up
        delete p_simulator1;
        delete p_simulator2;
    }

};

#endif /*TESTLATTICEBASEDTISSUESIMULATION_HPP_*/