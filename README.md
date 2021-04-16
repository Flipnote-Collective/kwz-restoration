# kwz-restoration

In the process of Nintendo's Flipnote Hatnea to Flipnote Gallery World conversion to the Flipnote Studio 3D .kwz format, the audio was improperly encoded in to the new format. Nintendo appears to not have reset the decoder variables between conversions of files, so some files have [extremely distorted audio](https://twitter.com/AustinSudomemo/status/1220367326085832704?s=20) using the default intial decoder state. This program finds proper initial step index and predictor decoder value in order to get the best possible value

# Restoration Process

 - Decode the track with the step index from 0 to 79 and with the predictor as 0 
 
 - Calculate the RMS of the decoded track:
   - Square root(the sum of (each sample squared) divided by the number of samples)
 
 - The step index with the lowest RMS is the correct step index

 - Decode the track again with the step index found above and a predictor of zero

 - Find the mean (average) of the track
   - Add all samples together and divide by the number of samples

 - Divide the mean by -16 and round (not int floor/cieling, traditional rounding to a whole number)

 - That is your correct predictor

 - Decode the track with these values

# Compilation

`g++ kwz-restoration.cpp -O3 -o kwz-restoration`

# Usage

`./kwz-restoration [input .kwz file path] [optional: output .wav file path]`

By specifying an output .wav file path, the audio with corrected audio will be written to that file.
