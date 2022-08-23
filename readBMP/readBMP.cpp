#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <fstream>

using namespace std;

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;

unsigned char* readBMP(const char* filename, int &size, int&width, int&height, int &image_size)
{
    FILE* fp = fopen(filename, "rb");

    if (fp == NULL) throw "Argument Exception";

    memset(&fileHeader, 0, sizeof(BITMAPFILEHEADER));
    memset(&infoHeader, 0, sizeof(BITMAPINFOHEADER));

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    size = fileHeader.bfSize;
    width = infoHeader.biWidth;
    height = infoHeader.biHeight;
    height = abs(height);
    image_size = infoHeader.biSizeImage;

    unsigned char* data_buf = (unsigned char*)malloc(sizeof(unsigned char) * image_size);
    
    fread(data_buf, image_size, 1, fp);
    fclose(fp);

    return data_buf;
}

void saveBMP(const char* save_name, int& width, int& height, unsigned char *save_buf) {
    FILE* fp = NULL;
    fopen_s(&fp, save_name, "wb");
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(save_buf, (width * height * 3), 1, fp);
    fclose(fp);
}

void copy_buf(unsigned char* data_buf, unsigned char* copy_buf, int image_size) {
    for (int i = 0; i < image_size; i++) {
        copy_buf[i] = data_buf[i];
    }
}

void make_histogram(unsigned char* data_buf, int image_size, int blue[256], int green[256], int red[256]) {
    for (int i = 0; i < image_size; i = i + 3) {
        blue[(int)data_buf[i]] += 1;
        green[(int)data_buf[i + 1]] += 1;
        red[(int)data_buf[i + 2]] += 1;
    }
}

void make_CSVfile(string filename, int blue[256], int green[256], int red[256]) {
    ofstream CSVfile;
    CSVfile.open(filename + "_histogram.csv");
    CSVfile << "histogram" << endl;
    CSVfile << "Color : blue" << endl;
    for (int i = 0; i < 256; i++) {
        CSVfile << blue[i] << ",";
    }
    CSVfile << endl << "Color : green" << endl;
    for (int i = 0; i < 256; i++) {
        CSVfile << green[i] << ",";
    }
    CSVfile << endl << "Color : red" << endl;
    for (int i = 0; i < 256; i++) {
        CSVfile << red[i] << ",";
    }
    CSVfile.close();
}

void flip(unsigned char* data_buf, unsigned char* flip_buf, int width, int height) {
    int mult_width = width * 3;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < mult_width; x = x + 3) {
            flip_buf[mult_width * y + x] = data_buf[mult_width * (y + 1) - (x + 3)];
            flip_buf[mult_width * y + x + 1] = data_buf[mult_width * (y + 1) - (x + 2)];
            flip_buf[mult_width * y + x + 2] = data_buf[mult_width * (y + 1) - (x + 1)];
        }
    }
}

void bright(unsigned char* bright_buf, int& image_size, double bright_percent) {
    for (int i = 0; i < image_size; i++) {
        int value = (double)bright_buf[i] * bright_percent;

        if (bright_buf[i] + value > 255) {
            bright_buf[i] = 255;
        }
        else bright_buf[i] += value;
    }
}

void zoom(unsigned char* data_buf, unsigned char* zoom_buf, int &width, int &height, int ratio) {
    int mult_width = width * 3;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < mult_width; x += 3) {
            if (ratio % 2 != 0) {
                for (int i = 0; i < (ratio * ratio) - 1; i = i + 3) {
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 0)] = data_buf[(mult_width * y) + (x + 0)];
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 1)] = data_buf[(mult_width * y) + (x + 1)];
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 2)] = data_buf[(mult_width * y) + (x + 2)];
                }
            }
            else {
                for (int i = 0; i < (ratio * ratio) + 1; i = i + 3) {
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 0)] = data_buf[(mult_width * y) + (x + 0)];
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 1)] = data_buf[(mult_width * y) + (x + 1)];
                    zoom_buf[(mult_width * y * (ratio * ratio)) + (x * ratio) + (i + 2)] = data_buf[(mult_width * y) + (x + 2)];
                }
            }
        } 
    }
    for (int y = ratio - 1; y < height * ratio; y = y + ratio) {
        for (int x = 0; x < mult_width * ratio; x++) {
            for (int i = 0; i < ratio - 1; i++) {
                memcpy(&zoom_buf[mult_width * (y - i) * ratio + x], &zoom_buf[mult_width * (y - (ratio - 1)) * ratio + x], 1);
            }
        }
    }
    infoHeader.biWidth *= ratio;
    infoHeader.biHeight *= ratio;

    width *= ratio;
    height *= ratio;
}

int main(int argc, char* argv[]) {
    string filename = argv[1];
    string name = filename.substr(0, filename.find('.'));
    const char* file_name = filename.c_str();

    int width, height, size, image_size;

    unsigned char* data_buf = readBMP(file_name, size, width, height, image_size);

    cout << "=========================" << endl;
    cout << "FILE information" << endl << endl;
    cout << "Name: " << filename << endl;
    cout << "Size: " << size << "byte" << endl;
    cout << "Width: " << width << endl;
    cout << "Height: " << height << endl;
    cout << "=========================" << endl << endl;

    int blue[256] = { 0, };
    int green[256] = { 0, };
    int red[256] = { 0, };

    make_histogram(data_buf, image_size, blue, green, red);
    make_CSVfile(name, blue, green, red);
    cout << "histogram save success" << endl << endl;

    unsigned char* flip_buf = (unsigned char*)malloc(sizeof(unsigned char) * image_size);
    flip(data_buf, flip_buf, width, height);
    string flip_savename = name + "_flip.bmp";
    const char* FlipSaveName = flip_savename.c_str();
    saveBMP(FlipSaveName, width, height, flip_buf);
    cout << "flip file save success" << endl << endl;

    unsigned char* bright_buf = (unsigned char*)malloc(sizeof(unsigned char) * image_size);
    copy_buf(data_buf, bright_buf, image_size);
    double bright_percent =0.5;
    bright(bright_buf, image_size, bright_percent);
    string bright_savename = name + "_bright.bmp";
    const char* BrightSaveName = bright_savename.c_str();
    saveBMP(BrightSaveName, width, height, bright_buf);
    cout << "bright file save success" << endl << endl;

    int ratio = 5;
    unsigned char* zoom_buf = (unsigned char*)malloc(sizeof(unsigned char) * ((width * ratio) * (height * ratio) * 3));

    zoom(data_buf, zoom_buf, width, height, ratio);
    string zoom_savename = name + "_zoom.bmp";
    const char* ZoomSaveName = zoom_savename.c_str();
    saveBMP(ZoomSaveName, width, height, zoom_buf);
    cout << "zoom file save success" << endl << endl;

    delete data_buf;
    delete flip_buf;
    delete bright_buf;
    delete zoom_buf;

    return 0;
}