#include "CImg.h"
#include <string>
#include <map>
#include <vector>
using namespace cimg_library;
using namespace std;

vector<string> getCharacterSet(string filename);
CImg<unsigned char> resizeImage(CImg<unsigned char> image, int outputWidth, float charAspect);
map<int, string> mapCharacterDensity(vector<string> characterSet, CImg<unsigned char> image, bool scaleContrast);
vector<string> renderImage(CImg<unsigned char> image, map<int, string> mapping);