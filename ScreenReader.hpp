#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <string>

// Captures desktop or a chosen window into a top-down, 32-bit DIB section.
class ScreenReader {
protected:
    std::string m_WindowClass, m_WindowDescription;
    HWND m_WindowHandle = nullptr;
    HDC m_WindowDC = nullptr;
    HDC m_CaptureDC = nullptr;
    HBITMAP m_CaptureBitmap = nullptr;
    HGDIOBJ m_OldBitmap = nullptr;
    BITMAPINFO m_BitmapInfo{};
    RECT m_Rectangle{};
    RGBQUAD* m_Pixels = nullptr; // Owned by m_CaptureBitmap (do not delete[]).

    LONG coordToIndex(LONG x, LONG y) const;
    void setup(bool entireDesktop);
    void destroy();
public:
    ScreenReader();
    ScreenReader(const std::string& windowClass, const std::string& windowDesc);
    virtual ~ScreenReader();
    int updatePixels();
    bool contains(LONG x, LONG y) const;

    [[nodiscard]] virtual RGBQUAD getPixel(LONG x, LONG y) const;
    [[nodiscard]] virtual BYTE getRed(LONG x, LONG y) const;
    [[nodiscard]] virtual BYTE getGreen(LONG x, LONG y) const;
    [[nodiscard]] virtual BYTE getBlue(LONG x, LONG y) const;
    [[nodiscard]] LONG getNumPixels() const;
    [[nodiscard]] LONG getWidth() const;
    [[nodiscard]] LONG getHeight() const;

    [[nodiscard]] const std::string& getWindowClass() const;
    [[nodiscard]] const std::string& getWindowDesc() const;
    [[nodiscard]] HWND const& getWindow() const;
    [[nodiscard]] HDC const& getWindowDC() const;
    [[nodiscard]] HDC const& getCaptureDC() const;
    [[nodiscard]] HBITMAP const& getCaptureBitmap() const;
    [[nodiscard]] const BITMAPINFO& getBitmapInfo() const;
    [[nodiscard]] const RECT& getRect() const;
    [[nodiscard]] RGBQUAD* const& getPixels() const;

    [[nodiscard]] virtual double getPixelDiff(LONG x, LONG y, RGBQUAD color) const;
    [[nodiscard]] virtual BOOL getPixelDiff(LONG x, LONG y, RGBQUAD color, double tolerance) const;
    [[nodiscard]] virtual UINT getPixelsDiff(LONG x, LONG y, LONG width, LONG height, RGBQUAD color, double tolerance) const;
    [[nodiscard]] virtual double getPixelsDiffPercent(LONG x, LONG y, LONG width, LONG height, RGBQUAD color, double tolerance) const;
};
