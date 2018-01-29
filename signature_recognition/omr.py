# -*- coding: utf-8 -*-
from __future__ import absolute_import
from __future__ import unicode_literals

import os
import shutil
import glob
import logging
import subprocess
from . import path

from operator import itemgetter

# https://github.com/regebro/svg.path
from svgpathtools import svg2paths

logger = logging.getLogger()

# Minimun GOOD path to score a positive
THRES_NUM_GOOD_PATH = 3
THRES_NUM_WARN_PATH = 10
THRES_NUM_BAD_PATH = 40

# Path length thresholds
# BAD < 700 < WARN < 1300 < GOOD
THRES_PATH_LEN_WARN = 700
THRES_PATH_LEN_GOOD = 1300  # was: 1100

RATING_GOOD = 0
RATING_WARN = 1
RATING_BAD = 2
RATING_UNKNOWN = 3
RATING_NAMES = [
    (RATING_GOOD, 'GOOD'),
    (RATING_WARN, 'WARN'),
    (RATING_BAD, 'BAD'),
    (RATING_UNKNOWN, 'UNKNOWN'),
]


def get_current_file(fname):
    base_name, ext = os.path.splitext(os.path.basename(fname))
    base_name, sig_number = base_name.split('.')
    return base_name


def get_rating_value(rating):
    return RATING_NAMES[rating['rating_value']][1]


def get_rating_detail(rating):
    return {
        'rating_value': get_rating_value(rating),
        'good': rating['good'],
        'warn': rating['warn'],
        'bad': rating['bad'],
        'signed': rating['signed']
    }


def _rate_signature(good, warn, bad):
    """Rate a signature based on its paths"""

    # enough GOOD found
    # (ignore all the rest: BAD will be there anyway)
    if good >= THRES_NUM_GOOD_PATH:
        return RATING_GOOD

    # too few GOOD found
    # and
    # less than 3 WARN or BAD
    elif (
            0 < good <= THRES_NUM_GOOD_PATH and
            (warn < 3 or bad < 3)
    ):
        return RATING_WARN

    # too few GOOD found
    # and
    # WARN are more than 2/3 of BAD
    elif (
            0 < good <= THRES_NUM_GOOD_PATH and
            ((warn + (warn * .66)) > bad)
    ):
        return RATING_WARN

    # very few GOOD found
    # and
    # WARN are less than 2/3 of BAD
    elif (
            0 < good <= THRES_NUM_GOOD_PATH and
            ((warn + (warn * .66)) < bad)
    ):
        return RATING_BAD

    # many BAD found
    # and up to a handful of GOOD and WARN
    elif (
            0 <= good <= 10 and
            0 <= warn <= 10 and
            bad >= 20
    ):
        return RATING_BAD

    # very low values of everything
    # (almost a blank area)
    elif (
            0 <= good <= 10 and
            0 <= warn <= 10 and
            0 <= bad <= 10
    ):
        return RATING_BAD

    # this should be investigated and managed above
    else:
        logger.warn(">>> what to do with these? good={} warn={} bad={}".format(
            good, warn, bad))
        return RATING_UNKNOWN


def _is_signed(rating):
    """Assess whether the item has a signature or not"""
    g_path = rating['good']
    w_path = rating['warn']
    b_path = rating['bad']
    signature_rating = rating['rating_value']
    signed = ''
    base_confidence = 0.5

    def update_confidence(_signed, _confidence, signature_rating):
        """
        What I'm trying to establish here:

        If I decide the sig. is there:
          RATING_GOOD: great!
          RATING_WARN: well ok
          RATING_BAD: maybe a lot of dirt?
          RATING_UNKNOWN: pls investigate
        else:
          RATING_GOOD: wtf!?
          RATING_WARN: very strange
          RATING_BAD: that's ok
          RATING_UNKNOWN: pls investigate

        """
        if _signed == 'Y':
            if signature_rating == RATING_GOOD:
                _confidence += 0.3
            elif signature_rating == RATING_WARN:
                _confidence -= 0.1
            elif signature_rating == RATING_BAD:
                _confidence -= 0.2
            elif signature_rating == RATING_UNKNOWN:
                _confidence -= 0.3
        else:
            if signature_rating == RATING_GOOD:
                _confidence += 0.2
            elif signature_rating == RATING_WARN:
                _confidence -= 0.1
            elif signature_rating == RATING_BAD:
                _confidence += 0.4
            elif signature_rating == RATING_UNKNOWN:
                _confidence += 0.4
        return _confidence

    # if there are NO GOOD paths AND WARN+BAD are VERY FEW
    # it's not signed
    if (
        (g_path == 0) and
        (w_path <= THRES_NUM_WARN_PATH) and
        (b_path <= THRES_NUM_BAD_PATH)
    ):
        signed = 'N'
    else:
        signed = 'Y'

    _confidence = update_confidence(signed, base_confidence, signature_rating)
    return signed, _confidence


