#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ImageUtils {

constexpr double PI = 3.14159265358979323846;

// Helper to get grayscale value
inline uint8_t GetGrayscale(const uint8_t* pixel, int channels) {
    if (channels >= 3) {
        return static_cast<uint8_t>((pixel[0] * 299 + pixel[1] * 587 + pixel[2] * 114) / 1000);
    }
    return pixel[0];
}

// The standalone pHash generator
std::string PHashFromRawPixels(const uint8_t* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 0 || channels <= 0) return "";

    const int SIZE = 32;       // Scale to 32x32
    const int HASH_SIZE = 8;   // We only want the top-left 8x8 of the DCT

    // 1. Box-Sample Downscale to 32x32 Grayscale
    std::vector<double> img32(SIZE * SIZE, 0.0);

    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            int startX = (x * width) / SIZE;
            int endX   = ((x + 1) * width) / SIZE;
            int startY = (y * height) / SIZE;
            int endY   = ((y + 1) * height) / SIZE;

            double sum = 0;
            int count = 0;
            for (int py = startY; py < endY; ++py) {
                for (int px = startX; px < endX; ++px) {
                    int srcIdx = (py * width + px) * channels;
                    sum += GetGrayscale(&data[srcIdx], channels);
                    count++;
                }
            }
            if (count > 0) {
                img32[y * SIZE + x] = sum / count;
            }
        }
    }

    // 2. Precompute Cosine tables for massive speed boost
    // Formula: cos(PI * u * (2x + 1) / (2 * SIZE))
    std::vector<std::vector<double>> cosTable(HASH_SIZE, std::vector<double>(SIZE));
    for (int u = 0; u < HASH_SIZE; ++u) {
        for (int i = 0; i < SIZE; ++i) {
            cosTable[u][i] = std::cos(PI * u * (2.0 * i + 1.0) / (2.0 * SIZE));
        }
    }

    // 3. Compute 2D DCT (Only for the top-left 8x8 block!)
    // By restricting u and v to HASH_SIZE, we avoid 90% of the useless math
    std::vector<double> dctBlock(HASH_SIZE * HASH_SIZE, 0.0);

    for (int u = 0; u < HASH_SIZE; ++u) {
        for (int v = 0; v < HASH_SIZE; ++v) {
            double sum = 0.0;
            for (int i = 0; i < SIZE; ++i) {
                for (int j = 0; j < SIZE; ++j) {
                    sum += img32[i * SIZE + j] * cosTable[u][i] * cosTable[v][j];
                }
            }

            // Apply Orthogonal DCT-II Scaling
            double cu = (u == 0) ? std::sqrt(1.0 / SIZE) : std::sqrt(2.0 / SIZE);
            double cv = (v == 0) ? std::sqrt(1.0 / SIZE) : std::sqrt(2.0 / SIZE);
            dctBlock[u * HASH_SIZE + v] = cu * cv * sum;
        }
    }

    // 4. Extract the 64 DCT values, skipping the DC component at [0,0]
    std::vector<double> dctValues;
    dctValues.reserve(HASH_SIZE * HASH_SIZE - 1);
    for (int i = 0; i < HASH_SIZE * HASH_SIZE; ++i) {
        if (i != 0) { // Skip [0,0] so brightness doesn't affect the hash
            dctValues.push_back(dctBlock[i]);
        }
    }

    // 5. Calculate Median
    std::vector<double> sortedValues = dctValues;
    std::sort(sortedValues.begin(), sortedValues.end());
    double median = sortedValues[sortedValues.size() / 2];

    // 6. Build the 64-bit Hash
    uint64_t hashVal = 0;
    for (int i = 0; i < HASH_SIZE * HASH_SIZE; ++i) {
        // Set the bit if the frequency is above the median
        if (dctBlock[i] > median) {
            // We construct it bit-by-bit into a 64-bit unsigned integer
            hashVal |= (1ULL << (63 - i));
        }
    }

    // 7. Format as 16-character Hex String
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << hashVal;
    return oss.str();
}
}
