#include <stdint.h>
#include <string.h>

// Optimized memcpy using chunked system memcpy - target <30ms
void* memcpy_a55_optimized(void* __restrict dst, const void* __restrict src, size_t size) {
    // For 6.2MB video frames, use chunked copying to optimize cache usage
    const size_t CHUNK_SIZE = 64 * 1024; // 64KB chunks - optimal for L2 cache
    
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    
    // Process in 64KB chunks
    while (size >= CHUNK_SIZE) {
        memcpy(d, s, CHUNK_SIZE);
        d += CHUNK_SIZE;
        s += CHUNK_SIZE;
        size -= CHUNK_SIZE;
    }
    
    // Handle remaining data
    if (size > 0) {
        memcpy(d, s, size);
    }
    
    return dst;
}

// Usage examples for different formats:

// Full HD RGB24: 1920 * 1080 * 3 = 6,220,800 bytes
void copy_fhd_rgb24(void* dst, const void* src) {
    memcpy_a55_optimized(dst, src, 1920 * 1080 * 3);
}

// VGA NV12: 640 * 480 * 1.5 = 460,800 bytes
void copy_vga_nv12(void* dst, const void* src) {
    memcpy_a55_optimized(dst, src, 640 * 480 * 3 / 2);
}

// 4K RGB24: 3840 * 2160 * 3 = 24,883,200 bytes
void copy_4k_rgb24(void* dst, const void* src) {
    memcpy_a55_optimized(dst, src, 3840 * 2160 * 3);
}

// Generic frame copy with size calculation
void copy_frame_rgb24(void* dst, const void* src, int width, int height) {
    memcpy_a55_optimized(dst, src, width * height * 3);
}

void copy_frame_nv12(void* dst, const void* src, int width, int height) {
    memcpy_a55_optimized(dst, src, width * height * 3 / 2);
}

