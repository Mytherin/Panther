

static CGBitmapInfo ComputeCGAlphaInfo_RGBA(SkAlphaType at) {
    CGBitmapInfo info = kCGBitmapByteOrder32Big;
    switch (at) {
        case kUnknown_SkAlphaType:
            break;
        case kOpaque_SkAlphaType:
            info |= kCGImageAlphaNoneSkipLast;
            break;
        case kPremul_SkAlphaType:
            info |= kCGImageAlphaPremultipliedLast;
            break;
        case kUnpremul_SkAlphaType:
            info |= kCGImageAlphaLast;
            break;
    }
    return info;
}

static CGBitmapInfo ComputeCGAlphaInfo_BGRA(SkAlphaType at) {
    CGBitmapInfo info = kCGBitmapByteOrder32Little;
    switch (at) {
        case kUnknown_SkAlphaType:
            break;
        case kOpaque_SkAlphaType:
            info |= kCGImageAlphaNoneSkipFirst;
            break;
        case kPremul_SkAlphaType:
            info |= kCGImageAlphaPremultipliedFirst;
            break;
        case kUnpremul_SkAlphaType:
            info |= kCGImageAlphaFirst;
            break;
    }
    return info;
}

static bool getBitmapInfo(const SkBitmap& bm,
                          size_t* bitsPerComponent,
                          CGBitmapInfo* info,
                          bool* upscaleTo32) {
    if (upscaleTo32) {
        *upscaleTo32 = false;
    }

    switch (bm.colorType()) {
        case kRGB_565_SkColorType:
#if 0
            // doesn't see quite right. Are they thinking 1555?
            *bitsPerComponent = 5;
            *info = kCGBitmapByteOrder16Little | kCGImageAlphaNone;
#else
            if (upscaleTo32) {
                *upscaleTo32 = true;
            }
            // now treat like RGBA
            *bitsPerComponent = 8;
            *info = ComputeCGAlphaInfo_RGBA(kOpaque_SkAlphaType);
#endif
            break;
        case kRGBA_8888_SkColorType:
            *bitsPerComponent = 8;
            *info = ComputeCGAlphaInfo_RGBA(bm.alphaType());
            break;
        case kBGRA_8888_SkColorType:
            *bitsPerComponent = 8;
            *info = ComputeCGAlphaInfo_BGRA(bm.alphaType());
            break;
        case kARGB_4444_SkColorType:
            *bitsPerComponent = 4;
            *info = kCGBitmapByteOrder16Little;
            if (bm.isOpaque()) {
                *info |= kCGImageAlphaNoneSkipLast;
            } else {
                *info |= kCGImageAlphaPremultipliedLast;
            }
            break;
        default:
            return false;
    }
    return true;
}

static SkBitmap* prepareForImageRef(const SkBitmap& bm,
                                    size_t* bitsPerComponent,
                                    CGBitmapInfo* info) {
    bool upscaleTo32;
    if (!getBitmapInfo(bm, bitsPerComponent, info, &upscaleTo32)) {
        return nullptr;
    }

    SkBitmap* copy;
    if (upscaleTo32) {
        copy = new SkBitmap;
        // here we make a deep copy of the pixels, since CG won't take our
        // 565 directly
        bm.copyTo(copy, kN32_SkColorType);
    } else {
        copy = new SkBitmap(bm);
    }
    return copy;
}


static void SkBitmap_ReleaseInfo(void* info, const void* pixelData, size_t size) {
    SkBitmap* bitmap = reinterpret_cast<SkBitmap*>(info);
    delete bitmap;
}

CGImageRef SkCreateCGImageRef(const SkBitmap& bm) {
    size_t bitsPerComponent SK_INIT_TO_AVOID_WARNING;
    CGBitmapInfo info       SK_INIT_TO_AVOID_WARNING;

    SkBitmap* bitmap = prepareForImageRef(bm, &bitsPerComponent, &info);
    if (nullptr == bitmap) {
        return nullptr;
    }

    const int w = bitmap->width();
    const int h = bitmap->height();
    const size_t s = bitmap->getSize();

    // our provider "owns" the bitmap*, and will take care of deleting it
    // we initially lock it, so we can access the pixels. The bitmap will be deleted in the release
    // proc, which will in turn unlock the pixels
    bitmap->lockPixels();
    CGDataProviderRef dataRef = CGDataProviderCreateWithData(bitmap, bitmap->getPixels(), s,
                                                             SkBitmap_ReleaseInfo);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGImageRef ref = CGImageCreate(w, h, bitsPerComponent,
                                   bitmap->bytesPerPixel() * 8,
                                   bitmap->rowBytes(), colorSpace, info, dataRef,
                                   nullptr, false, kCGRenderingIntentDefault);

    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(dataRef);
    return ref;
}

void SkCGDrawBitmap(CGContextRef cg, const SkBitmap& bm, float x, float y, float scale_factor) {
    CGImageRef img = SkCreateCGImageRef(bm);

    if (img) {
        CGRect r = CGRectMake(0, 0, bm.width(), bm.height());

        CGContextSaveGState(cg);
        CGContextTranslateCTM(cg, x, y);
        CGContextScaleCTM(cg, 1.0f / scale_factor, 1.0f / scale_factor);

        CGContextDrawImage(cg, r, img);

        CGContextRestoreGState(cg);

        CGImageRelease(img);
    }
}
