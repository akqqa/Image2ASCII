#define cimg_use_jpeg
#define cimg_use_png
#include "CImg.h"
#include "image2ASCII.h"
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <boost/program_options.hpp>
using namespace cimg_library;
using namespace std;

namespace po = boost::program_options;

const float CHAR_ASPECT = 0.4f; // Characters are ~half as wide as they are tall

// Algorithm:
// read characters, create a mapping of 256 range to characters evenly based on how many there are#
// calculate size of image, and split into average brightness of each size so one section = one character
// loop through and assign characters to each

vector<string> getCharacterSet(string filename) {
    ifstream ifs(filename);
    vector<string> characterSet;
    for (string line; getline(ifs, line);) {
        characterSet.push_back(line);
    }
    return characterSet;
}

CImg<unsigned char> averageResize(const CImg<unsigned char>& img, int outputWidth, int outputHeight) {
    CImg<unsigned char> result(outputWidth, outputHeight, 1, 1, 0);

    float scaleX = float(img.width()) / outputWidth;
    float scaleY = float(img.height()) / outputHeight;

    for (int x = 0; x < outputWidth; ++x) {
        for (int y = 0; y < outputHeight; ++y) {
            int x0 = int(x * scaleX);
            int y0 = int(y * scaleY);
            int x1 = int((x+1) * scaleX);
            int y1 = int((y+1) * scaleY);

            // Crop the block and take average
            CImg<unsigned char> block = img.get_crop(x0, y0, x1-1, y1-1);
            result(x, y) = (unsigned char)block.mean();
        }
    }

    return result;
}

// Resizes the image so that one pixel = one character. Has the bonus of performing the average operation for us
CImg<unsigned char> resizeImage(CImg<unsigned char> image, int outputWidth, float charAspect) {
    int imgWidth = image.width();
    int imgHeight = image.height();
    float aspectRatio = (float)imgHeight / imgWidth;

    int outputHeight = outputWidth * aspectRatio * charAspect;

    return averageResize(image, outputWidth, outputHeight);
}

map<int, string> mapCharacterDensity(vector<string> characterSet, CImg<unsigned char> image, bool scaleContrast = true) {
    // Get brightness range of image
    int maxBrightness = 0;
    int minBrightness = 255;
    if (scaleContrast) {
        cimg_forXY(image, x, y) {
            unsigned char value = image(x, y);
            if (value > maxBrightness) {
                maxBrightness = value;
            }
            if (value < minBrightness) {
                minBrightness = value;
            }
        }
    }

    // cout << "max brightness: " << maxBrightness << "\n";
    // cout << "min brightness: " << minBrightness << "\n";

    // Divide range by number of character levels
    int numLevels = characterSet.size();
    double intervalSize = double(maxBrightness - minBrightness + 1) / numLevels;

    // Create map - unsure what will happen - can the case of the top level only being say 1 value due to rounding? dont want this
    map<int, string> mapping;
    for (int level = 0; level < numLevels; ++level) {
        int upper = int(minBrightness + intervalSize * (level  + 1)) - 1;
        // Clamp last range to maxBrightness
        if (level == numLevels - 1) {
            upper = maxBrightness;
        }

        mapping.insert({upper, characterSet[level]});
    }

    return mapping;
}

vector<string> renderImage(CImg<unsigned char> image, map<int, string> mapping) {
    vector<string> result;

    cimg_forY(image, y) {
        string line;

        cimg_forX(image, x) {
            unsigned char value = image(x, y);

            // Get character for value
            auto it = mapping.lower_bound(value);
            string chars;
            if (it != mapping.end()) {
                chars += it->second;
            }
            else {
                chars += mapping.rbegin()->second;
            }     

            if (!chars.empty()) {
                line += chars;
            } else {
                line += ' ';
            }
        }

        result.push_back(line);
    }

    return result;
}

int main(int argc, char* argv[]) {
    // Program Options

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("characters,c", po::value<std::string>()->required(), "character set file")
        ("input,i", po::value<std::string>()->required(), "input file")
        ("width,w", po::value<int>()->default_value(60), "width of output in characters")
        ("aspect,a", po::value<float>()->default_value(0.4f), "aspect ratio of height to width of characters")
        ("invert,v", "invert the image");

    po::positional_options_description p;
    p.add("characters", 1);
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
    characterSetFile = vm["characters"].as<string>();
    width = vm["width"].as<int>();
    charAspect = vm["aspect"].as<float>();
    if (width < 10 || width > 500) {
        cout << "Error: width must be between 10 and 500\n";
        return 1;
    }
    if (vm.count("invert")) {
        invert = true;
    }

    vector<string> characterSet = getCharacterSet(characterSetFile);
    // cout << "characters: \n";
    // for (string chars: characterSet) {
    //     cout << chars << "\n";
    // }

    CImg<unsigned char> image(inputFile.c_str());
    if (image.spectrum() != 3) {
        image = image.get_channels(0, 2);
    } else {
        image = image.RGBtoYCbCr().channel(0);
    }
    
    if (invert) {
        image = 255 - image;
    }

    CImg<unsigned char> resizedImage = resizeImage(image, width, charAspect);

    map<int, string> mapping = mapCharacterDensity(characterSet, resizedImage, true);

    // for (auto it = mapping.begin(); it != mapping.end(); ++it) 
    //     cout << it->first << " " << it->second << endl;

    // Output image!
    vector<string> renderedImage = renderImage(resizedImage, mapping);
    cout << "\n";
    for (string line: renderedImage) {
        cout << line << "\n";
    }
    cout << "\n";
}