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

// Run the core methods to convert an input image to a string vector
vector<string> convertImage(CImg<unsigned char> image, string characterSetFile, int outputWidth, float charAspect, bool invert) {
    vector<string> characterSet = getCharacterSet(characterSetFile);

    if (image.spectrum() != 3) {
        image = image.get_channels(0, 2);
    }
    image = image.RGBtoYCbCr().channel(0);
    
    if (invert) {
        image = 255 - image;
    }

    CImg<unsigned char> resizedImage = resizeImage(image, outputWidth, charAspect);

    map<int, string> mapping = mapCharacterDensity(characterSet, resizedImage, true);

    return renderImage(resizedImage, mapping);
}

// Gif methods

CImg<unsigned char> frameToCImg(SavedImage& frame, ColorMapObject* globalColorMap, CImg<unsigned char>& canvas, bool hasTransparency, int transparentIndex) {
    GifImageDesc& desc = frame.ImageDesc;
    ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : globalColorMap;
    for (int y = 0; y < desc.Height; y++) {
        for (int x = 0; x < desc.Width; x++) {
            int idx = frame.RasterBits[y * desc.Width + x];
            if (hasTransparency && idx == transparentIndex) continue; // Skip transparent pixels
            GifColorType color = colorMap->Colors[idx];
            canvas(x + desc.Left, y + desc.Top, 0, 0) = (unsigned char)(0.299*color.Red + 0.587*color.Green + 0.114*color.Blue);
        }
    }
    return canvas;
}

vector<GifFrame> gifToCImgs(string inputFile) {
    int error;
    GifFileType* gif = DGifOpenFileName(inputFile.c_str(), &error);
    DGifSlurp(gif);

    vector<GifFrame> frames;
    CImg<unsigned char> canvas(gif->SWidth, gif->SHeight, 1, 1, 0);
    for (int i = 0; i < gif->ImageCount; i++) {
        CImg<unsigned char> previous = canvas;
        SavedImage& frame = gif->SavedImages[i];

        // Extract transparency and disposal before applying frame
        GraphicsControlBlock gcb;
        DGifSavedExtensionToGCB(gif, i, &gcb);

        bool hasTransparency = gcb.TransparentColor != NO_TRANSPARENT_COLOR;
        int transparentIndex = gcb.TransparentColor;
        int disposal = gcb.DisposalMode;
        int delay = gcb.DelayTime;

        CImg<unsigned char> image = frameToCImg(frame, gif->SColorMap, canvas, hasTransparency, transparentIndex);
        GifFrame frameData(image, delay);

        frames.push_back(frameData);

        if (disposal == 2) {
            canvas.fill(0);
        } else if (disposal == 3) {
            canvas = previous;
        }
    }

    DGifCloseFile(gif, &error);
    return frames;
}

vector<GifFrame> convertGif(string inputFile, string characterSetFile, int outputWidth, float charAspect, bool invert) {
    vector<GifFrame> frames = gifToCImgs(inputFile);

    vector<string> characterSet = getCharacterSet(characterSetFile);

    map<int, string> mapping = mapCharacterDensity(characterSet, frames);

    vector<GifFrame> convertedFrames;
    for (GifFrame frameData : frames) {

        CImg<unsigned char> frame = frameData.image;

        if (invert) {
            frame = 255 - frame;
        }

        CImg<unsigned char> resizedFrame = resizeImage(frame, outputWidth, charAspect);

        frameData.ascii = renderImage(resizedFrame, mapping);

        convertedFrames.push_back(frameData);
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

    if (inputFile.substr(inputFile.find_last_of(".") + 1) == "png" || inputFile.substr(inputFile.find_last_of(".") + 1) == "jpg" || inputFile.substr(inputFile.find_last_of(".") + 1) == "jpeg") {
        CImg<unsigned char> image(inputFile.c_str());
        vector<string> output = convertImage(image, characterSetFile, width, charAspect, invert);

        // Output image!
        cout << "\n";
        for (string line: output) {
            cout << line << "\n";
        }
        cout << "\n";
    } else if (inputFile.substr(inputFile.find_last_of(".") + 1) == "gif") {
        vector<GifFrame> convertedFrames = convertGif(inputFile, characterSetFile, width, charAspect, invert);

        int frameHeight = 0;
        int frameCounter = 0;
        while(true) {
            GifFrame frameData = convertedFrames[frameCounter];

            vector<string> frame = frameData.ascii;

            cout << "\033[" << frameHeight << "A";
            frameHeight = frame.size() + 1; // + 1 for the \n
            cout << "\n";
            for (string line: frame) {
                cout << line << "\n";
            }
            usleep(frameData.delay * 10000);

            frameCounter = (frameCounter + 1) % convertedFrames.size();
        }
    } else {
        cout << "Input file must be a jpg, png, or gif" << "\n";
    }
}