### Signature recognition

Extracts SVG paths from a PDF and calculate a confidence value for signatures extracted.

### Compile the OMR module

On Ubuntu:

- sudo apt-get install:
  - `libmagickwand-dev` for image processing
  - `libgs-dev` for PDF processing (unused for now)
  - `potrace` to calculate SVG paths length

n.b. instead of `potrace` you can use [inkscape](https://inkscape.org) (shooting butterflies with a cannon) or [autotrace](http://autotrace.sf.net) (slightly worse rasterization quality in my tests).

#### OMR Compilation FLAGS:

- USE_LOCAL_TIFF: (for debugging purposes) generate an intermediate TIFF file at fixed resolution according to "-r" param, instead of using directly the PDF from input
- {GS_ENGINE, MAGICK_ENGINE, GIMP_ENGINE}: choose an engine to generate the intermediate TIFF file (only MAGICK_ENGINE is implemented)
- DEBUG: verbose output

#### OMR usage example:
- `sh clean_tmp_files.sh`
- `./omr -d samples/pdfs -z samples/data.txt`

#### Signature analysis (a.k.a. how does it all work?)

The signature regions are extracted from the PDF, saved into BMP files and then converted into vectorized SVG images.

These path are then analyzed on these basis:

    each path length is measured (in px) and assigned a quality value

        THRES_PATH_LEN_WARN = 700
        THRES_PATH_LEN_GOOD = 1100

        GOOD paths are long enough to qualify for a signature segment
        WARN paths are not so good
        BAD paths could be dirt or segments extracted in too low quality (e.g. try to increase DPI on BMP extraction)

    GOOD, WARN, BAD paths are then counted, resulting in three totals.

    We define some thresholds:

        THRES_NUM_GOOD_PATH = 3
        THRES_NUM_WARN_PATH = 10
        THRES_NUM_BAD_PATH = 40

    Rate the signature quality, see `omr._rate_signature()`; rating can be:

        RATING_GOOD
        RATING_BAD
        RATING_WARN
        RATING_UNKNOWN

    Assess whether the paths qualify or not as a signature, see `omr._is_signed()`:

        if
            GOOD == 0 and
            WARN <= THRES_NUM_WARN_PATH and
            BAD <= THRES_NUM_BAD_PATH
            return IS_NOT_SIGNED
        else
            return IS_SIGNED

    A confidence value is attached to this assessment using a [0.0 - 0.9] value. Base value is 0.5 (neutral).
    This value is increased or decreased according to the quality of the signature:

        RATING_GOOD:    CONFIDENCE += 0.3
        RATING_WARN:    CONFIDENCE -= 0.1
        RATING_BAD:     CONFIDENCE -= 0.2
        RATING_UNKNOWN: CONFIDENCE -= 0.3
