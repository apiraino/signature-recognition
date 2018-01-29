#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>

#include <wand/MagickWand.h>

void ThrowWandException(MagickWand *wand)
{
    char *description;
    ExceptionType severity;

    description = MagickGetException(wand, &severity);
    printf("%s %s %lu %s\n", GetMagickModule(), description);
    description = (char *) MagickRelinquishMemory(description);
}

size_t createTiff(char *input_file, char *out_file, double resolution)
{
    // convert to TIFF CCITT Group 4, 150 pixels/inch
    // convert -density 150 -compress Group4 -alpha off

    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, input_file);
    if (status == MagickFalse)
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    int old_w = MagickGetImageWidth(wand);
    int old_h = MagickGetImageHeight(wand);
    printf("Image width=%d height=%d\n", old_w, old_h);

    double res_x, res_y;
    if (MagickFalse == MagickGetImageResolution(wand, &res_x, &res_y))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    printf("Image res_x=%f res_y=%f\n", res_x, res_y);

    float pdf_width = old_w / res_x;
    float pdf_height = old_h / res_y;
    printf("PDF is width=%f height=%f\n", pdf_width, pdf_height);

    // remove alpha channel
    if (MagickFalse == MagickSetImageAlphaChannel(wand, DeactivateAlphaChannel))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    // resize based on new resolution
    double new_w = pdf_width * resolution;
    double new_h = pdf_height * resolution;
    printf("Resize to width=%f height=%f with new resolution=%f\n", new_w, new_h, resolution);
    if (MagickFalse == MagickResizeImage(wand, new_w, new_h, LanczosFilter, 1.0))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    printf("set image units\n");
    if (MagickFalse == MagickSetImageUnits(wand, PixelsPerInchResolution))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    printf("set resolution\n");
    if (MagickFalse == MagickSetImageResolution(wand, resolution, resolution))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    if (MagickFalse == MagickSetImageFormat(wand, "TIFF"))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    // compress
    printf("Compress\n");
    if (MagickFalse == MagickSetImageCompression(wand, Group4Compression))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    printf("strip junk and save\n");
    if (MagickFalse == MagickStripImage(wand))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    if (MagickFalse == MagickWriteImage(wand, out_file))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    if (wand)
        DestroyMagickWand(wand);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    char *in_file = NULL;
    char *out_file = NULL;
    double resolution = .0;
    char opt;
    while ((opt=(char)(getopt(argc, argv, "i:o:r:"))) != EOF) {
        switch (opt) {
        case 'i':
            in_file = optarg;
            break;
        case 'o':
            out_file = optarg;
            break;
        case 'r':
             resolution = strtod(optarg, NULL);
            break;
        default:
            printf("\nUsage:\n");
            printf(" -i PDF input file\n");
            printf(" -o PDF output file\n");
            printf(" -r resolution\n");
            printf("\n");
            return EXIT_FAILURE;
        }
    }

    return createTiff(in_file, out_file, resolution);
}
