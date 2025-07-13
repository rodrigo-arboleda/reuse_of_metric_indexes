#ifndef COMPLEX_OBJECT_H
#define COMPLEX_OBJECT_H

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept> // Para std::runtime_error
#include <cstring>   // Para memcpy
#include <ostream>
#include <limits>    // Para std::numeric_limits (se necessário para comparações futuras)
#include <vector>    // Necessário para std::vector::operator==

// Metric Tree includes (Removido stUtil.h se não for mais necessário,
// mas mantendo caso a estrutura ainda seja usada externamente)
// #include <arboretum/stUtil.h> // Comente ou remova se não usar mais

//---------------------------------------------------------------------------
// Class TComplexObject
//---------------------------------------------------------------------------
/**
* This class abstracts a complex data object containing a label,
* a resolution value, and a vector of double-precision floating-point data.
*
* It implements the stObject interface for compatibility with metric trees.
*
* Required stObject methods:
* - TComplexObject() - Default constructor.
* - Clone() - Creates a clone of this object.
* - IsEqual() - Checks if this instance is equal to another (based on resolution and data vector).
* - GetSerializedSize() - Gets the size of the serialized version.
* - Serialize() - Gets the serialized version.
* - Unserialize() - Restores a serialized object.
*
* The serialized format is:
* <CODE>
* +------------+-----------------+-----------------+-------------+--------+
* | Resolution | Data Size (sz_t)| Label Len (sz_t)| Label (str) | Data[] |
* +------------+-----------------+-----------------+-------------+--------+
* </CODE>
* Resolution is int. Data Size and Label Len are size_t.
* Label is a variable-length string. Data[] elements are doubles.
*
* @version 1.2
* @author Adaptado
*/
class TComplexObject {
public:
    /**
    * Default constructor. Required by stObject interface.
    * Initializes resolution to 0, label to empty, data vector to empty.
    */
    TComplexObject() :
        Resolution(0), Label(""), Serialized(nullptr) {
        // Data vector is default-initialized to empty
        // Serialized buffer is invalidated.
    }

    /**
    * Creates a new complex object.
    *
    * @param label A descriptive label for the object.
    * @param resolution An integer resolution value.
    * @param data A vector of double values representing the object's features.
    */
    TComplexObject(const std::string& label, int resolution, const std::vector<double>& data) :
        Label(label), Resolution(resolution), Data(data), Serialized(nullptr) {
        // Serialized buffer is invalidated.
    }

   /**
    * Copy constructor.
    */
    TComplexObject(const TComplexObject& other) :
        Label(other.Label), Resolution(other.Resolution), Data(other.Data), Serialized(nullptr)
    {
        // Invalidate potential serialized buffer copy - create own when needed
    }

   /**
    * Assignment operator.
    */
    TComplexObject& operator=(const TComplexObject& other) {
        if (this != &other) {
            Label = other.Label;
            Resolution = other.Resolution;
            Data = other.Data;

            // Invalidate and clean up old serialized buffer
            if (Serialized != nullptr) {
                delete[] Serialized;
                Serialized = nullptr;
            }
        }
        return *this;
    }

    /**
    * Destroys this instance and releases the serialized buffer if allocated.
    */
    virtual ~TComplexObject() {
        if (Serialized != nullptr) {
            delete[] Serialized;
        }
    }

    // --- Getters ---
    const std::string& GetLabel() const { return Label; }
    int GetResolution() const { return Resolution; }
    const std::vector<double>& GetData() const { return Data; }

    // --- stObject interface methods ---

    /**
    * Creates a perfect clone of this object. Required by stObject interface.
    * @return A new instance of TComplexObject identical to this one.
    */
    TComplexObject* Clone() const {
        return new TComplexObject(Label, Resolution, Data);
    }

    /**
    * Checks if this object is equal to another.
    * Equality is defined as having the same Resolution and identical Data vectors
    * (element by element). The Label is NOT considered for equality.
    * Required by stObject interface.
    *
    * @param obj Another instance pointer of TComplexObject.
    * @return True if resolution and data vectors are equal, false otherwise.
    */
    bool IsEqual(const TComplexObject* obj) const {
        if (!obj) return false; // Handle null pointer case
        // Compare resolution first (quick check)
        if (Resolution != obj->Resolution) {
            return false;
        }
        // Compare data vectors (checks size and elements)
        return (Data == obj->Data); // Uses std::vector::operator==
    }

    /**
    * Returns the size of the serialized version of this object in bytes.
    * Required by stObject interface.
    */
    size_t GetSerializedSize() const {
        // Fixed parts: Resolution (int), Data size (size_t), Label length (size_t)
        // Variable parts: Label (string chars), Data (vector elements)
        return sizeof(int) + (sizeof(size_t) * 2) +
               Label.length() + (Data.size() * sizeof(double));
    }

    int GetResolutionSerial(const uint8_t* data, size_t datasize);

    /**
    * Returns the serialized version of this object.
    * Required by stObject interface. Allocates buffer if not present.
    */
    const uint8_t* Serialize();

    /**
    * Rebuilds the object from its serialized form.
    * Required by stObject interface.
    *
    * @param data The serialized object data buffer.
    * @param datasize The size of the data buffer in bytes.
    */
    void Unserialize(const uint8_t* data, size_t datasize);

    void dataCompression(const int lvlCompress);

private:
    // --- Core Data Members ---
    std::string Label;
    int Resolution;
    std::vector<double> Data;

    /**
    * Serialized version buffer. NULL if not created or invalidated.
    * Managed internally by Serialize() and Unserialize().
    */
    uint8_t* Serialized;

        /**
     * @brief Invalidates the current serialized buffer.
     */
    void InvalidateSerializedBuffer();

    /**
     * @brief Internal implementation for Haar wavelet compression.
     * @param levels Number of levels to compress (must be > 0).
     */
    void DoCompression(const int levels);

    /**
     * @brief Internal implementation for inverse Haar wavelet transform (decompression).
     * @param levels Number of levels to decompress (must be > 0).
     */
    void DoDecompression(const int levels);
};

// --- Output Operator ---
/**
* Writes a string representation of a TComplexObject to an output stream.
*/
std::ostream& operator<<(std::ostream& out, const TComplexObject& obj);

#endif // COMPLEX_OBJECT_H