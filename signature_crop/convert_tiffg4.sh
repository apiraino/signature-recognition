#!/bin/bash

# Using ImageMagick
CONVERT="/usr/bin/convert"
CCIT_PARAMS="-density 150 -compress Group4"

if [ -z "$1" ]; then
    echo "Convert a dir full of PDF. Creates a dir full of TIFF files"
    echo "Usage: " `basename $0` "pdf_dir tiff_dir"
    echo
    exit 0
fi

if [ ! -d "$1" ]; then
    echo "Please provide a src pdf dir"
    echo
    exit 0
fi

if [ -z "$2" ]; then
    echo "Please provide a dest tiff dir"
    echo
    exit 0
fi

rm -rf "$2" && mkdir "$2"
for f in $( ls $1/*.pdf ); do
    base=$( basename $f .pdf )
    tiff="$2/$base.tif"
    echo "Processing: $f --> $tiff"
    $CONVERT $CCIT_PARAMS "$f" "$tiff"
done

exit 0
