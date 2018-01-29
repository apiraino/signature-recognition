# -*- coding: utf-8 -*-
from __future__ import absolute_import
from __future__ import unicode_literals

import os
import sys
import shutil
import subprocess


def create_wheel():
    wheelhouse = os.path.abspath('wheelhouse')
    shutil.rmtree(wheelhouse, ignore_errors=True)
    cmd = [sys.executable, '-m', 'pip', 'wheel', '-q', '-w', wheelhouse, '--no-deps', '.']
    subprocess.run(cmd)
    return wheelhouse


def create_package():
    pkg_path = os.path.abspath('pkg.zip')
    shutil.rmtree(os.path.join('.', 'package'), ignore_errors=True)
    cmd = [sys.executable, '-m', 'pip', 'install', '-q', '-t', 'package', '.']
    subprocess.run(cmd)
    return pkg_path
