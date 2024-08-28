#!/bin/bash
if ! command -v convert &> /dev/null; then
    echo "Could not find ImageMagick, aborting..."
    exit 1
fi

if [ ! -d ./resources/144 ]; then
    mkdir ./resources/144
fi

if [ ! -d ./resources/assets ]; then
    mkdir ./resources/assets
fi

cp ./resources/presized/*.bmp ./resources/assets/

for image in ./resources/png/mill/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/144/$name.png"
    convert "$image" -scale 60x60 "$outputFile"
done

for image in ./resources/png/chicken/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/144/$name.png"
    convert "$image" -scale 32x32 "$outputFile"
done

for image in ./resources/png/palms/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/144/$name.png"
    convert "$image" -gravity North -crop 100%x60%+0+0 +repage -scale x80 "$outputFile"
done

for image in ./resources/png/palms/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/144/$name.png"
    convert "$image" -gravity North -crop 100%x60%+0+0 +repage -scale x80 "$outputFile"
done

for image in ./resources/png/buttons/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/144/$name.png"
    convert "$image" -scale 30x30 "$outputFile"
done

cp resources/png/others/* resources/144/
convert resources/png/others/ufo.png -scale x60 ./resources/144/ufo.png
convert resources/png/others/headerlogo.png -scale 44x44 -gravity center -background transparent -extent 44x44 resources/144/icon.png
convert resources/png/others/headerlogo.png -scale 30x18 -gravity center -background transparent -extent 30x18  resources/144/iconsmall.png
convert resources/png/others/headerlogo_centered.png -threshold 50% -scale 22x22 -gravity center -background transparent -extent 22x22 -negate  "BMP3:resources/assets/icon-1.bmp"
convert resources/png/others/headerlogo_centered.png -threshold 50% -scale 15x9 -gravity center -background transparent -extent 15x9 -negate "BMP3:resources/assets/iconsmall-1.bmp"


# LOW RES
for image in ./resources/144/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove -filter Point -resize 50% "BMP3:$outputFile"
done

# HI RES
for image in ./resources/144/*.png; do
    filename=$(basename "$image")
    name="${filename%.*}"
    outputFile="./resources/assets/$name-144.bmp"
    convert "$image" -channel alpha -threshold 20% +channel -background '#ff33ff' -alpha remove "BMP3:$outputFile"
done
