# ffTES
Implementation of Time Entropy Signature (TES) algorithm on audio files using ffmpeg to decode various file formats.

The output of the algorithm is a digital signature, which can be used for:

 * Comparing two signatures of two different audio files and determine the degree of similarity between them.
 * You can run it on your audio files collection and find all the duplicates.
 
You can use normalized Hamming distance between the two signatures to determine how similar they are.

## Example
Two signatures of the same music file decoded in different audio formats, flac and mp3 is:

7BC643685693BDC13C033ADEC37891F3BDCCE739C9D695AD68810 
7BC643685613BDC13C033ADEC37891F3BDCCE739C9D695AD68810

Normalized Hamming distance equals to: **0.004785** 
As you can see the distance value is relatively small, so we can determine that two audio files are identical. Comparing two random files of the same length will tend to produce a normalized distance close to 0.5 since 50% of the signature bits will differ for random inputs.

Run sample script:

    ./runSample.sh

## FFmpeg
The library depended on FFmpeg open source project in order to decode the input audio files. To build the library, for example, on a debian based system you have to install FFmpeg development version like this:

    sudo apt-get install libavformat-dev

*please ignore deprecated compilation warnings*
