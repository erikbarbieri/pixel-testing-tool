#include "ScreenReader.hpp"

#include <Windows.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
// The clipboard takes ownership of the allocated block if SetClipboardData succeeds.
bool toClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return false;
    if (!EmptyClipboard()) { CloseClipboard(); return false; }

    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (!memory) { CloseClipboard(); return false; }
    void* destination = GlobalLock(memory);
    if (!destination) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    std::memcpy(destination, text.c_str(), text.size() + 1);
    GlobalUnlock(memory);

    const bool success = SetClipboardData(CF_TEXT, memory) != nullptr;
    if (!success) GlobalFree(memory);
    CloseClipboard();
    return success;
}

std::string colorText(const RGBQUAD& pixel) {
    std::ostringstream output;
    output << std::hex << std::showbase << std::uppercase
           << static_cast<int>(pixel.rgbRed) << ", "
           << static_cast<int>(pixel.rgbGreen) << ", "
           << static_cast<int>(pixel.rgbBlue) << ", " << 0;
    return output.str();
}
}  // namespace

int main() {
    ScreenReader reader;
    std::cout << "Pixel Testing Tool (Windows)\n"
              << "Hold I = mouse coordinates; U = RGBQUAD; J = both.\n"
              << "The displayed value is copied to the clipboard.\n"
              << "Press Esc to exit.\n\n";

    std::string previousValue;
    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        POINT cursor{};
        if (!GetCursorPos(&cursor)) { Sleep(80); continue; }
        std::string value;
        std::ostringstream coordinates;
        coordinates << cursor.x << ", " << cursor.y;

        if (GetAsyncKeyState('U') & 0x8000) {
            if (reader.updatePixels() && reader.contains(cursor.x, cursor.y)) {
                value = colorText(reader.getPixel(cursor.x, cursor.y));
            }
        } else if (GetAsyncKeyState('I') & 0x8000) {
            value = coordinates.str();
        } else if (GetAsyncKeyState('J') & 0x8000) {
            if (reader.updatePixels() && reader.contains(cursor.x, cursor.y)) {
                value = coordinates.str() + ", " + colorText(reader.getPixel(cursor.x, cursor.y));
            }
        }

        if (!value.empty() && value != previousValue) {
            std::cout << value << '\n';
            if (!toClipboard(value)) {
                std::cerr << "Warning: could not copy to clipboard.\n";
            }
        }
        previousValue = value;
        Sleep(80); // Avoid continuously consuming a CPU core while waiting for keys.
    }
    return 0;
}
