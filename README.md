# Image2ASCII

# Features

- Allows an image (JPG/PNG) to be converted into ASCII art.
- Allows a default character set to be used, OR a custom file with all characters to be used.
    - Custom file need characters ordered from highest to lowest density.
- Allows the size of the output image to be chosen in terms of width (how many characters wide).
- Allows the aspect ratio of text characters to be input, to ensure the output image is not warped in the chosen display font.
- Allows the image to be inverted before conversion.


# Compilation

To compile to program, in the ```src/``` directory run the command:

```g++ image2ASCII.cpp -std=c++17 -O2 -o image2ASCII -ljpeg -lpng -lX11 -lboost_program_options```

Note that boost, libjpeg, libpng, and libx11 must be installed on the system.

# Usage

For basic usage, run the command as follows:

```image2ASCII <path/to/image>```

This will output the converted text in the terminal with the default settings defined in the program.

Further options include:

```-c <path/to/characterset>```: The path of the character set file to use

```-w <width>```: The number of characters wide the output should be

```-a <ratio>```: The aspect ratio of the font used to display the result. (For example, if the font is 20px tall and 10px wide, the ratio should be set to 0.5)

```-v```: Invert the input image