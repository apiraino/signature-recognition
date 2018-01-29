# -*- coding: utf-8 -*-
from __future__ import absolute_import

import click
import os

from . import utils
from . import om,r
from . import path


@click.group()
def tool():
    pass


# package commands

@click.command()
def build():
    """Prepare packages and wheel locally"""
    click.secho('Creating package ...')
    pkg = utils.create_package()
    click.secho('Package created: {}'.format(pkg), fg='green')
    click.secho('Creating wheel...')
    wheel_path = utils.create_wheel()
    click.secho('Wheel created in {}'.format(wheel_path), fg='green')
tool.add_command(build)


# application commands

@click.command()
@click.argument('pdf_dir')
@click.argument('zone_data_file')
def run_evaluation(pdf_dir, zone_data_file):
    """Run evaluation on a PDF dir"""

    if not os.path.isdir(pdf_dir):
        click.secho("Input MUST be a valid directory (got {})".format(pdf_dir), fg='red', err=True)
        return

    # init working dirs
    path.init_dirs()
    # Extract signature regions from PDF file(s) and save in BMP dir
    omr.extract_signatures_from_dir(pdf_dir, zone_data_file)

    # Generate vectorized images from raster images
    click.secho("Generating SVG files ...", fg='blue')
    omr.generate_svg(path.BMP_DIR)

    # Run SVG path evaluation, generate results CSV
    click.secho("Running evaluation ...", fg='blue')
    omr.run_evaluation(path.SVG_DIR)

    # working dirs are useful for debugging purposes
    path.cleanup_dirs()

tool.add_command(run_evaluation)

if __name__ == '__main__':
    tool()
