#pragma hdrstop // Mantido se estiver usando C++Builder, caso contrário pode remover
#include "complex_object.h"
#include <cstring> // for memcpy
#include <vector>
#include <stdexcept> // for std::runtime_error
#include <cmath>

#pragma package(smart_init) // Mantido se estiver usando C++Builder

//---------------------------------------------------------------------------
// Class TComplexObject
//---------------------------------------------------------------------------

void TComplexObject::InvalidateSerializedBuffer() {
    if (Serialized != nullptr) {
        delete[] Serialized;
        Serialized = nullptr;
    }
}

/**
* Returns the serialized version of this object.
* This method is required by stObject interface.
*/
const uint8_t* TComplexObject::Serialize() {
    // Check if the serialized version already exists and is valid
    if (Serialized == nullptr) {
        // Allocate buffer for the serialized data
        size_t totalSize = GetSerializedSize();
        Serialized = new uint8_t[totalSize];
        uint8_t* currentPos = Serialized;

        // --- Serialization Order ---
        // Resolution (int), Data Size (size_t), Label Length (size_t),
        // Label (char*), Data (double*)

        // 1. Resolution
        memcpy(currentPos, &Resolution, sizeof(int));
        currentPos += sizeof(int);

        // 2. Data Size (number of elements in the vector)
        size_t dataSize = Data.size();
        memcpy(currentPos, &dataSize, sizeof(size_t));
        currentPos += sizeof(size_t);

        // 3. Label Length
        size_t labelLen = Label.length();
        memcpy(currentPos, &labelLen, sizeof(size_t));
        currentPos += sizeof(size_t);

        // 4. Label data (variable length string)
        if (labelLen > 0) {
             memcpy(currentPos, Label.c_str(), labelLen);
             currentPos += labelLen;
        }

        // 5. Data vector elements (variable length array of doubles)
        if (dataSize > 0) {
             size_t dataBytes = dataSize * sizeof(double);
             memcpy(currentPos, Data.data(), dataBytes);
             // currentPos increment not needed as it's the last element
             // currentPos += dataBytes;
        }

       // Self-check: Did we write the expected number of bytes?
       // size_t bytesWritten = currentPos - Serialized;
       // if (bytesWritten != totalSize) {
       //     // This case should ideally recalculate based on what was *actually* written
       //     // or handle error more robustly
       //     delete[] Serialized;
       //     Serialized = nullptr;
       //     throw std::runtime_error("Serialization size mismatch.");
       // }

    } // end if Serialized == nullptr

    return Serialized;
}

int TComplexObject::GetResolutionSerial(const uint8_t* data, size_t datasize) {
    // Invalidate any existing owned serialized buffer first
    if (Serialized != nullptr) {
        delete[] Serialized;
        Serialized = nullptr; // Mark as invalid
    }

    int res;
    memcpy(&res, data, sizeof(int));
    return res;
}

/**
* Rebuilds a serialized object.
* This method is required by stObject interface.
*/
void TComplexObject::Unserialize(const uint8_t* data, size_t datasize) {
    // Invalidate any existing owned serialized buffer first
    if (Serialized != nullptr) {
        delete[] Serialized;
        Serialized = nullptr; // Mark as invalid
    }

    const uint8_t* currentPos = data;
    size_t remainingSize = datasize;

    // --- Deserialization Order (must match Serialize) ---
    // Resolution, Data Size, Label Length, Label, Data

    // Check minimum size before reading fixed parts
    size_t minFixedSize = sizeof(int) + sizeof(size_t) * 2;
    if (remainingSize < minFixedSize) {
        throw std::runtime_error("Insufficient data for TComplexObject Unserialize (fixed fields).");
    }

    // 1. Resolution
    memcpy(&Resolution, currentPos, sizeof(int));
    currentPos += sizeof(int);
    remainingSize -= sizeof(int);

    // 2. Data Size (number of elements)
    size_t dataSize;
    memcpy(&dataSize, currentPos, sizeof(size_t));
    currentPos += sizeof(size_t);
    remainingSize -= sizeof(size_t);

    // 3. Label Length
    size_t labelLen;
    memcpy(&labelLen, currentPos, sizeof(size_t));
    currentPos += sizeof(size_t);
    remainingSize -= sizeof(size_t);

    // Check if remaining size matches expected variable parts size
    size_t dataVecBytes = dataSize * sizeof(double);
    if (remainingSize < (labelLen + dataVecBytes)) {
         throw std::runtime_error("Insufficient data for TComplexObject Unserialize (variable fields size mismatch).");
    }
    // Can also check if remainingSize == (labelLen + dataVecBytes) for exact match

    // 4. Label
    if (labelLen > 0) {
        Label.assign(reinterpret_cast<const char*>(currentPos), labelLen);
        currentPos += labelLen;
        remainingSize -= labelLen;
    } else {
        Label.clear(); // Ensure label is empty if length was 0
    }

    // 5. Data vector
    Data.resize(dataSize); // Resize vector to hold the incoming data
    if (dataSize > 0) {
        // Double check remaining size is sufficient (though checked above)
        if (remainingSize < dataVecBytes) {
             throw std::runtime_error("Insufficient data for TComplexObject Unserialize (Data vector).");
        }
        memcpy(Data.data(), currentPos, dataVecBytes);
        // currentPos += dataVecBytes; // Not needed for last element read
        // remainingSize -= dataVecBytes;
    }

    // Object state is now updated. Keep Serialized as nullptr
    // until Serialize() is called again.
}

void TComplexObject::dataCompression(const int lvlCompress) {
    if (lvlCompress == 0) {
        return; // No operation needed
    }

    // Invalidate serialized buffer as data/resolution will change
    InvalidateSerializedBuffer();

    if (lvlCompress > 0) {
        // Call internal compression helper
        DoCompression(lvlCompress);
    } else {
        // Call internal decompression helper with positive level count
        DoDecompression(std::abs(lvlCompress));
    }
}

