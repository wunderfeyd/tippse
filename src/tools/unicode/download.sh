#!/bin/bash

mkdir -p download
mkdir -p output
wget "https://www.unicode.org/Public/10.0.0/ucd/UCD.zip" -O download/UCD.zip
unzip download/UCD.zip -d download


