#!/bin/bash

outputFile="resources/graphicResources.rcp"
assetsPath="resources/assets/"
fileSuffix=".bmp"
fileHiResSuffix="-144.bmp"
hires=0
if [ "$1" == "--hires" ]; then
    hires=1
fi

if [ -f $outputFile ]; then
    rm $outputFile
fi

while IFS=';' read -r identifier name
do
    echo "BITMAPFAMILY ID $identifier" >> $outputFile
    echo "BEGIN" >> $outputFile
    hiresFile=$assetsPath$name$fileHiResSuffix
    if [ $hires == 1 ] && [ -f $hiresFile ]; then
        echo "	BITMAP \"$assetsPath$name$fileHiResSuffix\" BPP 8 TRANSPARENTINDEX 4 DENSITY 144 COMPRESS"  >> $outputFile
    else
        echo "	BITMAP \"$assetsPath$name$fileSuffix\" BPP 8 TRANSPARENTINDEX 4 COMPRESS"  >> $outputFile
    fi
    
    echo "END"  >> $outputFile

done < "resources/graphicResources.map"