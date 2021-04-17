# kwz-restoration

In the process of Nintendo's Flipnote Hatnea to Flipnote Gallery World conversion to the Flipnote Studio 3D .kwz format, the audio was improperly encoded in to the new format. Nintendo appears to not have reset the decoder variables between conversions of files, so some files have [extremely distorted audio](https://twitter.com/AustinSudomemo/status/1220367326085832704?s=20) using the default intial decoder state. This program finds proper initial step index and predictor decoder value in order to get the best possible value

Note: step index/predictor naming is from the [IMA ADPCM standard](http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf)

# Compilation

`g++ kwz-restoration.cpp -O3 -o kwz-restoration`

# Usage

`./kwz-restoration [input .kwz file path] [optional: output .wav file path]`

By specifying an output .wav file path, the audio with corrected audio will be written to that file.

# Restoration Process

 - Decode the track with the step index from 0 to 79 ([clamping range](https://github.com/Flipnote-Collective/flipnote-studio-3d-docs/wiki/kwz-format#sound-data)) and with the predictor as 0 (default initial decoder state)
   - The track must be converted fully for all 80 step index values because the impact of an improper step index is extremely subtle in a short period, however when the entire track is decoded the increased values are easily caught by the next step:
 
 - Calculate the RMS of the decoded track:
   - Square root(the sum of (each sample squared) divided by the number of samples)
 
 - The step index with the lowest RMS is the correct step index

 - Decode the track again with the step index found above and a predictor of zero

 - Find the mean (average) of the track
   - Add all samples together and divide by the number of samples

 - Divide the mean by -16 and round (not int floor/cieling, traditional rounding to a whole number)

 - That is your correct predictor

 - Decode the track with these values

# Issues

 - Step index range of 0-79 appears to be too big. Notes >50 appear to have a spike at the very beginning and then have the lowest overall RMS. 
   - Possible solution: reduce range to 0-50
     - Doesn't seem like a complete solution, need to investigate further the root cause 
