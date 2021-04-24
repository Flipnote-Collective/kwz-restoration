#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <math.h>

#include "kwz-restoration.hpp"

uint32_t getUint16(int pos) {
    return *reinterpret_cast<uint16_t*>(file_buffer.data() + pos);
}

uint32_t getUint32(int pos) {
    return *reinterpret_cast<uint32_t*>(file_buffer.data() + pos);
}

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

void writeWAV(std::string t_path, std::vector<int16_t> t_input) {
    std::ofstream output_file(t_path, std::ios::binary);
    
    // Generate and write WAV header
    wav_hdr wav;
    wav.chunk_size = (uint32_t)(t_input.size() + 36);
    wav.subchunk_2_size = (uint32_t)(t_input.size() * 2);
    output_file.write(reinterpret_cast<const char*>(&wav), sizeof(wav));

    // Write audio data
    output_file.write(reinterpret_cast<const char*>(&t_input[0]), t_input.size() * 2);

    output_file.close();
}

double findRMS(std::vector<int16_t> input) {
    double rms = 0.0;

    // Square all values and add together
    for (auto i = 0; i < (int)input.size(); i++) {
        rms += input[i] * input[i];
    }
    
    // Get the square root of the sum of the squares divided by the size of the vector
    return std::sqrt(rms / (double)input.size());
}

int16_t clampValue(int16_t value, int min, int max) {
    if (value < min) value = min;
    if (value > max) value = max;
    return value;
}

std::vector<int16_t> decodeTrack(int track_size, int track_offset, int step_index) {
    // https://github.com/Flipnote-Collective/flipnote-studio-3d-docs/wiki/kwz-format#ksn-sound-data

    std::vector<int16_t> output;

    int16_t predictor = 0;
    int16_t sample = 0;
    int16_t step = 0;
    int16_t diff = 0;

    int bit_pos = 0;
    int8_t byte = 0;
    
    for (auto buffer_pos = track_offset; buffer_pos <= (track_offset + track_size); buffer_pos++) {
        byte = file_buffer[buffer_pos];
        bit_pos = 0;

        while (bit_pos < 8) {
            if (step_index < 18 || bit_pos > 4) {
                // Decode 2 bit sample
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
                // Decode 4 bit sample
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
            predictor = clampValue(predictor, -2048, 2047);
            
            // Add to output buffer
            output.push_back(predictor * 16);
        }
    }

    return output;
}

void getKSNMeta() {
    // Find sound section ("KSN " magic) offset by traversing the sections of the file
    // using the section sizes at the end of each section header
    int offset = 0;

    while (offset < (int)file_buffer.size()) {
        if (file_buffer[offset + 1] == 'S' && file_buffer[offset + 2] == 'N') {
            break;
        }
        else {
            // Add the section size + the size of the section header itself
            offset += getUint32(offset + 4) + 8;
        }
    }

    // Get track sizes
    bgm_size = getUint32(offset + 0xC);
    se_1_size = getUint32(offset + 0x10);
    se_2_size = getUint32(offset + 0x14);
    se_3_size = getUint32(offset + 0x18);
    se_4_size = getUint32(offset + 0x1C);

    // Calculate track offsets from sizes
    bgm_offset = offset + 0x24;
    se_1_offset = bgm_offset + bgm_size;
    se_2_offset = se_1_offset + se_1_size;
    se_3_offset = se_2_offset + se_2_size;
    se_4_offset = se_3_offset + se_3_size;
}

bool verifyFile() {
    bool result = false;

    if (file_buffer[0] == 'K') {
        result = true;
        getKSNMeta();
    }
    else {
        std::cout << "File is not a valid .kwz file!" << std::endl;
    }

    return result;
}

int findCorrectStepIndex(int track_size, int track_offset) {
    int result = -1;

    if (track_size > 0) {
        double step_index_rms[41] = { 0 };
        double least_rms_value = 100000;  // Higher than highest possible RMS

        // Decode the BGM track using every step index from 0-40 and record the RMS of the track
        for (int i = 0; i < 41; i++) {
            step_index_rms[i] = findRMS(decodeTrack(track_size, track_offset, i));
        }

        // Find the lowest RMS value recorded, which is the correct step index
        for (int i = 0; i < 41; i++) {
            if (step_index_rms[i] < least_rms_value) {
                least_rms_value = step_index_rms[i];
                result = i;
            }
        }
    }

    return result;
}

void findAllStepIndexes() {
    int step_index = 0;

    std::cout << "Correct track step indexes:" << std::endl;
    std::cout << "Note: if none appear, there are no audio tracks in the file." << std::endl;

    step_index = findCorrectStepIndex(bgm_size, bgm_offset);
    if (step_index != -1) {
        std::cout << "BGM: " << step_index << std::endl;
        bgm_step_index = step_index;
    }

    step_index = findCorrectStepIndex(se_1_size, se_1_offset);
    if (step_index != -1) {
        std::cout << "SE1: " << step_index << std::endl;
    }

    step_index = findCorrectStepIndex(se_2_size, se_2_offset);
    if (step_index != -1) {
        std::cout << "SE2: " << step_index << std::endl;
    }
    
    step_index = findCorrectStepIndex(se_3_size, se_3_offset);
    if (step_index != -1) {
        std::cout << "SE3: " << step_index << std::endl;
    }

    step_index = findCorrectStepIndex(se_4_size, se_4_offset);
    if (step_index != -1) {
        std::cout << "SE4: " << step_index << std::endl;
    }
}

int main(int argc, char** argv) {
    // Read arguments
    if (argc > 3) {
        std::cout << "Too many arguments passed!" << std::endl;
    }
    // 1 or 2 arguments passed, assuming they are valid.
    else if (argc == 3 || argc == 2) {
        readFile(argv[1]);

        if (file_buffer[0] == 'K') {
            getKSNMeta();
            findAllStepIndexes();

            // If the user chooses, output track decoded with the correct initial step index to a WAV file.
            if (argc == 3 && bgm_size > 0) {
                std::cout << "Writing properly decoded BGM track to WAV file" << std::endl;
                 
                writeWAV(argv[2], decodeTrack(bgm_size, bgm_offset, bgm_step_index));
            }
            else if (bgm_size == 0) {
                std::cout << "There is no BGM audio to output!" << std::endl;
            }
        }
        else {
            std::cout << "File is not a valid .kwz file!" << std::endl;
        }
    }
    // argc < 2
    else {
        std::cout << "Too few arguments passed!" << std::endl;
    }
}
