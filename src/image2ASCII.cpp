#include "core.h"
#include "CImg.h"
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
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

    CImg<unsigned char> image(inputFile.c_str());
    vector<string> output = convertImage(image, characterSetFile, width, charAspect, invert);

    // Output image!
    cout << "\n";
    for (string line: output) {
        cout << line << "\n";
    }
    cout << "\n";
}