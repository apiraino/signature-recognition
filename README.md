## Signature recognition

Signature recognition workflow from a PDF file. This is the poorman's version of an OMR (Optimal Mark Recognition).

## How does it work?

- PDF file in input, converted into a TIFF G4 (b/n)
- Crop regions from the TIFF (using a text file with coordinates) into a BMP
- The BMP file is passed to `potrace` and converted into SVG
- The SVG paths are analyzed using `svg_tools`
- Estimate whether there's a "signature" or not. The decision is based on the quality of the SVG paths found.

n.b. All this crazy TIFF > BMP > SVG conversion pipeline is due to ImageMagick not supporting SVG as output backend [unless you compile Imagemagick with autotrace support](https://www.imagemagick.org/Usage/draw/#svg_output) (good luck) and `potrace` not usable as a library (AFAIK for the sole purpose of converting into SVG).

## Components

A C executable (`signature_crop/omr.c`) that:
- extracts the first page from a PDF (optionally saves an intermediate black/white TIFF CCIT Group4)
- Crops from the extracted page some regions and saves them as BMP. These regions are defined as [x,y,w,h] coordinates in a txt file loaded at runtime

Please refer to `signature_crop/README.md` for compiling the C source code.

A Python script that triggers `potrace` and converts the BMP files into SVG, then runs the SVG file analysis.

## Requirements

- Python 3.x
- ImageMagick (for PDF and image processing)
- GhostScript
- potrace

## Build and usage

### The signature analyzer

See also `signature_crop/README.md`

`$ pip install -e .`

`$ signature-recognition run_evaluation <pdf_dir> <zone_data.txt>`

### Build wheel

`$ signature-recognition build`

## Notes

This is an overly complicated tool that has few practical purposes.

Flaws in this approach:
- Too many microcomponents
- Region cropping at fixed coordinates is limiting, there should be some image resampling, fix orientation, etc.
- Exporting a TIFF from a PDF at fixed resolution could produce an image of insufficient quality to have clean SVG paths. One can attempt to increase the export resolution (e.g. from 150 to 200 or 300 dpi), but ...
- ... changing export resolution will invalidate the cropping coordinates in your data file
- My SVG analysis parameters are vague

Also, image cropping to extract the BMP is wasteful (source file is openend for every crop)

A better approach can be implemented using a machine learning approach (e.g. TensorFlow).

Anyway this was an interesting experiment to learn something new about different tools.
