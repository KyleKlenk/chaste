/*

Copyright (c) 2005-2017, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef MISRAFORCE_HPP_
#define MISRAFORCE_HPP_

#include <boost/serialization/base_object.hpp>
#include <iostream>
#include "AbstractForce.hpp"
#include "ChasteSerialization.hpp"
#include "Exception.hpp"
#include "VertexBasedCellPopulation.hpp"

/**
 * A force class for use in vertex-based model simulations. This force is based on the
 * energy function proposed by Misra et al in the following paper:
 *
 * Mahim Misra, Basile Audoly, Ioannis G. Kevrekidis & Stanislav Y. Shvartsman.
 * Shape transformations of epithelial shells.
 * Biophysical Journal, 110(7):1670-1678 (2016).
 * http://dx.doi.org/10.1016/j.bpj.2016.03.009
 */
class MisraForce : public AbstractForce<3>
{
    friend class TestForces;

private:
    friend class boost::serialization::access;
    /**
     * Boost Serialization method for archiving/checkpointing.
     * Archives the object and its member variables.
     *
     * @param archive  The boost archive.
     * @param version  The current version of this class.
     */
    template <class Archive>
    void serialize(Archive& archive, const unsigned int version)
    {
        archive& boost::serialization::base_object<AbstractForce<3> >(*this);
        archive& mApicalLineTensionParameter;
        archive& mLateralSurfaceEnergyParameter;
        archive& mBasalSurfaceEnergyParameter;
        archive& mVolumeCompressibilityParameter;
    }

protected:
    /**
     * The strength of the apical edge term in the model. Corresponds to sigma in Misra et al's paper.
     * Initialised to 1.0 in the constructor.
     */
    double mApicalLineTensionParameter;

    /**
     * The strength of the lateral surface term in the model. Corresponds to alpha in Misra et al's paper.
     * Initialised to 2.0 in the constructor.
     */
    double mLateralSurfaceEnergyParameter;

    /**
     * The strength of the basal surface term in the model. Corresponds to gamma in Misra et al's paper.
     * Initialised to 0.98 in the constructor.
     */
    double mBasalSurfaceEnergyParameter;

    /**
     * The strength of the volume deviation term in the model. Corresponds to Beta in Misra et al's paper.
     * Initialised to 100.0 in the constructor.
     */
    double mVolumeCompressibilityParameter;

    /**
     * Temporary placeholder for target volume. (temporary value before GrowthModifier is implemented) ///\todo #2850
     * Initialised to 1.0 in the constructor.
     */
    double mTargetVolume;
    ///\todo allow prepattern and eventually curved surfaces.  #2850

public:
    /**
     * Constructor.
     */
    MisraForce();

    /**
     * Destructor.
     */
    virtual ~MisraForce();

    /**
     * Overridden AddForceContribution() method.
     *
     * Calculate the force on each node in the vertex-based cell population based on the energy function in Misra et al's paper.
     *
     * @param rCellPopulation reference to the cell population
     */
    virtual void AddForceContribution(AbstractCellPopulation<3>& rCellPopulation);

    /**
     * Get the line tension parameter for the edge between two given nodes.
     *
     * @param pNodeA one node
     * @param pNodeB the other node
     * @param rVertexCellPopulation reference to the cell population
     *
     * @return the line tension parameter for this edge.
     */
    virtual double GetApicalLineTensionParameter(Node<3>* pNodeA, Node<3>* pNodeB, VertexBasedCellPopulation<3>& rVertexCellPopulation) const;

    /**
     * @return mVolumeCompressibilityParameter
     */
    double GetVolumeCompressibilityParameter() const;

    /**
     * @return mLateralSurfaceEnergyParameter
     */
    double GetLateralSurfaceEnergyParameter() const;

    /**
     * @return mApicalLineTensionParameter
     */
    double GetApicalLineTensionParameter() const;

    /**
     * @return mBoundaryLineTensionParameter
     */
    double GetBoundaryLineTensionParameter() const; ///\todo do we really need this? #2850

    /**
     * @return mBasalSurfaceEnergyParameter
     */
    double GetBasalSurfaceEnergyParameter() const;

    /**
     * Set mVolumeCompressibilityParameter.
     *
     * @param volumeCompressibilityParameter the new value of mVolumeCompressibilityParameter
     */
    void SetVolumeCompressibilityParameter(double volumeCompressibilityParameter);

    /**
     * Set mLateralSurfaceEnergyParameter.
     *
     * @param lateralSurfaceEnergyParameter the new value of mLateralSurfaceEnergyParameter
     */
    void SetLateralSurfaceEnergyParameter(double lateralSurfaceEnergyParameter);

    /**
     * Set mApicalLineTensionParameter.
     *
     * @param apicalLineTensionParameter the new value of mApicalLineTensionParameter
     */
    void SetApicalLineTensionParameter(double apicalLineTensionParameter);

    /**
     * Set mBasalSurfaceEnergyParameter.
     *
     * @param basalSurfaceEnergyParameter the new value of mBasalSurfaceEnergyParameter
     */
    void SetBasalSurfaceEnergyParameter(double basalSurfaceEnergyParameter);

    /**
     * Set mTargetVolume.
     *
     * @param targetVolume the new value of mTargetVolume
     */
    void SetTargetVolume(double targetVolume);

    /**
     * Overridden OutputForceParameters() method.
     *
     * @param rParamsFile the file stream to which the parameters are output
     */
    void OutputForceParameters(out_stream& rParamsFile);
};

// Declare identifier for the serializer
#include "SerializationExportWrapper.hpp"
CHASTE_CLASS_EXPORT(MisraForce)

#endif /*MISRAFORCE_HPP_*/