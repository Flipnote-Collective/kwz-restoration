# kwz-restoration

In the process of Nintendo's Flipnote Hatnea to Flipnote Gallery World conversion to the Flipnote Studio 3D .kwz format, the audio was improperly encoded in to the new format. Nintendo appears to not have reset the decoder variables between conversions of files, so some files have [extremely distorted audio](https://twitter.com/AustinSudomemo/status/1220367326085832704?s=20) using the default intial decoder state. This program finds proper initial step index value in order to decode best possible audio.

Note: step index naming is from the [IMA ADPCM standard](http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf)

# Compilation

### GCC

`g++ kwz-restoration.cpp -Ofast -o kwz-restoration`

# Usage

`./kwz-restoration [input .kwz file path] [optional: output .wav file path]`

By specifying an output .wav file path, the BGM track will be decoded with the found correct initial step index and will be written to the file path specific.

# Restoration Process

 - Decode the track with the step index from 0 to 40 and with the predictor as 0
   - The track must be converted fully for all 40 step index values because the impact of an improper step index is extremely subtle in a short period, however when the entire track is decoded the increased values are easily caught by the next step:
   - The reasoning for the 0-40 range is that >40 trips the 4 bit detection flag too early, screwing up the ordering of bit decoding for the rest of the track, followed by the usual incorrect step index causing gradually higher and higher amplitude tracks.
 
 - Calculate the RMS of the decoded track:
   - Square each sample then add together
   - Divide by the total number of samples
   - Take the square root of that value to get your RMS.
 
 - The step index with the lowest RMS is the correct step index
   - Predictor correction is now no longer needed due to <=40 step indexes being found to correctly decode the audio.

# Issues
 - Are there false positives in some notes?
   - Where multiple step index are close to each other in value, but slightly different.
     - Is lowest RMS still the best solution? Need to investigate further, possibly with comparison to PPM audio if it can be found. Otherwise, will have to assume lowest is best.
