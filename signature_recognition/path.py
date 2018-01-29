# -*- coding: utf-8 -*-
from __future__ import absolute_import
from __future__ import unicode_literals

import os
import shutil

BASE_OUTPUT_DIR = "/tmp/"
CSV_FILE = 'results.csv'

OMR_BIN = os.environ.get('OMR_BIN_PATH',
                         os.path.join('signature_crop', 'omr'))
if not OMR_BIN:
    raise EnvironmentError("Could not find 'omr' executable")

CONVERT_BIN = 'convert'
PDF_DIR = os.path.join(BASE_OUTPUT_DIR, 'pdf')
BMP_DIR = os.path.join(BASE_OUTPUT_DIR, 'bmp')
SVG_DIR = os.path.join(BASE_OUTPUT_DIR, 'svg')
CSV_PATH = os.path.join(BASE_OUTPUT_DIR, CSV_FILE)


def init_dirs():
    for d in [BMP_DIR, SVG_DIR]:
        if os.path.exists(d):
            if os.path.isdir(d):
                shutil.rmtree(d)
            else:
                os.unlink(d)
        os.makedirs(d)

    # SVG results dirs
    if os.path.exists(SVG_DIR):
        for d in ['GOOD', 'WARN', 'BAD', 'UNKNOWN']:
            dd = os.path.join(SVG_DIR, d)
            os.makedirs(dd)

    if os.path.exists(CSV_PATH):
        os.unlink(CSV_PATH)


def cleanup_dirs():
    '''Cleanup dirs'''
    for d in [BMP_DIR, SVG_DIR]:
        shutil.rmtree(d)
