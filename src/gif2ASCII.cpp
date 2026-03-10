#include "core.h"
#include "CImg.h"
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <gif_lib.h>
using namespace std;

namespace po = boost::program_options;

const string DEFAULT_CHAR_SET = "../character_sets/characters.txt";

CImg<unsigned char> frameToCImg(SavedImage& frame, ColorMapObject* globalColorMap, CImg<unsigned char>& canvas) {
    GifImageDesc& desc = frame.ImageDesc;
    ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : globalColorMap;

    for (int y = 0; y < desc.Height; y++) {
        for (int x = 0; x < desc.Width; x++) {
            int idx = frame.RasterBits[y * desc.Width + x];
            GifColorType color = colorMap->Colors[idx];
            canvas(x + desc.Left, y + desc.Top, 0, 0) = (unsigned char)(0.299*color.Red + 0.587*color.Green + 0.114*color.Blue);
        }
    }

    return canvas;
}

vector<CImg<unsigned char>> gifToCImgs(string inputFile) {
    int error;
    GifFileType* gif = DGifOpenFileName(inputFile.c_str(), &error);
    DGifSlurp(gif);

    vector<CImg<unsigned char>> frames;
    CImg<unsigned char> canvas(gif->SWidth, gif->SHeight, 1, 1, 0);
    for (int i = 0; i < gif->ImageCount; i++) {
        CImg<unsigned char> previous = canvas; // backup canvas

        SavedImage& frame = gif->SavedImages[i];

        frames.push_back(frameToCImg(frame, gif->SColorMap, canvas));

        // Get the disposal method of the frame from the extension blocks
        for (int i = 0; i < frame.ExtensionBlockCount; i++) {
            ExtensionBlock& block = frame.ExtensionBlocks[i];
            if (block.Function == GRAPHICS_EXT_FUNC_CODE) {
                int disposal = (block.Bytes[0] >> 3) & 7;
                if (disposal == 2) { // 2 is code for dispose, so clear canvas
                    canvas.fill(0);
                } else if (disposal == 3) {
                    canvas = previous;
                }
            }
        }

    }

    DGifCloseFile(gif, &error);
    return frames;
}

vector<vector<string>> convertGif(string inputFile, string characterSetFile, int outputWidth, float charAspect, bool invert) {
    vector<CImg<unsigned char>> frames = gifToCImgs(inputFile);

    vector<string> characterSet = getCharacterSet(characterSetFile);

    vector<vector<string>> convertedFrames;
    for (CImg<unsigned char> frame : frames) {

        if (invert) {
            frame = 255 - frame;
        }

        CImg<unsigned char> resizedFrame = resizeImage(frame, outputWidth, charAspect);

        map<int, string> mapping = mapCharacterDensity(characterSet, resizedFrame, false);
        convertedFrames.push_back(renderImage(resizedFrame, mapping));
    }

    return convertedFrames;
}

int main(int argc, char* argv[]) {
    // Program Options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input file")
        ("charset,c", po::value<std::string>(), "character set file")
        ("width,w", po::value<int>()->default_value(60), "width of output in characters")
        ("aspect,a", po::value<float>()->default_value(0.4f, "0.4"), "aspect ratio of height to width of characters")
        ("invert,v", "invert the image");

    po::positional_options_description p;
    p.add("input", 1);

    po::variables_map vm;
    
    try {
        po::store(
            po::command_line_parser(argc, argv)
                .options(desc)
                .positional(p)
                .run(),
            vm
        );

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }

        po::notify(vm);
    } catch (const po::error& e) {
        cout << e.what() << "\n";
        cout << desc << "\n";
        return 1;
    }

    string inputFile;
    string characterSetFile;
    int width;
    float charAspect;
    bool invert = false;

    inputFile = vm["input"].as<string>();
    // Manually define as otherwise the default value in --help is annoying
    if (vm.count("charset")) {
        characterSetFile = vm["charset"].as<std::string>();
    } else {
        characterSetFile = DEFAULT_CHAR_SET;
    }
    width = vm["width"].as<int>();
    charAspect = vm["aspect"].as<float>();
    if (width < 10 || width > 500) {
        cout << "Error: width must be between 10 and 500\n";
        return 1;
    }
    if (vm.count("invert")) {
        invert = true;
    }

    vector<vector<string>> convertedFrames = convertGif(inputFile, characterSetFile, width, charAspect, invert);

    int frameHeight = 0;
    for (vector<string> frame : convertedFrames) {
        cout << "\033[" << frameHeight << "A";
        frameHeight = frame.size() + 2; // + 2 for the two \ns
        cout << "\n";
        for (string line: frame) {
            cout << line << "\n";
        }
        cout << "\n";
        usleep(100000);
    }
    

}


// issues:
// lines not being consistent when going back up - solved with frame updates and canvas. still weird for nyan cat??
// range not correct and when use standard its inverted? idk why at all.