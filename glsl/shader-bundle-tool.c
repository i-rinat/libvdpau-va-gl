#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>


int
main(int argc, char *argv[])
{
    if (argc <= 2) {
        printf("not enough arguments\n");
        return 1;
    }

    FILE *fp_h = fopen(argv[1], "w");
    if (!fp_h) {
        printf("can't open %s\n", argv[1]);
        return 2;
    }

    FILE *fp_c = fopen(argv[2], "w");
    if (!fp_c) {
        printf("can't open %s\n", argv[2]);
        return 2;
    }

    // h file
    fprintf(fp_h,
        "// generated file, all changes will be lost\n\n"
        "#ifndef VA_GL_GLSL_SHADERS_H\n"
        "#define VA_GL_GLSL_SHADERS_H\n"
        "\n"
        "#include <GL/gl.h>\n"
        "\n"
        "struct shader_s {\n"
        "    const char *body;\n"
        "    int         len;\n"
        "};\n"
        "\n"
        "extern struct shader_s glsl_shaders[%d];\n\n", argc - 3);
    fprintf(fp_h, "#define SHADER_COUNT     %d\n\n", argc - 3);
    fprintf(fp_h, "enum {\n");
    for (int k = 3; k < argc; k ++) {
        char *fname = strdup(argv[k]);
        char *bname = basename(fname);

        char *last = strchr(bname, '.');
        if (!last)
            continue;
        fprintf(fp_h, "    glsl_%.*s = %d,\n", (int)(last - bname), bname, k - 3);
        free(fname);
    }
    fprintf(fp_h,
        "};\n\n");
    fprintf(fp_h, "#endif /* VA_GL_GLSL_SHADERS_H */\n");
    fclose(fp_h);

    // c file
    fprintf(fp_c, "// generated file, all changes will be lost\n\n");
    char *tmps = strdup(argv[1]);
    char *h_name = basename(tmps);
    fprintf(fp_c, "#include \"%s\"\n", h_name);
    free(tmps);
    fprintf(fp_c, "\n");
    fprintf(fp_c, "struct shader_s glsl_shaders[%d] = {\n", argc - 3);
    for (int k = 3; k < argc; k ++) {
        FILE *fp_tmp = fopen(argv[k], "r");
        if (!fp_tmp) {
            printf("can't open %s\n", argv[k]);
            return 2;
        }

        struct stat sb;
        if (fstat(fileno(fp_tmp), &sb) != 0) {
            printf("can't fstat, errno = %d\n", errno);
            return 4;
        }
        char *buf = malloc(sb.st_size);
        if (!buf) {
            printf("not enough memory\n");
            return 3;
        }
        if (fread(buf, sb.st_size, 1, fp_tmp) != 1) {
            printf("can't read data from file\n");
            return 5;
        }
        fclose(fp_tmp);

        fprintf(fp_c, "  {\n");
        fprintf(fp_c, "    .body =\n");
        fprintf(fp_c, "      \"");
        int len = 0;
        for (unsigned int j = 0; j < sb.st_size; j ++) {
            switch (buf[j]) {
            case '\n':
                fprintf(fp_c, "\\n\"\n      \"");
                len ++;
                break;
            case '\r':
                break;
            default:
                fprintf(fp_c, "%c", buf[j]);
                len ++;
                break;
            }
        }
        fprintf(fp_c, "\",\n");
        fprintf(fp_c, "    .len = %d,\n", len);
        fprintf(fp_c, "  },\n");

        free(buf);
    }
    fprintf(fp_c, "};\n");
    fclose(fp_c);

    return 0;
}
