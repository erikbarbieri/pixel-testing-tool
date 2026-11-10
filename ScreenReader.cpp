#include "ScreenReader.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
std::wstring toWide(const std::string& text) {
    if (text.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (required <= 0) return {};
    std::wstring wide(static_cast<size_t>(required), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], required)) return {};
    wide.resize(static_cast<size_t>(required - 1));
    return wide;
}
} // namespace

LONG ScreenReader::coordToIndex(LONG x, LONG y) const {
    return (y - m_Rectangle.top) * getWidth() + (x - m_Rectangle.left);
}

void ScreenReader::setup(bool entireDesktop) {
    if (entireDesktop) {
        m_Rectangle.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
        m_Rectangle.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
        m_Rectangle.right = m_Rectangle.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
        m_Rectangle.bottom = m_Rectangle.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    } else if (!GetWindowRect(m_WindowHandle, &m_Rectangle)) {
        return;
    }
    const LONG width = getWidth(), height = getHeight();
    if (width <= 0 || height <= 0) return;

    m_WindowDC = GetDC(nullptr);
    if (!m_WindowDC) return;
    m_CaptureDC = CreateCompatibleDC(m_WindowDC);
    if (!m_CaptureDC) return;

    m_BitmapInfo = {};
    m_BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_BitmapInfo.bmiHeader.biWidth = width;
    m_BitmapInfo.bmiHeader.biHeight = -height; // Top-down: coordinates match screen coordinates.
    m_BitmapInfo.bmiHeader.biPlanes = 1;
    m_BitmapInfo.bmiHeader.biBitCount = 32;
    m_BitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* rawPixels = nullptr;
    m_CaptureBitmap = CreateDIBSection(m_WindowDC, &m_BitmapInfo, DIB_RGB_COLORS,
                                      &rawPixels, nullptr, 0);
    if (!m_CaptureBitmap) return;
    m_Pixels = static_cast<RGBQUAD*>(rawPixels);
    m_OldBitmap = SelectObject(m_CaptureDC, m_CaptureBitmap);
}

void ScreenReader::destroy() {
    if (m_CaptureDC && m_OldBitmap) SelectObject(m_CaptureDC, m_OldBitmap);
    if (m_CaptureBitmap) DeleteObject(m_CaptureBitmap);
    if (m_CaptureDC) DeleteDC(m_CaptureDC);
    if (m_WindowDC) ReleaseDC(nullptr, m_WindowDC);
    m_Pixels = nullptr;
}

ScreenReader::ScreenReader() {
    m_WindowHandle = GetDesktopWindow();
    setup(true);
    updatePixels();
}

ScreenReader::ScreenReader(const std::string& windowClass, const std::string& windowDesc)
    : m_WindowClass(windowClass), m_WindowDescription(windowDesc) {
    const std::wstring cls = toWide(windowClass), name = toWide(windowDesc);
    m_WindowHandle = FindWindowW(cls.empty() ? nullptr : cls.c_str(),
                                 name.empty() ? nullptr : name.c_str());
    if (m_WindowHandle) {
        setup(false);
        updatePixels();
    }
}

ScreenReader::~ScreenReader() { destroy(); }

int ScreenReader::updatePixels() {
    if (!m_Pixels || !m_WindowDC || !m_CaptureDC) return 0;
    return BitBlt(m_CaptureDC, 0, 0, getWidth(), getHeight(),
                  m_WindowDC, m_Rectangle.left, m_Rectangle.top,
                  SRCCOPY | CAPTUREBLT) ? 1 : 0;
}

bool ScreenReader::contains(LONG x, LONG y) const {
    return m_Pixels && x >= m_Rectangle.left && x < m_Rectangle.right &&
           y >= m_Rectangle.top && y < m_Rectangle.bottom;
}

RGBQUAD ScreenReader::getPixel(LONG x, LONG y) const {
    if (!contains(x, y)) return RGBQUAD{};
    return m_Pixels[coordToIndex(x, y)];
}
BYTE ScreenReader::getRed(LONG x, LONG y) const { return getPixel(x, y).rgbRed; }
BYTE ScreenReader::getGreen(LONG x, LONG y) const { return getPixel(x, y).rgbGreen; }
BYTE ScreenReader::getBlue(LONG x, LONG y) const { return getPixel(x, y).rgbBlue; }
LONG ScreenReader::getNumPixels() const { return getWidth() * getHeight(); }
LONG ScreenReader::getWidth() const { return m_Rectangle.right - m_Rectangle.left; }
LONG ScreenReader::getHeight() const { return m_Rectangle.bottom - m_Rectangle.top; }
const std::string& ScreenReader::getWindowClass() const { return m_WindowClass; }
const std::string& ScreenReader::getWindowDesc() const { return m_WindowDescription; }
HWND const& ScreenReader::getWindow() const { return m_WindowHandle; }
HDC const& ScreenReader::getWindowDC() const { return m_WindowDC; }
HDC const& ScreenReader::getCaptureDC() const { return m_CaptureDC; }
HBITMAP const& ScreenReader::getCaptureBitmap() const { return m_CaptureBitmap; }
const BITMAPINFO& ScreenReader::getBitmapInfo() const { return m_BitmapInfo; }
const RECT& ScreenReader::getRect() const { return m_Rectangle; }
RGBQUAD* const& ScreenReader::getPixels() const { return m_Pixels; }

double ScreenReader::getPixelDiff(LONG x, LONG y, RGBQUAD color) const {
    const RGBQUAD pixel = getPixel(x, y);
    const double dr = static_cast<double>(pixel.rgbRed) - color.rgbRed;
    const double dg = static_cast<double>(pixel.rgbGreen) - color.rgbGreen;
    const double db = static_cast<double>(pixel.rgbBlue) - color.rgbBlue;
    return std::sqrt(dr * dr + dg * dg + db * db);
}
BOOL ScreenReader::getPixelDiff(LONG x, LONG y, RGBQUAD color, double tolerance) const {
    return getPixelDiff(x, y, color) <= tolerance ? TRUE : FALSE;
}
UINT ScreenReader::getPixelsDiff(LONG x, LONG y, LONG width, LONG height,
                                RGBQUAD color, double tolerance) const {
    if (width <= 0 || height <= 0) return 0;
    const LONG left = std::max(x, m_Rectangle.left);
    const LONG top = std::max(y, m_Rectangle.top);
    const LONG right = std::min(x + width, m_Rectangle.right);
    const LONG bottom = std::min(y + height, m_Rectangle.bottom);
    UINT count = 0;
    for (LONG py = top; py < bottom; ++py) {
        for (LONG px = left; px < right; ++px) {
            count += getPixelDiff(px, py, color, tolerance) ? 1U : 0U;
        }
    }
    return count;
}
double ScreenReader::getPixelsDiffPercent(LONG x, LONG y, LONG width, LONG height,
                                         RGBQUAD color, double tolerance) const {
    if (width <= 0 || height <= 0) return 0.0;
    const LONG left = std::max(x, m_Rectangle.left);
    const LONG top = std::max(y, m_Rectangle.top);
    const LONG right = std::min(x + width, m_Rectangle.right);
    const LONG bottom = std::min(y + height, m_Rectangle.bottom);
    if (right <= left || bottom <= top) return 0.0;
    const double area = static_cast<double>(right - left) * (bottom - top);
    return static_cast<double>(getPixelsDiff(x, y, width, height, color, tolerance)) / area;
}
