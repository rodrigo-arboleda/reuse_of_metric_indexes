#ifndef DISTANCE_CALCULATOR_H
#define DISTANCE_CALCULATOR_H

#include <vector>
#include <stdexcept>    // For std::runtime_error
#include <iostream>
#include <memory>
#include "complex_object.h" // Include the definition of the object type

// Include for the base class DistanceFunction
#include <hermes/DistanceFunction.h>

//---------------------------------------------------------------------------
// Class TComplexObjectDistanceEvaluator
//---------------------------------------------------------------------------
/**
* Calculates distance between TComplexObject instances based on their 'Data' vectors.
*
* This implementation uses the Manhattan distance between the Data vectors.
* It requires the Data vectors of both objects to have the same dimension.
*
*
* @version 1.0
* @author Adapted from TCityDistanceEvaluator
*/
class TComplexObjectDistanceEvaluator : public DistanceFunction<TComplexObject> {
public:
    TComplexObjectDistanceEvaluator() {}

    virtual ~TComplexObjectDistanceEvaluator() {} // Good practice for base classes

    /**
    * Calculates the Manhattan distance between the 'Data' vectors of two objects.
    * Throws std::runtime_error if data vectors have different dimensions.
    *
    * @param obj1 First TComplexObject.
    * @param obj2 Second TComplexObject.
    * @return The Manhattan distance.
    */
    virtual double GetDistance2(TComplexObject& obj1, TComplexObject& obj2) {

        // --- Section 1: Prepare Data Pointers and Handle Resolution Differences ---

        const int targetResolution = obj2.GetResolution();
        const std::vector<double>& data2_obj = obj2.GetData();

        // Pointer to the data vector of obj1 (or its temporary clone)
        const std::vector<double>* data1_ptr = &obj1.GetData();

        // Smart pointer to manage the lifetime of a temporary clone of obj1
        // Using unique_ptr ensures clone is deleted even if exceptions occur
        std::unique_ptr<TComplexObject> obj1_temp_clone_ptr = nullptr;

        int currentResolution = obj1.GetResolution();

        // Check if obj1 needs resolution adjustment
        if (currentResolution != targetResolution) {
            // Clone obj1 to avoid modifying the original object permanently
            // Clone returns a raw pointer, manage it with unique_ptr
             try {
                obj1_temp_clone_ptr.reset(obj1.Clone()); // Transfer ownership to unique_ptr
             } catch (const std::bad_alloc& e) {
                 throw std::runtime_error("Failed to allocate memory for obj1 clone.");
             }
            if (!obj1_temp_clone_ptr) {
                 throw std::runtime_error("Failed to clone obj1 for distance calculation (null result).");
            }

            // Check for fundamental size mismatch before transforming
            if (obj1_temp_clone_ptr->GetData().size() != data2_obj.size()) {
                 // No need to delete clone, unique_ptr handles it.
                 throw std::runtime_error("Objects have different underlying data sizes, cannot compare.");
            }

            // Calculate the transformation level needed for the clone
            // Positive = compress, Negative = decompress
            int levelsToTransform = targetResolution - currentResolution;

            // Apply the transformation *to the clone* using the public dataCompression method
            // This method now internally handles positive (compress) and negative (decompress) levels.
            obj1_temp_clone_ptr->dataCompression(levelsToTransform);

            // Verify that the transformation resulted in the target resolution
            if (obj1_temp_clone_ptr->GetResolution() != targetResolution) {
                // This might happen if transform couldn't complete all levels (e.g., hit res 0)
                // Depending on requirements, could throw or maybe compare at the achieved resolution?
                // Throwing is safer if exact resolution match is expected.
                 // unique_ptr handles deletion of clone.
                 throw std::runtime_error("Failed to adjust obj1 clone to target resolution. CloneRes="
                    + std::to_string(obj1_temp_clone_ptr->GetResolution()) + ", TargetRes=" + std::to_string(targetResolution));
            }

            // Update the pointer to use the transformed data from the clone
            data1_ptr = &obj1_temp_clone_ptr->GetData();

        } else {
             // Resolutions already match, still check for size compatibility
             if (obj1.GetData().size() != data2_obj.size()) {
                 throw std::runtime_error("Objects have different underlying data sizes, cannot compare.");
             }
             // data1_ptr already points to obj1.GetData() - no clone needed
        }

        // --- Section 2: Calculate Distance using Approximation Coefficients ---

        // Now, data1_ptr points to the data of obj1 (or its clone) at targetResolution
        // and data2_obj is the data of obj2 at targetResolution. Their underlying sizes match.

        // Calculate the number of approximation coefficients at the target resolution
        if (data2_obj.empty()) {
            updateDistanceCount(); // Count distance calc even for empty objects? Your choice.
            return 0.0; // Distance is 0 if objects are empty
        }
        if (!data1_ptr || data1_ptr->empty()) {
             // Should not happen if data2_obj is not empty and sizes match, but check
             updateDistanceCount();
             return 0.0;
        }


        // Use double for potentially large sizes or intermediate pow result
        double powerOfTwo = pow(2.0, static_cast<double>(targetResolution));
         // Basic checks for pow result
        if (powerOfTwo <= 0 || !std::isfinite(powerOfTwo)) {
             throw std::runtime_error("Invalid power calculation for resolution.");
        }
        // Avoid division by zero or near-zero
        if (powerOfTwo < 1.0) powerOfTwo = 1.0;


        // Calculate approximate size, ensuring it doesn't exceed actual vector size due to precision issues
        size_t vectorSize = data2_obj.size(); // Use common size
        size_t approxSize = static_cast<size_t>(static_cast<double>(vectorSize) / powerOfTwo);
        approxSize = std::min(approxSize, vectorSize); // Ensure not larger than total size


        if (approxSize == 0 && vectorSize > 0) {
             // Resolution might be too high for the data size
             // Log or handle as appropriate. Returning 0 might be misleading.
             // Consider throwing an error if comparison at this level is meaningless.
              std::cerr << "Warning: Resolution " << targetResolution
                       << " results in zero approximation coefficients for size "
                       << vectorSize << std::endl;
             updateDistanceCount();
             return 0.0; // Or throw std::runtime_error("Cannot compare at this resolution level.");
        }

        // Calculate the Manhattan distance using only the approximation coefficients
        double sumOfDiff = 0.0;
        for (size_t i = 0; i < approxSize; ++i) {
            // Access data using the pointer dereference (*data1_ptr)
            double diff = (*data1_ptr)[i] - data2_obj[i];
            sumOfDiff += std::abs(diff); // Use std::abs for double
        }

        updateDistanceCount(); // Update statistics counter from base class
        return sumOfDiff;
    }


    /**
    * Calculates the Euclidean distance between the 'Data' vectors of two objects.
    * This is the primary distance function.
    * Uses GetDistance2 internally for calculation.
    * Throws std::runtime_error if data vectors have different dimensions.
    *
    * @param obj1 First TComplexObject.
    * @param obj2 Second TComplexObject.
    * @return The Euclidean distance.
    */
    virtual double GetDistance(TComplexObject& obj1, TComplexObject& obj2) {
         // No need to call updateDistanceCount() here if GetDistance2 does it.
         return GetDistance2(obj1, obj2);
    }


    /**
     * Provided for compatibility if the original code used getDistance
     * directly in some places. Delegates to GetDistance.
     */
    double getDistance(TComplexObject& obj1, TComplexObject& obj2){
       return GetDistance(obj1, obj2);
    }

}; // end class TComplexObjectDistanceEvaluator

#endif // DISTANCE_CALCULATOR_H