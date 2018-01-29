#include "common.h"
#include "log.h"
#include "utils.h"
#include "omr.h"

#include <wand/MagickWand.h>

int LOG_LEVEL;

size_t doOMR(char **input_files, int files_count, char *zone_file, double resolution)
{
    char *input_file;
    ListResultFiles *results;

    if (NULL == (results = malloc(sizeof(ListResultFiles))))
    {
        log_error("Failed memory allocation\n");
        return EXIT_FAILURE;
    }
    else
    {
        results->done = 0;
        results->skipped = 0;
        results->errored = 0;
    }

    for (int i = 0; i < files_count; i++)
    {
        input_file = input_files[i];
        log_info("[%d/%d] Picked up %s\n", i, files_count, input_file);

#ifdef  USE_LOCAL_TIFF

        // extract TIFF from PDF
        if (NULL != strstr(input_file, ".pdf"))
        {
            char *tiff_file = get_dest_path(input_file, "tiff");
            remove(tiff_file);
            if (EXIT_FAILURE == createTiff(input_file, tiff_file, resolution))
            {
                log_error("Failed creation of tiff %s from %s, skipping file\n", tiff_file, input_file);
                results->skipped++;
                continue;
            }
            input_file = tiff_file;
        }

#endif

        log_info("Now processing %s at %f dpi\n", input_file, resolution);

#ifdef DEBUG

        // load zone data
        char *zone_data = NULL;
        if (EXIT_SUCCESS != get_zone_filename(zone_file, &zone_data, ".txt"))
        {
            log_error("Could not retrieve zone file from zone data file %s\n", zone_file);
            return EXIT_FAILURE;
        }
        log_debug("Will load zone data from %s\n", zone_data);

        ZoneCoord *zone_coords[MAX_ZONE_COUNT];
        for (int i = 0; i < MAX_ZONE_COUNT; i++)
        {
            if (NULL == (zone_coords[i] = malloc(sizeof(ZoneCoord))))
            {
                log_error("ERROR Failed memory allocation\n");
                return EXIT_FAILURE;
            }
            zone_coords[i]->x = 0;
            zone_coords[i]->y = 0;
            zone_coords[i]->w = 0;
            zone_coords[i]->h = 0;
        }

        if (EXIT_SUCCESS != load_zone_data(zone_data, &zone_coords))
        {
            log_error("Failed zone data file loading %s\n", zone_data);
            return EXIT_FAILURE;
        }
        free(zone_data);

        int zone_count = sizeof(zone_coords) / sizeof(*zone_coords);
        for (int i = 0; i < zone_count; i++)
            log_debug("[%d] zone x=%d, y=%d, w=%d, h=%d\n",
                      i, zone_coords[i]->x, zone_coords[i]->y, zone_coords[i]->w, zone_coords[i]->h);

        // crop out signatures retrieved at zone coords
        for (int i = 0; i < zone_count; i++)
        {
            char *tiff_file = strdup(input_file);
            char *base_name = basename(tiff_file);
            char *extn = strrchr(base_name, '.');
            if(extn != NULL)
                *extn = '\0';
            // BMP_DIR + slash + bmp_base_name + dot + i + ".bmp" + \0
            size_t len = strlen(BMP_DIR) + 1 + strlen(base_name) + 1 + 1 + strlen(".bmp") + 1;
            char dest_path[len]; dest_path[0] = '\0';
            snprintf(dest_path, len, "%s/%s.%d.bmp", BMP_DIR, base_name, i);

            int crop_w = zone_coords[i]->w;
            int crop_h = zone_coords[i]->h;
            int crop_x = zone_coords[i]->x;
            int crop_y = zone_coords[i]->y;

            if (EXIT_FAILURE == magick_cropSaveImage(input_file, dest_path, crop_w, crop_h, crop_x, crop_y))
            {
                log_debug("MAGICK FAILED saving cropped zone %d in %s\n", i, dest_path);
                return EXIT_FAILURE;
            }
            log_debug("MAGICK cropped zone %d in %s\n", i, dest_path);
            if (tiff_file != NULL) { free(tiff_file); tiff_file = NULL; }
        }

        // free
        for (int i = 0; i < MAX_ZONE_COUNT; i++)
        {
            free(zone_coords[i]); zone_coords[i] = NULL;
        }

#endif

#ifndef DEBUG
#ifdef  USE_LOCAL_TIFF
        // remove intermediate TIFF files
        remove(input_file);
#endif
#endif
        results->done++;

        // end of file(s) iteration

    }

    log_info("Processed %d file(s): %d done %d skipped %d errored\n",
           files_count, results->done, results->skipped, results->errored);

    if (results != NULL) { free(results); results = NULL; }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    // TODO: ifdef loglevels...
    char *input_file = NULL;
    char *input_dir = NULL;
    char *zone_file = NULL;
    double resolution = 150.0;
    char opt;
    while ((opt=(char)(getopt(argc, argv, "i:d:z:r:"))) != EOF) {
        switch (opt) {
        case 'i':
            input_file = optarg;
            break;
        case 'z':
            zone_file = optarg;
            break;
        case 'd':
            input_dir = optarg;
            break;
        case 'r':
            resolution = strtod(optarg, NULL);
            break;
        default:
            log_info("\nUsage:\n");
            log_info(" -i PDF input file\n");
            log_info(" or\n");
            log_info(" -d PDF input dir\n");
            log_info(" -z zone file\n");
            log_info(" -r resolution (default: 150)\n");
            log_info(" -h prints this help\n");
            log_info("\n");
            return EXIT_SUCCESS;
        }
    }
    if (EXIT_SUCCESS != validate_params(input_file, input_dir, zone_file))
    {
        log_info("Could not validate params. Bail out.\n");
        return EXIT_FAILURE;
    }

    char **files = NULL;
    int files_count = 0;
    if (input_dir != NULL)
    {
        log_info("Processing dir %s\n", input_dir);
        // TODO: glob files
        /* glob_t glob_result; */
        /* if (GLOB_ERR != glob(input_dir, GLOB_DOOFFS | GLOB_APPEND, NULL, &glob_result)) */
        /* { */
        /*     for(unsigned int i = 0; i < glob_result.gl_pathc; i++) */
        /*     { */
        /*         log_info(">>> from %s %s\n", input_dir, glob_result.gl_pathv[i]); */
        /*     } */
        /* } */
        /* else */
        /*     log_error(">>> Can't read dir %s\n", input_dir); */
        /* globfree(&glob_result); */

        if (-1 == (files_count = get_files_from_dir(input_dir, &files)))
        {
            log_error("Failed files retrieval from %s\n", input_dir);
            return EXIT_FAILURE;
        }
        log_info("Processing %d files\n", files_count);
    }
    else if (input_file != NULL)
    {
        log_info("Processing just one file %s\n", input_file);
        if (NULL == (files = calloc(1, sizeof(char *))))
        {
            log_error("Failed memory allocation\n");
            return EXIT_FAILURE;
        }
        files[files_count++] = strdup(input_file);
    }
    else
        log_error("input_dir and input_file is NULL, thou shalt not reach this point\n");

    initDirs();

    log_info("Analyzing %d files(s)\n", files_count);
    if (EXIT_SUCCESS != doOMR(files, files_count, zone_file, resolution))
        log_error("Error while running OMR\n");

    if (files != NULL) { free(files); files = NULL; }
    return EXIT_SUCCESS;
}
