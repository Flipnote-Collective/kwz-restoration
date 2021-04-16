#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <math.h>
#include <chrono>

#include "kwz-restoration.hpp"

void readFile(std::string path) {
    std::ifstream file(path, std::ios::binary);
    
    if (file) {
        // Stop eating newlines and white space in binary mode
        file.unsetf(std::ios::skipws);

        file.seekg(0, std::ios::end);
        std::streamoff file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        file_buffer.reserve(file_size);
        file_buffer.insert(file_buffer.begin(),
                           std::istream_iterator<uint8_t>(file),
                           std::istream_iterator<uint8_t>());

        file.close();
    }
    else {
        std::cout << "Failed to read file. " << std::endl;
        exit(-1);
    }
}

double findRMS(std::vector<int16_t> t_input) {
    double rms = 0.0;

    // Square all values and add together
    for (int i = 0; i < (int)t_input.size(); i++) {
        rms += std::pow(t_input[i], 2);
    }
    
    // Get the square root of the sum of the squares divided by the size of the vector
    return std::sqrt(rms / (double)t_input.size());
}

double findMean(std::vector<int16_t> t_input) {
    double mean = 0.0;

    // Add all values together
    for (int i = 0; i < (int)t_input.size(); i++) {
        mean += t_input[i];
    }
    
    // Divide the sum of all values by the number of values
    return mean / (double)t_input.size();
}

uint32_t getUint32(int pos) {
    return *reinterpret_cast<uint32_t*>(file_buffer.data() + pos);
}

void getKSNMeta() {
    // Find sound section ("KSN" magic) offset
    // Traverse sections using sizes at the end of each header
    int offset = 0;

    while (offset < (int)file_buffer.size()) {
        if (file_buffer[offset + 1] == 'S' && file_buffer[offset + 2] == 'N') {
            ksn_offset = offset;
            break;
        }
        offset += getUint32(offset + 4) + 8;
    }
    offset += getUint32(offset - 4);

    bgm_size = getUint32(ksn_offset + 0xC);
    bgm_offset = ksn_offset + 0x24;
}

std::vector<int16_t> decodeTrack(int track_size, int track_offset, int step_index, int16_t predictor) {
    // https://github.com/Flipnote-Collective/flipnote-studio-3d-docs/wiki/kwz-format#ksn-sound-data

    std::vector<int16_t> output;

    int16_t sample = 0;
    int16_t step = 0;
    int16_t diff = 0;

    int bit_pos = 0;
    int8_t byte = 0;
    
    for (int buffer_pos = track_offset; buffer_pos <= (track_offset + track_size); buffer_pos++) {
        byte = file_buffer[buffer_pos];
        bit_pos = 0;

        while (bit_pos < 8) {
            if (step_index < 18 || bit_pos > 4) {
                // 2 bit sample
                sample = byte & 0x3;

                step = adpcm_step_table[step_index];
                diff = step >> 3;

                if (sample & 1) diff += step;
                if (sample & 2) diff = -diff;

                predictor += diff;
                step_index += adpcm_index_table_2_bit[sample];

                byte >>= 2;
                bit_pos += 2;
            }
            else {
                // 4 bit sample
                sample = byte & 0xF;

                step = adpcm_step_table[step_index];
                diff = step >> 3;

                if (sample & 1) diff += step >> 2;
                if (sample & 2) diff += step >> 1;
                if (sample & 4) diff += step;
                if (sample & 8) diff = -diff;

                predictor += diff;
                step_index += adpcm_index_table_4_bit[sample];

                byte >>= 4;
                bit_pos += 4;
            }

            // Clamp step index and predictor
            step_index = clampValue(step_index, 0, 79);
            predictor = clampValue((int)predictor, -2048, 2047);
            
            // Add to output buffer
            output.push_back(predictor * 16);
        }
    }

    return output;
}

int main(int argc, char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Read arguments
    if (argc > 2) {
        std::cout << "Too many arguments passed!" << std::endl;
        exit(-1);
    }
    else if (argc == 2) {
        readFile(argv[1]);
    }
    else {
        std::cout << "Too few paramters! " << std::endl;
        exit(-1);
    }

    // Quickly verify file is a valid KWZ file
    if (file_buffer[0] == 'K') {
        // Get location and size of the BGM track
        getKSNMeta();

        // Verify that the BGM track exists. If it does then begin the process.
        if (bgm_size == 0) {
            std::cout << "File does not have BGM track." << std::endl;
            exit(-1);
        }
        else {
            // Find the RMS value for the decoded track using every step index
            for (auto i = 0; i < 80; i++) {
                step_index_rms[i] = findRMS(decodeTrack(bgm_size, bgm_offset, i, 0));
            }

            // Find the lowest RMS value step index
            for (auto i = 0; i <= 79; i++) {
                if (step_index_rms[i] < least_rms) {
                    least_rms = step_index_rms[i];
                    least_rms_i = i;
                }
            }

            std::cout << "Step index: " << least_rms_i << std::endl;

            // Find the correct initial predictor
            result_predictor = std::round(findMean(decodeTrack(bgm_size, bgm_offset, least_rms_i, 0)) / -16);

            std::cout << "Predictor: " << result_predictor << std::endl;
        }
    }
    else {
        std::cout << "File is invalid! " << std::endl;
        exit(-1);
    }
    
    std::cout << std::endl << "Total execution time: " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count() << " microseconds." << std::endl;
}
