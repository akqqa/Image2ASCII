#include "CImg.h"
#include <string>
#include <map>
#include <vector>
using namespace cimg_library;
using namespace std;

struct GifFrame {
    CImg<unsigned char> image;
    vector<string> ascii;
    int delay; // Centiseconds
    GifFrame(CImg<unsigned char> img, int d) : image(img), delay(d) {}
};

vector<string> getCharacterSet(string filename);
CImg<unsigned char> resizeImage(CImg<unsigned char> image, int outputWidth, float charAspect);
map<int, string> mapCharacterDensity(vector<string> characterSet, CImg<unsigned char>& image, bool scaleContrast = true);
map<int, string> mapCharacterDensity(vector<string> characterSet, vector<GifFrame>& images, bool scaleContrast = true);
vector<string> renderImage(CImg<unsigned char> image, map<int, string> mapping);

