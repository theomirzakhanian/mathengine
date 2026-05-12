#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <cstring>

extern "C" int win32_open_file_dialog(char* out, size_t out_size) {
    OPENFILENAMEA ofn = {};
    char file_buf[1024] = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = file_buf;
    ofn.nMaxFile = sizeof(file_buf);
    ofn.lpstrFilter = "Images\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tga;*.tiff\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Choose background image";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        strncpy(out, file_buf, out_size - 1);
        out[out_size - 1] = '\0';
        return 1;
    }
    return 0;
}
#endif
