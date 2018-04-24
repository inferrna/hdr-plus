#include "Halide.h"
#include "halide_load_raw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include "align.h"
#include "merge.h"
#include "finish.h"

using namespace Halide;

/*
 * HDRPlus Class -- Houses file I/O, defines pipeline attributes and calls
 * processes main stages of the pipeline.
 */
class HDRPlus {

    private:

        const Buffer<uint16_t> imgs;

    public:

        int width; // phone camera pixels
        int height;

        const BlackPoint bp;
        const WhitePoint wp;
        const WhiteBalance wb;
        const Compression c;
        const Gain g;

        HDRPlus(Buffer<uint16_t> imgs, BlackPoint bp, WhitePoint wp, WhiteBalance wb, Compression c, Gain g) : imgs(imgs), bp(bp), wp(wp), wb(wb), c(c), g(g) {
            width = imgs.width();
            height = imgs.height();
            assert(imgs.dimensions() == 3);         // width * height * img_idx
            assert(imgs.width() == width);
            assert(imgs.height() == height);
            assert(imgs.extent(2) >= 2);            // must have at least one alternate image
        }

        /*
         * process -- Calls all of the main stages (align, merge, finish) of the pipeline.
         */
        Buffer<uint8_t> process() {

            Func alignment = align(imgs);
            Func merged = merge(imgs, alignment);
            Func finished = finish(merged, width, height, bp, wp, wb, c, g);

            ///////////////////////////////////////////////////////////////////////////
            // realize image
            ///////////////////////////////////////////////////////////////////////////

            Buffer<uint8_t> output_img(3, width, height);

            finished.realize(output_img);

            // transpose to account for interleaved layout

            output_img.transpose(0, 1);
            output_img.transpose(1, 2);

            return output_img;
        }

        /*
         * load_raws -- Loads CR2 (Canon Raw) files into a Halide Image.
         */
        static bool load_raws(std::string dir_path, std::vector<std::string> &img_names, Buffer<uint16_t> &imgs, uint16_t width, uint16_t height) {

            int num_imgs = img_names.size();

            imgs = Buffer<uint16_t>(width, height, num_imgs);

            uint16_t *data = imgs.data();

            for (int n = 0; n < num_imgs; n++) {

                std::string img_name = img_names[n];
                std::string img_path = dir_path + "/" + img_name;

                if(!Tools::load_image(img_path, data, width, height)) {

                    std::cerr << "Input image failed to load" << std::endl;
                    return false;
                }

                data += width * height;
            }
            return true;
        }

        /*
         * save_png -- Writes an interleaved Halide image to an output file.
         */
        static bool save_png(std::string dir_path, std::string img_name, Buffer<uint8_t> &img) {

            std::string img_path = dir_path + "/" + img_name;

            std::remove(img_path.c_str());

            int stride_in_bytes = img.width() * img.channels();

            if(!stbi_write_png(img_path.c_str(), img.width(), img.height(), img.channels(), img.data(), stride_in_bytes)) {

                std::cerr << "Unable to write output image '" << img_name << "'" << std::endl;
                return false;
            }

            return true;
        }
};

/*
 * read_white_balance -- Reads white balance multipliers from file and returns WhiteBalance.
 */
const WhiteBalance read_white_balance(std::string file_path) {

    Tools::Internal::PipeOpener f(("dcraw -v -i " + file_path).c_str(), "r");
    
    char buf[1024];

    while(f.f != nullptr && !feof(f.f)) {

        f.readLine(buf, 1024);

        float r, g0, g1, b;

        if(sscanf(buf, "Camera multipliers: %f %f %f %f", &r, &g0, &b, &g1) == 4) {

            float rm, g0m, g1m, bm; //To avoid divide by 0 set zero values to maximum
            g1m = g1>0 ? g1 : 100000;
            g0m = g0>0 ? g0 : 100000;
            bm = b>0 ? b : 100000;
            rm = r>0 ? r : 100000;
            float m = std::min(std::min(rm, g0m), std::min(g1m, bm));

            return {std::max(1.0f, r/m), std::max(1.0f, g0/m), std::max(1.0f, g1/m), std::max(1.0f, b/m)};
        }
    }

    return {2, 1, 2, 1};
}

int main(int argc, char* argv[]) {
    
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " [-c compression (default 3.8) -g gain (default 1.1)] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    Compression c = 3.8f;
    Gain g = 1.1f;

    int i = 1;

    while(argv[i][0] == '-') {

        if(argv[i][1] == 'c') {

            c = atof(argv[++i]);
            i++;
            continue;
        }
        else if(argv[i][1] == 'g') {

            g = atof(argv[++i]);
            i++;
            continue;
        }
        else {
            std::cerr << "Invalid flag '" << argv[i][1] << "'" << std::endl;
            return 1;
        }
    }

    if (argc - i < 4) {
        std::cerr << "Usage: " << argv[0] << " [-c compression (default 3.8) -g gain (default 1.1)] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    std::string dir_path = argv[i++];
    std::string out_name = argv[i++];

    std::vector<std::string> in_names;

    while (i < argc) in_names.push_back(argv[i++]);

    Buffer<uint16_t> imgs;

    if(!HDRPlus::load_raws(dir_path, in_names, imgs, 5312, 2988)) return -1;

    const WhiteBalance wb = read_white_balance(dir_path + "/" + in_names[0]);
    const BlackPoint bp = 2050;
    const WhitePoint wp = 15464;

    HDRPlus hdr_plus = HDRPlus(imgs, bp, wp, wb, c, g);

    Buffer<uint8_t> output = hdr_plus.process();
    
    if(!HDRPlus::save_png(dir_path, out_name, output)) return -1;

    return 0;
}
