# kwz-restoration

![GIF of bart simpson lining up megaphones causing a sound blast, with KWZ audio related text added](media/kwz-kwz-audio.gif)

*(gif credit: https://github.com/J0w03L)*

This repo contains an explanation of why some Flipnote Studio 3D DSi Library .kwz files have distorted audio and an example implementation of the restoration process.

Notes:

- Decoder variable naming is from the [IMA ADPCM standard](http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf).

- The explanation and correction process here are not final. This is the best explanation of the issue and solution I can come up with given the current state of research into the DSi Library .kwz audio format.

- This repo will be actively updated as any major discoveries are made.


# Background
In the process of Nintendo's Flipnote Hatena to Flipnote Gallery World conversion to the Flipnote Studio 3D .kwz format, the audio was improperly encoded in to the new format. Nintendo appears to not have reset the step index variable between conversions of files or encodings of individual tracks, which caused some files or tracks have [extremely distorted audio](https://twitter.com/AustinSudomemo/status/1220367326085832704).


# Restoration Process
To decode an audio track properly, follow this procedure first. After that, decode the track with the resulting initial step index.

- Decode the track with the step index from 0 to 40 and with the predictor as 0
    - The track must be converted fully for all 40 step index values because the impact of an improper step index is extremely subtle in a short period, however when the entire track is decoded the increased values are easily caught by the next step:
    - The reasoning for the 0-40 range is that >40 trips the 4 bit detection flag too early, messing up the ordering of bit decoding for the rest of the track, followed by the usual incorrect step index causing gradually higher and higher amplitude tracks, which results in distortion.

- Calculate the RMS of the decoded track:
    - Square each sample then add together
    - Divide by the total number of samples
    - Take the square root of that value to get your RMS.

- The step index with the lowest RMS is the correct initial step index value.


# Example Program
This program finds proper initial step index value for all audio tracks in order to decode the best possible audio given the data in the converted .kwz files.

### Compilation
To compile, first install `build-essential` or equivalent (includes `gcc` and `make`, as well as required libraries for code compilation), then run:

```shell
make
```

Note: this Makefile is written for Linux, however the code is portable enough that `mingw` or Visual Studio shouldn't have issues compiling it.

### Usage
`./kwz-restoration [input .kwz file path] [optional: output .wav file path] [optional: output .wav file track index]`

By specifying an output .wav file path, the BGM track will be decoded with the correct initial step index then will be written to the file path specified.

Refer to the chart below for valid track indexes. If no index or an invalid index is specified, the BGM track (0) will be used for the output.

| Track Index | Description           |
|-------------|-----------------------|
| 0 (Default) | BGM, background music |
| 1           | Sound Effect 1 (A)    |
| 2           | Sound Effect 2 (X)    |
| 3           | Sound Effect 3 (Y)    |


# Credits
- Everyone mentioned in [flipnote.js acknowledgments](https://flipnote.js.org/pages/docs/acknowledgements.html)