def get_rating(items):
    good, warn, bad = 0, 0, 0
    rating = {}
    for item in items:
        p_len = item['len']

        # classify path based on length
        if p_len < THRES_PATH_LEN_WARN:
            bad += 1
        elif THRES_PATH_LEN_WARN <= p_len < THRES_PATH_LEN_GOOD:
            warn += 1
        elif p_len >= THRES_PATH_LEN_GOOD:
            good += 1
        else:
            logger.warn(">>> What now?")
    rating_val = _rate_signature(good, warn, bad)
    rating.update({
        'rating_value': rating_val,
        'good': good,
        'warn': warn,
        'bad': bad
    })
    signed, confidence = _is_signed(rating)
    rating.update({
        'confidence': confidence,
        'signed': signed
    })
    return rating


def extract_signatures_from_dir(pdf_dir, zone_data_file, resolution=150):
    if not pdf_dir.endswith('/'):
        pdf_dir += '/'
    cmd = [path.OMR_BIN, '-d', pdf_dir, '-z', zone_data_file, '-r', str(resolution)]
    subprocess.run(cmd, check=True)


# def extract_signatures_from_file(pdf_file, zone_data_file, resolution=300):
#     cmd = [path.OMR_BIN, '-i', pdf_file, '-z', zone_data_file, '-r', str(resolution)]
#     subprocess.run(cmd, check=True)


def generate_svg(src_path):
    """Prepare src data for processing, create SVG from raster images

    Args:
      src_path: file or dir

    """
    # gather file(s)
    if not os.path.isdir(src_path):
        logger.debug("Will process file {}".format(src_path))
        svg_file = os.path.join(path.SVG_DIR, os.path.basename(src_path))
        cmd = ['potrace', src_path, '-s', '-tight', '-o', svg_file]
        subprocess.run(cmd, check=True)
    else:
        # create SVG files
        logger.debug("Will process dir {}".format(src_path))
        logger.debug("Generate SVG files from {} -> {}".format(src_path, path.SVG_DIR))
        for bmp_file in glob.glob('{}/*.bmp'.format(path.BMP_DIR)):
            svg_file = bmp_file.replace('.bmp', '.svg')
            svg_file = os.path.join(path.SVG_DIR, os.path.basename(svg_file))
            cmd = ['potrace', bmp_file, '-s', '-tight', '-o', svg_file]
            subprocess.run(cmd, check=True)


def evaluation(fname):
    paths, attributes = svg2paths(fname)
    logger.debug("{} paths found".format(len(paths)))
    res = []
    for i in range(len(paths)):
        res.append({'i': i, 'len': paths[i].length(error=1e-5)})
    items = sorted(res, key=itemgetter('len'), reverse=True)
    svg_tot_len = 0
    for item in items:
        logger.debug("Path {} is {} long".format(item['i'], item['len']))
        svg_tot_len += item['len']
    return svg_tot_len, get_rating(items)


def run_evaluation(input_data, save_csv=True):
    """Evaluate signatures on a set of SVG files"""
    if save_csv:
        logging.basicConfig(format='%(message)s', filename=path.CSV_PATH, level=logging.INFO)
    else:
        logging.basicConfig(format='%(message)s', level=logging.DEBUG)
    logger.debug("Process SVG files")
    if not os.path.isdir(input_data):
        try:
            _, rating = evaluation(input_data)
        except Exception as exc:
            logger.error("Error for file {}".format(input_data))
        logger.info("--- File {} ---".format(input_data))
        logger.info(rating)
    else:
        ratings, prev_file = '', ''
        files = sorted(glob.glob('{}/*.svg'.format(input_data)))
        file_count = len(files)
        count = 0
        for fname in files:
            curr_file = get_current_file(fname)
            try:
                _, rating = evaluation(fname)
            except Exception as exc:
                logger.error("Error for file {}".format(fname))
            rating_val = get_rating_value(rating)
            signed = rating['signed']
            if curr_file != prev_file:
                if prev_file:
                    # not first iteration, save prev row on CSV
                    logger.info(ratings)
                # start a new row
                ratings = "{},{}-{}".format(curr_file, signed, rating['confidence'])
                prev_file = curr_file
            else:
                # append signature rating+confidence
                ratings += ",{}-{}".format(signed, rating['confidence'])

            # save debug signature file in rating directory
            dest_dir = os.path.join(path.SVG_DIR, rating_val)

            # move file in SVG classification directory
            shutil.move(fname, dest_dir)

            count += 1
            # flush buffer on last iteration
            if count == file_count:
                logger.info(ratings)

    if save_csv:
        logger.debug("Saved ResultSet in {}".format(path.CSV_PATH))
