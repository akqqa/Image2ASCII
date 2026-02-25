# Image2ASCII

This project will:
- Allow an image to be passed as a cmd line variable, and printed out in ASCII
- Allow a default character set to be used, OR a custom file with all characters to be used
    - Custom file will need characters ordered from highest to lowest density.
- Allow the resolution to be specified (default to the right size for the image dimensions?)
- Allow custom sets to be input in any order, and programatically assign order by pixel density (bonus)


# Compilation

g++ image2ASCII.cpp -std=c++17 -O2 -o image2ASCII -ljpeg -lpng -lX11 -lboost_program_options