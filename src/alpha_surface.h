#pragma once
#include <windows.h>
#include <algorithm>

// GDI writes zero to the high byte of pixels it draws. Start with a high-byte
// marker to distinguish untouched pixels, including when the chosen RGB is black
// or the former color key. Convert once to premultiplied alpha before composition.
namespace overlay {
class AlphaSurface {
public:
    HDC dc = nullptr;
    DWORD* pixels = nullptr;
    int width, height;
    AlphaSurface(int w, int h) : width(w), height(h) {
        if (w <= 0 || h <= 0) return;
        dc = CreateCompatibleDC(nullptr);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0);
        if (dc && bitmap) previous = SelectObject(dc, bitmap);
    }
    ~AlphaSurface() {
        if (previous) SelectObject(dc, previous);
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
    }
    AlphaSurface(const AlphaSurface&) = delete;
    AlphaSurface& operator=(const AlphaSurface&) = delete;
    explicit operator bool() const { return dc && bitmap && pixels; }
    RECT Bounds(RECT r) const {
        r.left = std::clamp<LONG>(r.left, 0, width); r.right = std::clamp<LONG>(r.right, r.left, width);
        r.top = std::clamp<LONG>(r.top, 0, height); r.bottom = std::clamp<LONG>(r.bottom, r.top, height);
        return r;
    }
    void ClearGdi(RECT region, COLORREF background) {
        GdiFlush();
        const DWORD marker = 0xff000000u | (GetRValue(background) << 16) | (GetGValue(background) << 8) | GetBValue(background);
        const auto r = Bounds(region);
        for (int y = r.top; y < r.bottom; ++y)
            std::fill(pixels + y * width + r.left, pixels + y * width + r.right, marker);
    }
    void FinishGdi() {
        GdiFlush();
        for (int i = 0; i < width * height; ++i)
            pixels[i] = (pixels[i] >> 24) == 255 ? 0 : pixels[i] | 0xff000000u;
    }
    void CompositeGdi(const AlphaSurface& source, RECT region, int opacity) {
        GdiFlush();
        const unsigned a = std::clamp(opacity, 0, 100) * 255 / 100;
        const auto r = Bounds(region);
        if (!a) return;
        for (int y = r.top; y < r.bottom; ++y) for (int x = r.left; x < r.right; ++x) {
            const DWORD s = source.pixels[y * source.width + x];
            if ((s >> 24) == 255) continue;
            DWORD& d = pixels[y * width + x];
            DWORD out = (a + ((d >> 24) * (255-a) + 127) / 255) << 24;
            for (unsigned shift : {0u, 8u, 16u})
                out |= ((((s >> shift) & 255) * a + ((d >> shift) & 255) * (255-a) + 127) / 255) << shift;
            d = out;
        }
    }
private:
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
};
}
