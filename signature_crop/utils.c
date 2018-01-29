#include "common.h"
#include "utils.h"
#include "log.h"

#include <wand/MagickWand.h>

// Binary string repr from int
char *baseconv(unsigned int num, int base)
{
    static char retbuf[33]; retbuf[0] = '\0';
    char *p;

    if(base < 2 || base > 16)
        return NULL;

    p = &retbuf[sizeof(retbuf)-1];
    *p = '\0';

    do {
        *--p = "0123456789abcdef"[num % base];
        num /= base;
    } while(num != 0);

    return p;
}

size_t _createTiffGs(char *input_file, char *out_file, double resolution)
{
    // TODO: use the API
    // gs -dNOPAUSE -dBATCH -sDEVICE=tiffg4 -sOutputFile=page.tiff -dFirstPage=1 -dLastPage=1 -r150 -fsample.pdf
    int buflen = 200;
    char cmd[buflen]; cmd[0] = '\0';
    snprintf(cmd, buflen, "gs -dNOPAUSE -dBATCH -sDEVICE=tiffg4 -sOutputFile=%s -dFirstPage=1 -dLastPage=1 -r%d -f%s",
             out_file, (int)resolution, input_file);
    log_debug("GS cmd %s\n", cmd);
    if (EXIT_SUCCESS != system(cmd))
    {
        log_error("GS ERROR in executing cmd: %s\n", cmd);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void ThrowWandException(MagickWand *wand)
{
    // FIXME: FFS!
    return;
    char *description;
    ExceptionType severity;

    description = MagickGetException(wand, &severity);
    log_error("MAGICK %s %s %lu %s\n", GetMagickModule(), description);
    MagickRelinquishMemory(description);
}

size_t _createTiffMagick(char *input_file, char *out_file, double resolution)
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
    log_debug("MAGICK Image width=%d height=%d\n", old_w, old_h);

    double res_x, res_y;
    if (MagickFalse == MagickGetImageResolution(wand, &res_x, &res_y))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    log_debug("MAGICK Image res_x=%f res_y=%f\n", res_x, res_y);

    float pdf_width = old_w / res_x;
    float pdf_height = old_h / res_y;
    log_debug("MAGICK PDF is width=%f height=%f\n", pdf_width, pdf_height);

    // remove alpha channel
    if (MagickFalse == MagickSetImageAlphaChannel(wand, DeactivateAlphaChannel))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

    // resize based on new resolution
    double new_w = pdf_width * resolution;
    double new_h = pdf_height * resolution;
    log_debug("MAGICK Resize to width=%f height=%f with new resolution=%f\n", new_w, new_h, resolution);
    if (MagickFalse == MagickResizeImage(wand, new_w, new_h, LanczosFilter, 1.0))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
    if (MagickFalse == MagickSetImageUnits(wand, PixelsPerInchResolution))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }
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
    if (MagickFalse == MagickSetImageCompression(wand, Group4Compression))
    {
        ThrowWandException(wand);
        return EXIT_FAILURE;
    }

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
    log_debug("MAGICK Image %s written\n", out_file);

    if (wand)
        DestroyMagickWand(wand);

    return EXIT_SUCCESS;
}

size_t createTiff(char *input_file, char *out_file, double resolution)
{

#define MAGICK_ENGINE 1

#ifdef MAGICK_ENGINE
    return _createTiffMagick(input_file, out_file, resolution);
#endif

#ifdef GS_ENGINE
    return _createTiffGs(input_file, out_file, resolution);
#endif

#ifdef GIMP_ENGINE
    // not implemented and probably will never be
    return EXIT_FAILURE;
#endif

    log_debug("Got %s %s %f\n", input_file, out_file, resolution);
    log_error("Application is not compiled with an imaging engine!\n");
    log_error("Please compile with either -DMAGICK_ENGINE -DGS_ENGINE or -DGIMP_ENGINE\n");
    return EXIT_FAILURE;
}