// --- Private Helper: DoCompression ---
void TComplexObject::DoCompression(const int levels) {
    if (levels <= 0) return; // Sanity check
    if (Data.empty()) return; // Cannot compress empty data

    std::vector<double> tempBuffer(Data.size()); // Temporary buffer for results
    int currentLevel = 0;

    // Loop for the requested number of levels or until max compression is reached
    while (currentLevel < levels) {
        // Use double for calculation involving pow to avoid potential integer truncation
        double currentApproxSizeF = static_cast<double>(Data.size()) / pow(2.0, static_cast<double>(Resolution));
        size_t approxSize = static_cast<size_t>(currentApproxSizeF);

        // Check if further compression is possible
        if (approxSize <= 1) {
            break; // Cannot reduce further
        }

        // Ensure approxSize is even for pairing (should be if original size was power of 2 based)
        // This implicitly handles non-power-of-2 sizes by stopping compression early.
        if (approxSize % 2 != 0) {
             // Or log a warning: "Data size at current resolution is odd, stopping compression."
            break;
        }

        size_t nextApproxSize = approxSize / 2; // Size after one more compression step

        // 1. Calculate new approximations and details
        for (size_t i = 0; i < nextApproxSize; ++i) {
            double val1 = Data[2 * i];
            double val2 = Data[(2 * i) + 1];
            tempBuffer[i] = (val1 + val2) / 2.0; // Approximation
            tempBuffer[i + nextApproxSize] = (val1 - val2) / 2.0; // Detail
        }

        // 2. Copy untouched detail coefficients from previous levels (if any)
        // These start after the currently processed block (indices from approxSize up to Data.size())
        for (size_t i = approxSize; i < Data.size(); ++i) {
            tempBuffer[i] = Data[i];
        }

        // 3. Update Resolution and copy data back from buffer
        Resolution++;
        // Use std::vector's move assignment for potential efficiency
        Data = std::move(tempBuffer);
        // Re-allocate tempBuffer for the next iteration if needed (or resize)
        // Since Data size doesn't change, we can just reuse the buffer by swapping back
        // Or just create a new one each loop if clarity is preferred and size isn't huge
        tempBuffer.resize(Data.size()); // Ensure size is correct if we were to reuse


        currentLevel++;
    }
    // Serialized buffer was invalidated at the start of dataCompression
}

// --- Private Helper: DoDecompression ---
void TComplexObject::DoDecompression(const int levels) {
    if (levels <= 0) return; // Sanity check
    if (Data.empty() || Resolution == 0) {
         // Cannot decompress empty data or data at base resolution
        return;
    }

    std::vector<double> tempBuffer(Data.size()); // Temporary buffer
    int currentLevel = 0;

    // Loop for the requested number of levels or until resolution is 0
    while (currentLevel < levels && Resolution > 0) {

        // Size of the approximation part at the *current* resolution (before decompression)
        double currentApproxSizeF = static_cast<double>(Data.size()) / pow(2.0, static_cast<double>(Resolution));
        size_t approxSize = static_cast<size_t>(currentApproxSizeF);

        // Check for potential issues from pow or non-integer results
         if (approxSize == 0 && !Data.empty()) {
             // Should not happen if Resolution > 0, but check anyway
             // Log warning: "Approximation size is zero during decompression, stopping."
             break;
         }
         // Check if approxSize * 2 would exceed original data size (corruption?)
         if (approxSize > Data.size() / 2) {
              // Log warning: "Calculated approximation size seems too large, stopping decompression."
             break;
         }


        // Size of the data part that will be reconstructed in this step
        size_t reconstructSize = approxSize * 2;

        // 1. Reconstruct data using approximations and details
        for (size_t i = 0; i < approxSize; ++i) {
            double approx = Data[i];
            // Details are stored right after approximations
            // Add boundary check for safety, although logic implies it should be fine
            if ((i + approxSize) >= Data.size()) {
                 throw std::runtime_error("Data index out of bounds during decompression (detail coeff).");
            }
            double detail = Data[i + approxSize];

            // Check indices for write access
            if ((2 * i + 1) >= tempBuffer.size()) {
                 throw std::runtime_error("Buffer index out of bounds during decompression (write access).");
            }

            tempBuffer[2 * i]     = approx + detail;
            tempBuffer[2 * i + 1] = approx - detail;
        }

        // 2. Copy untouched detail coefficients from previous levels (if any)
        // These start after the block containing the details we just used (indices from reconstructSize up to Data.size())
        for (size_t i = reconstructSize; i < Data.size(); ++i) {
             // Check indices for read/write access
             if (i >= Data.size() || i >= tempBuffer.size()) {
                 throw std::runtime_error("Index out of bounds during decompression (copying details).");
             }
            tempBuffer[i] = Data[i];
        }

        // 3. Update Resolution and copy data back from buffer
        Resolution--;
        Data = std::move(tempBuffer); // Replaces Data's content
         // Re-allocate or resize tempBuffer if loop continues
        tempBuffer.resize(Data.size());


        currentLevel++;
    }
     // Serialized buffer was invalidated at the start of dataCompression
}


//---------------------------------------------------------------------------
// Output operator
//---------------------------------------------------------------------------
/**
* Writes a string representation of a TComplexObject to an output stream.
*/
std::ostream& operator<<(std::ostream& out, const TComplexObject& obj) {
    out << "[Object Label=" << obj.GetLabel()
        << "; Res=" << obj.GetResolution()
        << "; DataSize=" << obj.GetData().size() << "]";
    return out;
}