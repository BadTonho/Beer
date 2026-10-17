#include "tradutorlinux/runtime/winapi.hpp"
#include "tradutorlinux/loader/module.hpp"
#include "tradutorlinux/loader/builtin_modules.hpp"
#include "core/runtime_state_common.hpp"

#include <cstdlib>

namespace tradutorlinux {

extern "C" {

TL_MSABI int tl_GdiplusStartup(void* token, const void* input, void* output) noexcept {
    (void)input;
    (void)output;
    if (token == nullptr) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    static constexpr std::uintptr_t kGdiplusToken = 0x47444950ULL; // "GDIP"
    if (!write_guest_value(token, reinterpret_cast<void*>(kGdiplusToken))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    set_last_error(abi::kErrorSuccess);
    return 0; // Ok
}

TL_MSABI void tl_GdiplusShutdown(void* token) noexcept {
    (void)token;
    set_last_error(abi::kErrorSuccess);
}

TL_MSABI void* tl_GdipAlloc(std::size_t size) noexcept {
    if (size == 0) {
        return nullptr;
    }
    void* const ptr = std::malloc(size);
    if (ptr == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return nullptr;
    }
    set_last_error(abi::kErrorSuccess);
    return ptr;
}

TL_MSABI void tl_GdipFree(void* ptr) noexcept {
    if (ptr != nullptr) {
        std::free(ptr);
    }
    set_last_error(abi::kErrorSuccess);
}

TL_MSABI int tl_GdipCreateBitmapFromStream(void* stream, void** bitmap) noexcept {
    if (bitmap == nullptr || !write_guest_value(bitmap, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    if (stream == nullptr) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    void* const dummy_bitmap = std::malloc(sizeof(std::uintptr_t));
    if (dummy_bitmap == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3; // OutOfMemory
    }
    *reinterpret_cast<std::uintptr_t*>(dummy_bitmap) = 0x424d5031ULL; // "BMP1"
    if (!write_guest_value(bitmap, dummy_bitmap)) {
        std::free(dummy_bitmap);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0; // Ok
}

TL_MSABI int tl_GdipCloneImage(void* image, void** clone) noexcept {
    if (clone == nullptr || !write_guest_value(clone, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    if (image == nullptr) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    void* const dummy_clone = std::malloc(sizeof(std::uintptr_t));
    if (dummy_clone == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3; // OutOfMemory
    }
    *reinterpret_cast<std::uintptr_t*>(dummy_clone) = *reinterpret_cast<const std::uintptr_t*>(image);
    if (!write_guest_value(clone, dummy_clone)) {
        std::free(dummy_clone);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0; // Ok
}

TL_MSABI int tl_GdipDisposeImage(void* image) noexcept {
    if (image == nullptr) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    std::free(image);
    set_last_error(abi::kErrorSuccess);
    return 0; // Ok
}

TL_MSABI int tl_GdipCreateHBITMAPFromBitmap(void* bitmap, void** hbm, std::uint32_t background) noexcept {
    (void)background;
    if (hbm == nullptr || !write_guest_value(hbm, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    if (bitmap == nullptr) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2; // InvalidParameter
    }
    static std::uintptr_t s_next_hbm = 0x8000;
    const auto handle = reinterpret_cast<void*>(++s_next_hbm);
    if (!write_guest_value(hbm, handle)) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0; // Ok
}

static void* allocate_dummy_gdiplus_object(std::uintptr_t magic) {
    void* const ptr = std::malloc(sizeof(std::uintptr_t));
    if (ptr != nullptr) {
        *reinterpret_cast<std::uintptr_t*>(ptr) = magic;
    }
    return ptr;
}

TL_MSABI int tl_GdipCreateBitmapFromFile(const std::uint16_t* filename, void** bitmap) noexcept {
    (void)filename;
    if (bitmap == nullptr || !write_guest_value(bitmap, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x424d5032ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(bitmap, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCreateBitmapFromHBITMAP(void* hbm, void* hpal, void** bitmap) noexcept {
    (void)hbm;
    (void)hpal;
    if (bitmap == nullptr || !write_guest_value(bitmap, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x424d5033ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(bitmap, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCreateFromHDC(void* hdc, void** graphics) noexcept {
    (void)hdc;
    if (graphics == nullptr || !write_guest_value(graphics, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x47524150ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(graphics, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDeleteGraphics(void* graphics) noexcept {
    if (graphics != nullptr) {
        std::free(graphics);
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCreatePen1(std::uint32_t color, float width, int unit, void** pen) noexcept {
    (void)color;
    (void)width;
    (void)unit;
    if (pen == nullptr || !write_guest_value(pen, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x50454e31ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(pen, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDeletePen(void* pen) noexcept {
    if (pen != nullptr) {
        std::free(pen);
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipSetPenStartCap(void* pen, int startCap) noexcept {
    (void)pen;
    (void)startCap;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipSetPenEndCap(void* pen, int endCap) noexcept {
    (void)pen;
    (void)endCap;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipSetPenLineJoin(void* pen, int lineJoin) noexcept {
    (void)pen;
    (void)lineJoin;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCreateSolidFill(std::uint32_t color, void** brush) noexcept {
    (void)color;
    if (brush == nullptr || !write_guest_value(brush, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x42525348ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(brush, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCloneBrush(void* brush, void** cloneBrush) noexcept {
    (void)brush;
    if (cloneBrush == nullptr || !write_guest_value(cloneBrush, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x42525348ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(cloneBrush, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDeleteBrush(void* brush) noexcept {
    if (brush != nullptr) {
        std::free(brush);
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipCreatePath(int fillMode, void** path) noexcept {
    (void)fillMode;
    if (path == nullptr || !write_guest_value(path, static_cast<void*>(nullptr))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    void* const dummy = allocate_dummy_gdiplus_object(0x50415448ULL);
    if (dummy == nullptr) {
        set_last_error(abi::kErrorNotEnoughMemory);
        return 3;
    }
    if (!write_guest_value(path, dummy)) {
        std::free(dummy);
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDeletePath(void* path) noexcept {
    if (path != nullptr) {
        std::free(path);
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipClosePathFigure(void* path) noexcept {
    (void)path;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipAddPathLine(void* path, float x1, float y1, float x2, float y2) noexcept {
    (void)path;
    (void)x1; (void)y1; (void)x2; (void)y2;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipAddPathArc(void* path, float x, float y, float width, float height, float startAngle, float sweepAngle) noexcept {
    (void)path;
    (void)x; (void)y; (void)width; (void)height; (void)startAngle; (void)sweepAngle;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipSetSmoothingMode(void* graphics, int smoothingMode) noexcept {
    (void)graphics;
    (void)smoothingMode;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDrawLine(void* graphics, void* pen, float x1, float y1, float x2, float y2) noexcept {
    (void)graphics; (void)pen;
    (void)x1; (void)y1; (void)x2; (void)y2;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDrawLineI(void* graphics, void* pen, int x1, int y1, int x2, int y2) noexcept {
    (void)graphics; (void)pen;
    (void)x1; (void)y1; (void)x2; (void)y2;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDrawRectangle(void* graphics, void* pen, float x, float y, float width, float height) noexcept {
    (void)graphics; (void)pen;
    (void)x; (void)y; (void)width; (void)height;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDrawEllipse(void* graphics, void* pen, float x, float y, float width, float height) noexcept {
    (void)graphics; (void)pen;
    (void)x; (void)y; (void)width; (void)height;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipDrawPath(void* graphics, void* pen, void* path) noexcept {
    (void)graphics; (void)pen; (void)path;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipFillRectangle(void* graphics, void* brush, float x, float y, float width, float height) noexcept {
    (void)graphics; (void)brush;
    (void)x; (void)y; (void)width; (void)height;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipFillPolygon(void* graphics, void* brush, const void* points, int count, int fillMode) noexcept {
    (void)graphics; (void)brush; (void)points; (void)count; (void)fillMode;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipFillEllipse(void* graphics, void* brush, float x, float y, float width, float height) noexcept {
    (void)graphics; (void)brush;
    (void)x; (void)y; (void)width; (void)height;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipFillPath(void* graphics, void* brush, void* path) noexcept {
    (void)graphics; (void)brush; (void)path;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipBitmapGetPixel(void* bitmap, int x, int y, std::uint32_t* color) noexcept {
    (void)bitmap; (void)x; (void)y;
    if (color == nullptr || !write_guest_value(color, static_cast<std::uint32_t>(0))) {
        set_last_error(abi::kErrorInvalidParameter);
        return 2;
    }
    set_last_error(abi::kErrorSuccess);
    return 0;
}

TL_MSABI int tl_GdipBitmapSetPixel(void* bitmap, int x, int y, std::uint32_t color) noexcept {
    (void)bitmap; (void)x; (void)y; (void)color;
    set_last_error(abi::kErrorSuccess);
    return 0;
}

} // extern "C"
} // namespace tradutorlinux

namespace tradutorlinux::loader {

void register_gdiplus_module() {
    static const ExportedFunction kGdiplusExports[] = {
        {"GdiplusStartup", 1, reinterpret_cast<std::uintptr_t>(&tl_GdiplusStartup), ExportSupport::Stub},
        {"GdiplusShutdown", 2, reinterpret_cast<std::uintptr_t>(&tl_GdiplusShutdown), ExportSupport::Stub},
        {"GdipAlloc", 3, reinterpret_cast<std::uintptr_t>(&tl_GdipAlloc), ExportSupport::Stub},
        {"GdipFree", 4, reinterpret_cast<std::uintptr_t>(&tl_GdipFree), ExportSupport::Stub},
        {"GdipCreateBitmapFromStream", 5, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateBitmapFromStream), ExportSupport::Stub},
        {"GdipCloneImage", 6, reinterpret_cast<std::uintptr_t>(&tl_GdipCloneImage), ExportSupport::Stub},
        {"GdipDisposeImage", 7, reinterpret_cast<std::uintptr_t>(&tl_GdipDisposeImage), ExportSupport::Stub},
        {"GdipCreateHBITMAPFromBitmap", 8, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateHBITMAPFromBitmap), ExportSupport::Stub},
        {"GdipCreateBitmapFromFile", 9, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateBitmapFromFile), ExportSupport::Stub},
        {"GdipCreateBitmapFromHBITMAP", 10, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateBitmapFromHBITMAP), ExportSupport::Stub},
        {"GdipCreateFromHDC", 11, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateFromHDC), ExportSupport::Stub},
        {"GdipDeleteGraphics", 12, reinterpret_cast<std::uintptr_t>(&tl_GdipDeleteGraphics), ExportSupport::Stub},
        {"GdipCreatePen1", 13, reinterpret_cast<std::uintptr_t>(&tl_GdipCreatePen1), ExportSupport::Stub},
        {"GdipDeletePen", 14, reinterpret_cast<std::uintptr_t>(&tl_GdipDeletePen), ExportSupport::Stub},
        {"GdipSetPenStartCap", 15, reinterpret_cast<std::uintptr_t>(&tl_GdipSetPenStartCap), ExportSupport::Stub},
        {"GdipSetPenEndCap", 16, reinterpret_cast<std::uintptr_t>(&tl_GdipSetPenEndCap), ExportSupport::Stub},
        {"GdipSetPenLineJoin", 17, reinterpret_cast<std::uintptr_t>(&tl_GdipSetPenLineJoin), ExportSupport::Stub},
        {"GdipCreateSolidFill", 18, reinterpret_cast<std::uintptr_t>(&tl_GdipCreateSolidFill), ExportSupport::Stub},
        {"GdipCloneBrush", 19, reinterpret_cast<std::uintptr_t>(&tl_GdipCloneBrush), ExportSupport::Stub},
        {"GdipDeleteBrush", 20, reinterpret_cast<std::uintptr_t>(&tl_GdipDeleteBrush), ExportSupport::Stub},
        {"GdipCreatePath", 21, reinterpret_cast<std::uintptr_t>(&tl_GdipCreatePath), ExportSupport::Stub},
        {"GdipDeletePath", 22, reinterpret_cast<std::uintptr_t>(&tl_GdipDeletePath), ExportSupport::Stub},
        {"GdipClosePathFigure", 23, reinterpret_cast<std::uintptr_t>(&tl_GdipClosePathFigure), ExportSupport::Stub},
        {"GdipAddPathLine", 24, reinterpret_cast<std::uintptr_t>(&tl_GdipAddPathLine), ExportSupport::Stub},
        {"GdipAddPathArc", 25, reinterpret_cast<std::uintptr_t>(&tl_GdipAddPathArc), ExportSupport::Stub},
        {"GdipSetSmoothingMode", 26, reinterpret_cast<std::uintptr_t>(&tl_GdipSetSmoothingMode), ExportSupport::Stub},
        {"GdipDrawLine", 27, reinterpret_cast<std::uintptr_t>(&tl_GdipDrawLine), ExportSupport::Stub},
        {"GdipDrawLineI", 28, reinterpret_cast<std::uintptr_t>(&tl_GdipDrawLineI), ExportSupport::Stub},
        {"GdipDrawRectangle", 29, reinterpret_cast<std::uintptr_t>(&tl_GdipDrawRectangle), ExportSupport::Stub},
        {"GdipDrawEllipse", 30, reinterpret_cast<std::uintptr_t>(&tl_GdipDrawEllipse), ExportSupport::Stub},
        {"GdipDrawPath", 31, reinterpret_cast<std::uintptr_t>(&tl_GdipDrawPath), ExportSupport::Stub},
        {"GdipFillRectangle", 32, reinterpret_cast<std::uintptr_t>(&tl_GdipFillRectangle), ExportSupport::Stub},
        {"GdipFillPolygon", 33, reinterpret_cast<std::uintptr_t>(&tl_GdipFillPolygon), ExportSupport::Stub},
        {"GdipFillEllipse", 34, reinterpret_cast<std::uintptr_t>(&tl_GdipFillEllipse), ExportSupport::Stub},
        {"GdipFillPath", 35, reinterpret_cast<std::uintptr_t>(&tl_GdipFillPath), ExportSupport::Stub},
        {"GdipBitmapGetPixel", 36, reinterpret_cast<std::uintptr_t>(&tl_GdipBitmapGetPixel), ExportSupport::Stub},
        {"GdipBitmapSetPixel", 37, reinterpret_cast<std::uintptr_t>(&tl_GdipBitmapSetPixel), ExportSupport::Stub},
    };
    static const InternalModule kGdiplusModule{"gdiplus.dll", kGdiplusExports};
    register_module(kGdiplusModule);
}

} // namespace tradutorlinux::loader