char *get_dest_path(char *src_file, char *ext)
{
    char *tmp = strdup(src_file);
    char *bname = basename(tmp);
    char *path = "/tmp/_";
    // remove file extension
    char *extn = strrchr(bname, '.');
    if(extn != NULL)
        *extn = '\0';
    size_t len = strlen(path) + strlen(bname) + strlen(ext) + 1;
    char dest_str[len]; dest_str[0] = '\0';

    strncat(dest_str, path, strlen(path));
    strncat(dest_str, bname, strlen(bname));
    strncat(dest_str, ".", 1);
    strncat(dest_str, ext, strlen(ext));
    // FIXME: leak? Need to free(dest_real)
    char *dest_real = strdup(dest_str);
    free(tmp); tmp = NULL;
    return dest_real;
}

const char *_get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

size_t initDirs()
{
    /* int file_counter = 0; */
    /* int wrapper(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) */
    /* { */
    /*     printf("[%d] File %d in %s\n", file_counter, ftwbuf, fpath); */
    /*     file_counter++; */
    /*     return EXIT_SUCCESS; */
    /* } */

    /* struct stat st = {0}; */
    /* if (-1 != (errno = stat(BMP_DIR, &st))) */
    /* { */
    /*     if (EXIT_SUCCESS != (errno = remove(BMP_DIR))) */
    /*     { */
    /*         log_debug("Traversing %s\n", BMP_DIR); */
    /*         int flags = 0; */
    /*         nftw(BMP_DIR, wrapper, 200, flags); */
    /*         log_debug("Done Traversing %s\n", BMP_DIR); */
    /*         log_error("\nCould not remove dirtree %s: %d\n", BMP_DIR, errno); */
    /*     } */
    /* } */

    // FIXME: doh :-/
    int len = strlen(BMP_DIR);
    char cmd[7 + len + 1]; cmd[0] = '\0';
    snprintf(cmd, 7+len+1, "rm -rf %s", BMP_DIR);
    if (EXIT_SUCCESS != (errno = system(cmd)))
        log_error("\nCould not rm dir %s: %d\n", BMP_DIR, errno);

    if (EXIT_SUCCESS != (errno = mkdir(BMP_DIR, 0700)))
        log_error("\nCould not create %s: %d\n", BMP_DIR, errno);

    return EXIT_SUCCESS;
}

