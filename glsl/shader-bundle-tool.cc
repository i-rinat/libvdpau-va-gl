#include <fstream>
#include <iostream>
#include <sstream>


using std::cerr;
using std::fstream;
using std::ios_base;
using std::string;
using std::stringstream;

namespace {

string
get_basename(const string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos == string::npos)
        return path;

    return path.substr(pos + 1);
}

} // anonymous namespace

int
main(int argc, char *argv[])
{
    if (argc <= 2) {
        cerr << "not enough arguments\n";
        return 1;
    }

    fstream fp_h{argv[1], ios_base::out};
    fstream fp_c{argv[2], ios_base::out};
    
    if (!fp_h.is_open()) {
        cerr << "can't open " << argv[1] << "\n";
        return 2;
    }

    if (!fp_c.is_open()) {
        cerr << "can't open " << argv[2] << "\n";
        return 2;
    }

    // h file
    fp_h << "// generated file, all changes will be lost\n\n"
            "#pragma once\n\n\n"
            "struct shader_s {\n"
            "    const char *body;\n"
            "    int         len;\n"
            "};\n"
            "\n"
            "extern struct shader_s glsl_shaders[" << argc - 3 << "];\n\n";
    fp_h << "#define SHADER_COUNT     " << argc - 3 << "\n\n";
    fp_h << "enum {\n";

    for (int k = 3; k < argc; k ++) {
        const string bname = get_basename(argv[k]);

        size_t pos = bname.find_last_of('.');
        if (pos == string::npos)
            continue;

        fp_h << "    glsl_" << bname.substr(0, pos) << " = " << k - 3 << ",\n";
    }

    fp_h << "};\n";

    // c file
    fp_c << "// generated file, all changes will be lost\n\n";

    const string h_name = get_basename(argv[1]);

    fp_c << "#include \"" << h_name << "\"\n";
    fp_c << "\n";
    fp_c << "struct shader_s glsl_shaders[" << argc - 3 << "] = {\n";

    for (int k = 3; k < argc; k ++) {
        stringstream ss;
        fstream fp_tmp{argv[k], ios_base::in};

        if (!fp_tmp.is_open()) {
            cerr << "can't open " << argv[k] << "\n";
            return 2;
        }

        ss << fp_tmp.rdbuf();

        fp_c << "  {\n";
        fp_c << "    .body =\n";
        fp_c << "      \"";

        size_t len = 0;
        for (const char c: ss.str()) {
            switch (c) {
            case '\n':
                fp_c << "\\n\"\n      \"";
                len ++;
                break;

            case '\r':
                break;

            default:
                fp_c << c;
                len ++;
                break;
            }
        }
        fp_c <<"\",\n";
        fp_c << "    .len = " << len << ",\n";
        fp_c << "  },\n";
    }

    fp_c << "};\n";

    return 0;
}
