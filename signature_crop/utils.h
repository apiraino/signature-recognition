#ifndef UTILS_H
#define UTILS_H

#include <wand/MagickWand.h>

#define BMP_DIR "/tmp/bmp"

typedef struct {
    int x;
    int y;
    int w;
    int h;
} ZoneCoord;

typedef struct {
    int done;
    int skipped;
    int errored;
} ListResultFiles;

// ImageMagick utils
size_t createTiff(char *input_file, char *out_file, double resolution);
size_t _createTiffMagick(char *input_file, char *out_file, double resolution);
size_t _createTiffGs(char *input_file, char *out_file, double resolution);
size_t _createTiffGimp(char *input_file, char *out_file, double resolution);
size_t magick_cropSaveImage(char *src_file, char *dest_file, int w, int h, int x, int y);

// generic utils
size_t initDirs();
size_t validate_params(char *input_file, char *input_dir, char *zone_file);
char *get_dest_path(char *src_file, char *ext);
int get_files_from_dir(char *input_dir, char ***ls);
char *baseconv(unsigned int num, int base);
size_t load_zone_data(char *zone_data, ZoneCoord **zone_coords);
size_t get_zone_filename(char *src_file, char **dst_file, char *new_ext);

#endif