size_t validate_params(char *input_file, char *input_dir, char *zone_file)
{
    if ((input_file != NULL) && (input_dir != NULL))
    {
        log_info("\n\tPlease provide either input file *OR* input dir\n");
        log_info("\tSee -h for help\n");
        return EXIT_FAILURE;
    }
    if (
        ((input_file == NULL) || (input_dir == NULL)) &&
        (zone_file == NULL)
        )
    {
        log_info("\n\tPlease provide input file/dir and zone file\n");
        log_info("\tSee -h for help\n");
        return EXIT_FAILURE;
    }

    if (((input_file != NULL) && access(input_file, R_OK) != 0))
    {
        log_info("Cannot access input file %s\n", input_file);
        return EXIT_FAILURE;
    }
    if (((input_dir != NULL) && access(input_dir, R_OK) != 0))
    {
        log_info("Cannot access input dir %s\n", input_dir);
        return EXIT_FAILURE;
    }
    if (access(zone_file, R_OK) != 0)
    {
        log_info("Cannot access zone file %s\n", zone_file);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

size_t magick_cropSaveImage(char *src_file, char *dest_path, int w, int h, int x, int y)
{
    MagickWand *wand = NewMagickWand();
    log_debug("MAGICK cropping to w=%d h=%d x=%d y=%d\n", w, h, x, y);

    if (x == 0 && y == 0)
    {
        return EXIT_SUCCESS;
    }

    if(MagickFalse == MagickReadImage(wand, src_file))
    {
        log_error("MAGICK FAILED opening file %s\n", src_file);
        DestroyMagickWand(wand);
        return EXIT_FAILURE;
    }

    MagickWand *clone = CloneMagickWand(wand);
    if (MagickFalse == MagickCropImage(clone, w, h, x, y))
    {
        ThrowWandException(clone);
        DestroyMagickWand(clone);
        DestroyMagickWand(wand);
        return EXIT_FAILURE;
    }

    if (MagickFalse == MagickWriteImage(clone, dest_path))
    {
        ThrowWandException(clone);
        DestroyMagickWand(clone);
        DestroyMagickWand(wand);
        return EXIT_FAILURE;
    }
    DestroyMagickWand(wand);
    return EXIT_SUCCESS;
}

int get_files_from_dir(char *input_dir, char ***ls)
{
    // TODO: improve perf, dir is scanned twice
    DIR *dir = NULL;
    struct dirent *ent;
    *ls = NULL;
    int file_count = 0;
    bool error_flag = false;
    // TODO: Ensure dir has a trailing slash
    if ((dir = opendir(input_dir)) != NULL)
    {
        // first pass, count files
        while ((ent = readdir (dir)) != NULL)
        {
            // log_debug("Got %s and parsing %s\n", input_dir, ent->d_name);
            if (! (ent->d_type & DT_DIR))
            {
                // TODO: return lowercase ext
                const char *ext = _get_filename_ext(ent->d_name);
                if (
                    (0 == strncmp(ext, "tiff", strlen("tiff"))) ||
                    (0 == strncmp(ext, "tif", strlen("tif"))) ||
                    (0 == strncmp(ext, "pdf", strlen("pdf")))
                    )
                {
                    file_count++;
                }
            }
        }

        // rewind
        rewinddir(dir);
        *ls = calloc(file_count, sizeof(char *));
        // second pass, get files
        file_count = 0;
        char _tmp[256]; _tmp[0] = '\0';
        while ((ent = readdir (dir)) != NULL)
        {
            if (! (ent->d_type & DT_DIR))
            {
                // TODO: return lowercase ext
                const char *ext = _get_filename_ext(ent->d_name);
                if (
                    (0 == strncmp(ext, "tiff", strlen("tiff"))) ||
                    (0 == strncmp(ext, "tif", strlen("tif"))) ||
                    (0 == strncmp(ext, "pdf", strlen("pdf")))
                    )
                {
                    strncat(_tmp, input_dir, strlen(input_dir));
                    // if missing, append a trailing slash
                    if (input_dir[strlen(input_dir)-1] != '/')
                        strncat(_tmp, "/", 1);
                    strncat(_tmp, ent->d_name, strlen(ent->d_name));
                    (*ls)[file_count++] = strdup(_tmp);
                    _tmp[0] = '\0';
                }
            }
        }
        closedir (dir);
    }
    else
    {
        log_error("ERROR Can't read dir %s\n", input_dir);
        error_flag = true;
    }
    if (error_flag)
        return -1;
    else
        return file_count;
}

size_t load_zone_data(char* zone_data, ZoneCoord **zone_coords)
{
    // manually load user generated from zone_data file
    char *delim = ",";
    ssize_t read = 0;
    size_t len = 0;
    char *line = NULL;
    char *token = NULL;
    int count = 0;
    int tk_count = 0;
    FILE *fp = fopen(zone_data, "r");

    if (fp == NULL)
    {
        log_error("ERROR Could not open file %s\n", zone_data);
        return EXIT_FAILURE;
    }

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (NULL != strchr(line, ';'))
            continue;
        if (line[0] == '\n')
           continue;
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        log_debug("Retrieved line of length %zu: '%s'\n", read, line);
        tk_count = 0;
        while ((NULL != (token = strsep(&line, delim))))
        {
            if (tk_count == 0)
                zone_coords[count]->x = atoi(token);
            else if (tk_count == 1)
                zone_coords[count]->y = atoi(token);
            else if (tk_count == 2)
                zone_coords[count]->w = atoi(token);
            else if (tk_count == 3)
                zone_coords[count]->h = atoi(token);
            tk_count++;
        }
        count++;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

size_t get_zone_filename(char *src_file, char **dst_file, char *new_ext)
{
    // e.g. zone_data/delega_tipo_4/qr_zones_150dpi.txt
    char *tmp = strdup(src_file);
    int _len = strlen(src_file);
    char *bname = basename(tmp);
    char *dname = dirname(tmp);
    // retrieve and delete file extension
    char *extn = strrchr(bname, '.');
    if(extn != NULL)
        *extn = '\0';
    char zone_file[_len + 1]; zone_file[0] = '\0';
    strncat(zone_file, dname, strlen(dname));
    strncat(zone_file, "/", 1);
    strncat(zone_file, bname, strlen(bname));
    strncat(zone_file, new_ext, strlen(new_ext));
    free(tmp);

    *dst_file = strdup(zone_file);

    return EXIT_SUCCESS;
}
