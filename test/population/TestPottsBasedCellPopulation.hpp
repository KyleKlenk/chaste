/*

Copyright (C) University of Oxford, 2005-2011

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

#ifndef TESTPOTTSBASEDCELLPOPULATION_HPP_
#define TESTPOTTSBASEDCELLPOPULATION_HPP_

#include <cxxtest/TestSuite.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "CellwiseData.hpp"
#include "CellsGenerator.hpp"
#include "PottsBasedCellPopulation.hpp"
#include "PottsMeshGenerator.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "ArchiveOpener.hpp"
#include "WildTypeCellMutationState.hpp"
#include "ApcOneHitCellMutationState.hpp"
#include "ApcTwoHitCellMutationState.hpp"
#include "BetaCateninOneHitCellMutationState.hpp"
#include "CellLabel.hpp"

class TestPottsBasedCellPopulation : public AbstractCellBasedTestSuite
{
public:

    // Test construction, accessors and iterator
    void TestCreateSmallPottsBasedCellPopulationAndGetWidth() throw (Exception)
    {
        // Create a simple 2D PottsMesh
        PottsMeshGenerator generator(4, 4, 2, 2, 2, 2);
        PottsMesh* p_mesh = generator.GetMesh();

        // Create cells
        std::vector<CellPtr> cells;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasic(cells, p_mesh->GetNumElements());

        // Create cell population
        unsigned num_cells = cells.size();
        PottsBasedCellPopulation cell_population(*p_mesh, cells);

        // Test we have the correct number of cells and elements
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), p_mesh->GetNumElements());
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), num_cells);

        unsigned counter = 0;

        // Test PottsBasedCellPopulation::Iterator
        for (PottsBasedCellPopulation::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            // Test operator* and that cells are in sync
            TS_ASSERT_EQUALS(cell_population.GetLocationIndexUsingCell(*cell_iter), counter);

            // Test operator-> and that cells are in sync
            TS_ASSERT_DELTA(cell_iter->GetAge(), (double)counter, 1e-12);

            TS_ASSERT_EQUALS(counter, p_mesh->GetElement(counter)->GetIndex());

            counter++;
        }

        // Test we have gone through all cells in the for loop
        TS_ASSERT_EQUALS(counter, cell_population.GetNumRealCells());

        // Test GetNumNodes() method
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), p_mesh->GetNumNodes());

        // Test GetWidth() method
        double width_x = cell_population.GetWidth(0);
        TS_ASSERT_DELTA(width_x, 3.0, 1e-4);

        double width_y = cell_population.GetWidth(1);
        TS_ASSERT_DELTA(width_y, 3.0, 1e-4);

        // Test RemoveDeadCells() method
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 4u);
        cell_population.Begin()->Kill();
        TS_ASSERT_EQUALS(cell_population.RemoveDeadCells(), 1u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 3u);
        TS_ASSERT_EQUALS(p_mesh->GetNumElements(), 3u);
        TS_ASSERT_EQUALS(p_mesh->GetNumAllElements(), 4u);
    }

    void TestValidate() throw (Exception)
    {
		// Create a simple potts-based mesh
        PottsMeshGenerator generator(4, 4, 2, 2, 2, 2);
        PottsMesh* p_mesh = generator.GetMesh();

        // Create cells
        std::vector<CellPtr> cells;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasic(cells, p_mesh->GetNumElements()-1);

        std::vector<unsigned> cell_location_indices;
        for (unsigned i=0; i<cells.size(); i++)
        {
            cell_location_indices.push_back(i);
        }

		// This should throw an exception as the number of cells does not equal the number of elements
		std::vector<CellPtr> cells_copy(cells);
		TS_ASSERT_THROWS_THIS(PottsBasedCellPopulation cell_population(*p_mesh, cells_copy),
				"Element 3 does not appear to have a cell associated with it");

        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
		FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
		p_model->SetCellProliferativeType(STEM);
		CellPtr p_cell(new Cell(p_state, p_model));

		double birth_time = 0.0 - p_mesh->GetNumElements()-1;
		p_cell->SetBirthTime(birth_time);

		cells.push_back(p_cell);
		cell_location_indices.push_back(p_mesh->GetNumElements()-1);

		// This should pass as the number of cells equals the number of elements
		std::vector<CellPtr> cells_copy2(cells);
		TS_ASSERT_THROWS_NOTHING(PottsBasedCellPopulation cell_population(*p_mesh, cells_copy2));

		// Create cell population
		PottsBasedCellPopulation cell_population(*p_mesh, cells);

		// Check correspondence between elements and cells
		for (PottsMesh::PottsElementIterator iter = p_mesh->GetElementIteratorBegin();
			 iter != p_mesh->GetElementIteratorEnd();
			 ++iter)
		{
			std::set<unsigned> expected_node_indices;
			unsigned expected_index = iter->GetIndex();

			for (unsigned i=0; i<iter->GetNumNodes(); i++)
			{
				expected_node_indices.insert(iter->GetNodeGlobalIndex(i));
			}

			std::set<unsigned> actual_node_indices;
			unsigned elem_index = iter->GetIndex();
			CellPtr p_cell = cell_population.GetCellUsingLocationIndex(elem_index);
			PottsElement* p_actual_element = cell_population.GetElementCorrespondingToCell(p_cell);
			unsigned actual_index = p_actual_element->GetIndex();

			for (unsigned i=0; i<p_actual_element->GetNumNodes(); i++)
			{
				actual_node_indices.insert(p_actual_element->GetNodeGlobalIndex(i));
			}

			TS_ASSERT_EQUALS(actual_index, expected_index);
			TS_ASSERT_EQUALS(actual_node_indices, expected_node_indices);
		}

		// Create another simple potts-based mesh
        PottsMeshGenerator generator2(4, 4, 2, 2, 2, 2);
        PottsMesh* p_mesh2 = generator2.GetMesh();

        // Create cells
        std::vector<CellPtr> cells2;
        CellsGenerator<FixedDurationGenerationBasedCellCycleModel, 2> cells_generator2;
        cells_generator2.GenerateBasic(cells2, p_mesh2->GetNumElements()+1);

        std::vector<unsigned> cell_location_indices2;
        for (unsigned i=0; i<cells2.size(); i++)
        {
            cell_location_indices2.push_back(i%p_mesh2->GetNumElements()); // Element 0 will have 2 cells
        }

		// This should throw an exception as the number of cells
		// does not equal the number of elements
		TS_ASSERT_THROWS_THIS(PottsBasedCellPopulation cell_population2(*p_mesh2, cells2, false, true, cell_location_indices2),
				"Element 0 appears to have 2 cells associated with it");
    }
};

#endif /*TESTPOTTSBASEDCELLPOPULATION_HPP_*/