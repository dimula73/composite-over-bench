#include <QtCore/QString>
#include <QtTest/QtTest>
#include <malloc.h>
#include <stdint.h>

class TestAvxCompositeOverTest : public QObject
{
    Q_OBJECT

public:
    TestAvxCompositeOverTest();

private Q_SLOTS:
    void testPerPixelComposition();
    void testAVXComposition();
    void testAVXCompositionx2();
    void testAVXComposition_linear();
    void testAVXComposition_linearx2();
    void testAVXComposition_linear_uniform_color();
    void testAVXComposition_linear_uniform_color_float_mask();
    void testAVXComposition_float_mask_unpackpack();

    void testAVXCompositionAlphaDarken_linear();

    void testCudaOver();
    void testCudaDataTransfers();
    void testMemcpy();
private:
    void compareMethods();
};

TestAvxCompositeOverTest::TestAvxCompositeOverTest()
{
}

const int alpha_pos = 3;

void generateDataLine(uint seed, int numPixels, quint8 *srcPixels, quint8 *dstPixels, quint8 *mask)
{
    Q_ASSERT(numPixels >= 4);

    for (int i = 0; i < 4; i++) {
        srcPixels[4*i]   = i * 10 + 30;
        srcPixels[4*i+1] = i * 10 + 30;
        srcPixels[4*i+2] = i * 10 + 30;
        srcPixels[4*i+3] = i * 10 + 35;

        dstPixels[4*i]   = i * 10 + 160;
        dstPixels[4*i+1] = i * 10 + 160;
        dstPixels[4*i+2] = i * 10 + 160;
        dstPixels[4*i+3] = i * 10 + 165;

        mask[i] = i * 10 + 225;
    }

    qsrand(seed);
    numPixels -= 4;
    srcPixels += 4 * 4;
    dstPixels += 4 * 4;
    mask += 4;

    for (int i = 0; i < numPixels; i++) {
        for (int j = 0; j < 4; j++) {
            *(srcPixels++) = 50 + qrand() % 205;
            *(dstPixels++) = 50 + qrand() % 205;
        }
        *(mask++) = 50 + qrand() % 205;
    }
}

void printData(int numPixels, quint8 *srcPixels, quint8 *dstPixels, quint8 *mask)
{
    for (int i = 0; i < numPixels; i++) {
        qDebug() << "Src: "
                 << srcPixels[i*4] << "\t"
                 << srcPixels[i*4+1] << "\t"
                 << srcPixels[i*4+2] << "\t"
                 << srcPixels[i*4+3] << "\t"
                 << "Msk:" << mask[i];

        qDebug() << "Dst: "
                 << dstPixels[i*4] << "\t"
                 << dstPixels[i*4+1] << "\t"
                 << dstPixels[i*4+2] << "\t"
                 << dstPixels[i*4+3];
    }
}

//#define DEBUG_VALUES
#ifdef DEBUG_VALUES

void print64(const char *prefix, quint8 *p)
{
    printf("%s ", prefix);
    for (int i = 0; i < 8; i++) {
        printf("0x%hhX ", p[i]);
    }
    printf("\n");
}

void print128(const char *prefix, quint8 *p)
{
    printf("%s ", prefix);
    for (int i = 0; i < 16; i++) {
        printf("0x%hhX ", p[i]);
    }
    printf("\n");
}

void print256(const char *prefix, quint8 *p)
{
    printf("%s ", prefix);
    for (int i = 0; i < 32; i++) {
        printf("0x%hhX ", p[i]);
    }
    printf("\n");
}

#else
#define print64(prefix, p)
#define print128(prefix, p)
#define print256(prefix, p)
#endif

void print256ov(const char *prefix, quint8 *p)
{
    printf("%s ", prefix);
    for (int i = 0; i < 32; i++) {
        printf("0x%hhX ", p[i]);
    }
    printf("\n");
}

inline quint8 multLegacy(quint8 a, quint8 b, quint8 c)
{
    uint t = a * b * c + 0x7F5B;
    return ((t >> 7) + t) >> 16;
}

inline quint8 multLegacy(quint8 a, quint8 b)
{
    uint c = a * b + 0x80u;
    return ((c >> 8) + c) >> 8;
}

inline quint8 divideLegacy(quint8 a, quint8 b)
{
    uint c = (a * UINT8_MAX + (b / 2u)) / b;
    return c;
}

inline quint8 blendLegacy(quint8 a, quint8 b, quint8 alpha)
{
    int c = (int(a) - int(b)) * alpha + 0x80u;
    c = ((c >> 8) + c) >> 8;
    return c + b;
}

inline void compositePixelLegacy(quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    quint8 srcAlpha = src[alpha_pos];
    srcAlpha = multLegacy(*mask, srcAlpha, opacity);
    quint8 dstAlpha = dst[alpha_pos];
    quint8 newAlpha = dstAlpha + multLegacy(UINT8_MAX - dstAlpha, srcAlpha);
    quint8 srcBlend = divideLegacy(srcAlpha, newAlpha);

    quint8 *src2 = src+4, *dst2 = dst + 4, *mask2 = mask + 1;
    quint8 srcAlpha2 = src2[alpha_pos];
    srcAlpha2 = multLegacy(*mask2, srcAlpha2, opacity);

    dst[0] = blendLegacy(src[0], dst[0], srcBlend);

    quint8 dstAlpha2 = dst2[alpha_pos];
    quint8 newAlpha2 = dstAlpha2 + multLegacy(UINT8_MAX - dstAlpha2, srcAlpha2);

    dst[1] = blendLegacy(src[1], dst[1], srcBlend);

    quint8 srcBlend2 = divideLegacy(srcAlpha2, newAlpha2);




    dst2[0] = blendLegacy(src2[0], dst2[0], srcBlend2);
    dst2[1] = blendLegacy(src2[1], dst2[1], srcBlend2);

    dst[2] = blendLegacy(src[2], dst[2], srcBlend);
    dst[alpha_pos] = newAlpha;


    dst2[2] = blendLegacy(src2[2], dst2[2], srcBlend2);
    dst2[alpha_pos] = newAlpha2;
}

typedef int     v8si  __attribute__ ((vector_size (32)));  /* int[8],    AVX  */
typedef unsigned int  v8sui  __attribute__ ((vector_size (32)));  /* int[8],    AVX  */
typedef float   v8sf  __attribute__ ((vector_size (32)));  /* float[8],  AVX  */
typedef double  v4df  __attribute__ ((vector_size (32)));  /* double[4], AVX  */
typedef float   v4sf  __attribute__ ((vector_size (16)));  /* float[4],  SSE  */
typedef int     v4si  __attribute__ ((vector_size (16)));  /* int[4],    SSE  */
typedef double  v2df  __attribute__ ((vector_size (16)));  /* double[2], SSE2 */
typedef int     v2si  __attribute__ ((vector_size (8)));   /* int[2],    MMX  */
typedef short   v4hi  __attribute__ ((vector_size (8)));   /* short[4],  MMX  */
typedef short   v8hi  __attribute__ ((vector_size (16)));   /* short[8],    */
typedef char    v8qi  __attribute__ ((vector_size (8)));   /* char[8],   MMX  */
typedef char    v16qi  __attribute__ ((vector_size (16)));   /* char[16],      */


inline v8sf repeat_byte_operand(quint8 byte) {
    float value = byte;

    return __builtin_ia32_vbroadcastss256(&value);
}

inline v8sf load_8_bytes(quint8 *bytes) {
    // check order of loading/shuffling

    quint8 buf[16] __attribute__ ((aligned (16)));
    *((int*)buf) = *((int*)(bytes+4));

    v4si bytes_0 = __builtin_ia32_pmovzxbd128(*((v16qi*)bytes));
    v4sf bytes_0f = __builtin_ia32_cvtdq2ps(bytes_0);

    v4si bytes_1 = __builtin_ia32_pmovzxbd128(*((v16qi*)(buf)));
    v4sf bytes_1f = __builtin_ia32_cvtdq2ps(bytes_1);

    return __builtin_ia32_vinsertf128_ps256(*((v8sf*)&bytes_0f), bytes_1f, 1);
}

inline v8sf fetch_8_alpha(quint8 *bytes) {
    __attribute__ ((aligned (32)))
        static const char fetchAlphaMask[16] = {0x03, 0x80, 0x80, 0x80,
                                                0x07, 0x80, 0x80, 0x80,
                                                0x0B, 0x80, 0x80, 0x80,
                                                0x0F, 0x80, 0x80, 0x80};

    v16qi mask = *((v16qi*)fetchAlphaMask);

    v16qi data_0 = __builtin_ia32_pshufb128(*((v16qi*) bytes    ), mask);
    v16qi data_1 = __builtin_ia32_pshufb128(*((v16qi*)(bytes+16)), mask);



    quint8 data_f[32] __attribute__ ((aligned (32)));

    // use 256 version instead
    *((v4sf*)data_f) = __builtin_ia32_cvtdq2ps((v4si)data_0);
    *((v4sf*)(data_f+16)) = __builtin_ia32_cvtdq2ps((v4si)data_1);

    return *((v8sf*)data_f);
}

inline v8sf expand_blend(quint8 *blend) {
    float b_0 = ((float*)blend)[0];
    float b_1 = ((float*)blend)[1];

    v8sf hi = __builtin_ia32_vbroadcastss256(&b_1);
    v4sf lo = __builtin_ia32_vbroadcastss(&b_0);

    return __builtin_ia32_vinsertf128_ps256(hi, lo, 0);
}

inline v8sf process_2_pixels(quint8 *src, quint8 *dst, quint8 *blend)
{
    v8sf blend_0 = expand_blend(blend);

    v8sf src_colors_0 = load_8_bytes(src);
    v8sf dst_colors_0 = load_8_bytes(dst);

    v8sf res_0 = blend_0 * (src_colors_0 - dst_colors_0) + dst_colors_0;

    return res_0;
}

inline void write_2_pixels(quint8 *dst, v8sf colors, quint8 *alpha)
{
    quint8 a_0 = ((float*)alpha)[0];
    quint8 a_1 = ((float*)alpha)[1];

    v8si colors_i = __builtin_ia32_cvtps2dq256(colors);

    v8hi packed_words = __builtin_ia32_packusdw128(*((v4si*)&colors_i), *(((v4si*)&colors_i)+1));
    v8qi packed_bytes = __builtin_ia32_packuswb(*((v4hi*)&packed_words), *(((v4hi*)&packed_words)+1));

    *((v8qi*)dst) = packed_bytes;

    dst[3] = a_0;
    dst[7] = a_1;
}

__attribute__ ((aligned (32)))
static const char uint8Max_val[32] = {0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43,
                                      0x0, 0x0, 0x7F, 0x43};
__attribute__ ((aligned (32)))
static const char uint8MaxRec1_val[32] = {0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B,
                                          0x81, 0x80, 0x80, 0x3B};

__attribute__ ((aligned (32)))
static const char uint8MaxRec2_val[32] = {0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37,
                                          0x82, 0x1, 0x81, 0x37};

inline void compositePixelsAVX(quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    v8sf srcA = fetch_8_alpha(src);
    v8sf dstA = fetch_8_alpha(dst);

    v8sf maskA = load_8_bytes(mask);
    //print256("maskA     ", (quint8*)&maskA);

    v8sf opacityA = repeat_byte_operand(opacity);

    // save the const
    //v8sf uint8Max = repeat_byte_operand(UINT8_MAX);

    v8sf uint8Max = *((v8sf*)uint8Max_val);
    v8sf uint8MaxRec1 = *((v8sf*)uint8MaxRec1_val);
    v8sf uint8MaxRec2 = *((v8sf*)uint8MaxRec2_val);


    srcA *= maskA * opacityA * uint8MaxRec2;

    v8sf newA = dstA + (uint8Max - dstA) * srcA * uint8MaxRec1;

    v8sf recNewA = __builtin_ia32_rcpps256(newA);
    v8sf srcBlend = srcA * recNewA;
    //v8sf srcBlend = srcA / newA; // normalized by 1

    quint8 *b = (quint8*)&srcBlend;
    quint8 *a = (quint8*)&newA;

    v8sf colors = process_2_pixels(src, dst, b);
    v8sf colors1 = process_2_pixels(src+8, dst+8, b+8);
    v8sf colors2 = process_2_pixels(src+16, dst+16, b+16);
    v8sf colors3 = process_2_pixels(src+24, dst+24, b+24);
    write_2_pixels(dst, colors, a);
    write_2_pixels(dst+8, colors1, a+8);
    write_2_pixels(dst+16, colors2, a+16);
    write_2_pixels(dst+24, colors3, a+24);
}

#include <Vc/Vc>
#include <Vc/IO>


inline Vc::float_v fetch_mask_8(quint8 *data) {
    Vc::uint_v data_i(data);
    return Vc::float_v(data_i);
}

inline Vc::float_v fetch_alpha_8(quint8 *data) {
    Vc::uint_v data_i((quint32*)data);
    return Vc::float_v(data_i >> 24);
}

inline void fetch_colors_8(quint8 *data,
                           Vc::float_v &c1,
                           Vc::float_v &c2,
                           Vc::float_v &c3) {

    Vc::uint_v data_i((quint32*)data);

    quint32 lowByteMask = 0xFF;
    Vc::uint_v mask(lowByteMask);

    c1 = Vc::float_v((data_i >> 16) & mask);
    c2 = Vc::float_v((data_i >> 8)  & mask);
    c3 = Vc::float_v( data_i        & mask);
}

inline void write_channels_8(quint8 *data,
                             Vc::float_v alpha,
                             Vc::float_v c1,
                             Vc::float_v c2,
                             Vc::float_v c3) {

    quint32 lowByteMask = 0xFF;
    Vc::uint_v mask(lowByteMask);

    Vc::uint_v v1 = Vc::uint_v(alpha) << 24;
    Vc::uint_v v2 = (Vc::uint_v(c1) & mask) << 16;
    Vc::uint_v v3 = (Vc::uint_v(c2) & mask) <<  8;
    v1 = v1 | v2;
    Vc::uint_v v4 = Vc::uint_v(c3) & mask;
    v3 = v3 | v4;

    *((Vc::uint_v*)data) = v1 | v3;
}

inline Vc::uint_v write_channels_8_ret(quint8 *data,
                                       Vc::float_v alpha,
                                       Vc::float_v c1,
                                       Vc::float_v c2,
                                       Vc::float_v c3) {

    quint32 lowByteMask = 0xFF;
    Vc::uint_v mask(lowByteMask);

    Vc::uint_v v1 = Vc::uint_v(alpha) << 24;
    Vc::uint_v v2 = (Vc::uint_v(c1) & mask) << 16;
    Vc::uint_v v3 = (Vc::uint_v(c2) & mask) <<  8;
    v1 = v1 | v2;
    Vc::uint_v v4 = Vc::uint_v(c3) & mask;
    v3 = v3 | v4;

    return v1 | v3;
}

__attribute__ ((always_inline))
inline void compositePixelsAVXAlphaDarken_linear(quint8 *src, quint8 *dst, quint8 *mask, float opacity, float flow)
{
    Vc::float_v src_alpha;
    Vc::float_v dst_alpha;

    Vc::float_v opacity_vec(255.0 * opacity * flow);
    Vc::float_v flow_norm_vec(flow);



    Vc::float_v uint8MaxRec2((float)1.0/(255.0 * 255.0));
    Vc::float_v uint8MaxRec1((float)1.0/255.0);

    src_alpha = fetch_alpha_8(src);
    Vc::float_v mask_vec = fetch_mask_8(mask);
    Vc::float_v msk_norm_alpha = src_alpha * mask_vec * uint8MaxRec2;

    dst_alpha = fetch_alpha_8(dst);
    src_alpha = msk_norm_alpha * opacity_vec;


    Vc::float_v src_c1;
    Vc::float_v src_c2;
    Vc::float_v src_c3;

    Vc::float_v dst_c1;
    Vc::float_v dst_c2;
    Vc::float_v dst_c3;


    fetch_colors_8(src, src_c1, src_c2, src_c3);
    Vc::float_v dst_blend = src_alpha * uint8MaxRec1;

    Vc::float_v alpha1 = src_alpha + dst_alpha -
        dst_blend * dst_alpha;
    fetch_colors_8(dst, dst_c1, dst_c2, dst_c3);

    // TODO: if (dstAlpha == 0) dstC = srcC;
    dst_c1 = dst_blend * (src_c1 - dst_c1) + dst_c1;
    dst_c2 = dst_blend * (src_c2 - dst_c2) + dst_c2;
    dst_c3 = dst_blend * (src_c3 - dst_c3) + dst_c3;


    Vc::float_m alpha2_mask = opacity > dst_alpha;
    Vc::float_v opt1 = (opacity_vec - dst_alpha) * msk_norm_alpha + dst_alpha;
    Vc::float_v alpha2;
    alpha2(!alpha2_mask) = dst_alpha;
    alpha2(alpha2_mask) = opt1;

    dst_alpha = (alpha2 - alpha1) * flow_norm_vec + alpha1;

    write_channels_8(dst, dst_alpha, dst_c1, dst_c2, dst_c3);

//    src = dst;
//    qDebug() << "int  :" << src[0] << src[1] << src[2] << src[3] << src[4] << src[5] << src[6] << src[7];
//    float *f = (float*)&dst_c3;
//    qDebug() << "float:" << f[0] << f[1] << f[2] << f[3] << f[4] << f[5] << f[6] << f[7];
}

inline void compositePixelsAVX_linear(quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    Vc::float_v src_alpha;
    Vc::float_v dst_alpha;

    src_alpha = fetch_alpha_8(src);

    Vc::float_v mask_vec = fetch_mask_8(mask);
    Vc::float_v opacity_vec(opacity);

    Vc::float_v uint8MaxRec2((float*)uint8MaxRec2_val);
    Vc::float_v uint8Max((float*)uint8Max_val);
    Vc::float_v uint8MaxRec1((float*)uint8MaxRec1_val);

    src_alpha *= mask_vec * opacity_vec * uint8MaxRec2;
    dst_alpha = fetch_alpha_8(dst);

    Vc::float_v src_c1;
    Vc::float_v src_c2;
    Vc::float_v src_c3;

    Vc::float_v dst_c1;
    Vc::float_v dst_c2;
    Vc::float_v dst_c3;

    Vc::float_v new_alpha =
        dst_alpha + (uint8Max - dst_alpha) * src_alpha * uint8MaxRec1;
    fetch_colors_8(src, src_c1, src_c2, src_c3);

    Vc::float_v src_blend = src_alpha * Vc::reciprocal(new_alpha);
    //Vc::float_v src_blend = src_alpha / new_alpha;

    fetch_colors_8(dst, dst_c1, dst_c2, dst_c3);

    dst_c1 = src_blend * (src_c1 - dst_c1) + dst_c1;
    dst_c2 = src_blend * (src_c2 - dst_c2) + dst_c2;
    dst_c3 = src_blend * (src_c3 - dst_c3) + dst_c3;

    write_channels_8(dst, new_alpha, dst_c1, dst_c2, dst_c3);

//    src = dst;
//    qDebug() << "int  :" << src[0] << src[1] << src[2] << src[3] << src[4] << src[5] << src[6] << src[7];
//    float *f = (float*)&dst_c3;
//    qDebug() << "float:" << f[0] << f[1] << f[2] << f[3] << f[4] << f[5] << f[6] << f[7];
}

inline void compositePixelsAVX_linear_x2(quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    Vc::float_v src_alpha;
    Vc::float_v dst_alpha;

    src_alpha = fetch_alpha_8(src);

    Vc::float_v mask_vec = fetch_mask_8(mask);
    Vc::float_v opacity_vec(opacity);

    Vc::float_v uint8MaxRec2((float*)uint8MaxRec2_val);
    Vc::float_v uint8Max((float*)uint8Max_val);
    Vc::float_v uint8MaxRec1((float*)uint8MaxRec1_val);

    src_alpha *= mask_vec;

    quint8 *src2 = src + 32;
    quint8 *dst2 = dst + 32;
    quint8 *mask2 = mask + 8;
    Vc::float_v src_alpha2;
    Vc::float_v dst_alpha2;
    src_alpha2 = fetch_alpha_8(src2);

    src_alpha *= opacity_vec * uint8MaxRec2;
    dst_alpha = fetch_alpha_8(dst);

    Vc::float_v mask_vec2 = fetch_mask_8(mask2);
    src_alpha2 *= mask_vec2;
    src_alpha2 *= opacity_vec * uint8MaxRec2;
    dst_alpha2 = fetch_alpha_8(dst2);


    Vc::float_v src_c1;
    Vc::float_v src_c2;
    Vc::float_v src_c3;

    Vc::float_v dst_c1;
    Vc::float_v dst_c2;
    Vc::float_v dst_c3;

    Vc::float_v new_alpha =
        dst_alpha + (uint8Max - dst_alpha) * src_alpha * uint8MaxRec1;
    fetch_colors_8(src, src_c1, src_c2, src_c3);


    Vc::float_v src_c12;
    Vc::float_v src_c22;
    Vc::float_v src_c32;

    Vc::float_v dst_c12;
    Vc::float_v dst_c22;
    Vc::float_v dst_c32;

    Vc::float_v new_alpha2 =
        dst_alpha2 + (uint8Max - dst_alpha2) * src_alpha2 * uint8MaxRec1;
    fetch_colors_8(src2, src_c12, src_c22, src_c32);


    Vc::float_v src_blend = src_alpha * Vc::reciprocal(new_alpha);
    //Vc::float_v src_blend = src_alpha / new_alpha;
    fetch_colors_8(dst, dst_c1, dst_c2, dst_c3);

    Vc::float_v src_blend2 = src_alpha2 * Vc::reciprocal(new_alpha2);
    //Vc::float_v src_blend2 = src_alpha2 / new_alpha2;
    fetch_colors_8(dst2, dst_c12, dst_c22, dst_c32);



    dst_c1 = src_blend * (src_c1 - dst_c1) + dst_c1;
    dst_c2 = src_blend * (src_c2 - dst_c2) + dst_c2;
    dst_c3 = src_blend * (src_c3 - dst_c3) + dst_c3;
    dst_c12 = src_blend2 * (src_c12 - dst_c12) + dst_c12;
    dst_c22 = src_blend2 * (src_c22 - dst_c22) + dst_c22;
    dst_c32 = src_blend2 * (src_c32 - dst_c32) + dst_c32;


    Vc::uint_v v1 = write_channels_8_ret(dst, new_alpha, dst_c1, dst_c2, dst_c3);
    Vc::uint_v v2 = write_channels_8_ret(dst2, new_alpha2, dst_c12, dst_c22, dst_c32);

    *((Vc::uint_v*)dst) = v1;
    *((Vc::uint_v*)dst2) = v2;

//    src = dst;
//    qDebug() << "int  :" << src[0] << src[1] << src[2] << src[3] << src[4] << src[5] << src[6] << src[7];
//    float *f = (float*)&dst_c3;
//    qDebug() << "float:" << f[0] << f[1] << f[2] << f[3] << f[4] << f[5] << f[6] << f[7];
}

inline void compositePixelsAVX_linear_uniform_color(Vc::float_v src_alpha,
                                                    Vc::float_v src_c1,
                                                    Vc::float_v src_c2,
                                                    Vc::float_v src_c3,
                                                    quint8 *dst, quint8 *mask, quint8 opacity)
{
    Vc::float_v dst_alpha;

    Vc::float_v mask_vec = fetch_mask_8(mask);
    Vc::float_v opacity_vec(opacity);

    Vc::float_v uint8MaxRec2((float*)uint8MaxRec2_val);
    Vc::float_v uint8Max((float*)uint8Max_val);
    Vc::float_v uint8MaxRec1((float*)uint8MaxRec1_val);

    src_alpha *= mask_vec * opacity_vec * uint8MaxRec2;
    dst_alpha = fetch_alpha_8(dst);

    Vc::float_v dst_c1;
    Vc::float_v dst_c2;
    Vc::float_v dst_c3;

    Vc::float_v new_alpha =
        dst_alpha + (uint8Max - dst_alpha) * src_alpha * uint8MaxRec1;

    Vc::float_v src_blend = src_alpha * Vc::reciprocal(new_alpha);
    //Vc::float_v src_blend = src_alpha / new_alpha;

    fetch_colors_8(dst, dst_c1, dst_c2, dst_c3);

    dst_c1 = src_blend * (src_c1 - dst_c1) + dst_c1;
    dst_c2 = src_blend * (src_c2 - dst_c2) + dst_c2;
    dst_c3 = src_blend * (src_c3 - dst_c3) + dst_c3;

    write_channels_8(dst, new_alpha, dst_c1, dst_c2, dst_c3);

//    src = dst;
//    qDebug() << "int  :" << src[0] << src[1] << src[2] << src[3] << src[4] << src[5] << src[6] << src[7];
//    float *f = (float*)&dst_c3;
//    qDebug() << "float:" << f[0] << f[1] << f[2] << f[3] << f[4] << f[5] << f[6] << f[7];
}

inline void compositePixelsAVX_linear_uniform_color_float_mask(Vc::float_v src_alpha,
                                                               Vc::float_v src_c1,
                                                               Vc::float_v src_c2,
                                                               Vc::float_v src_c3,
                                                               quint8 *dst, float *mask, quint8 opacity)
{
    Vc::float_v dst_alpha;

    Vc::float_v mask_vec(mask);
    Vc::float_v opacity_vec(opacity);

    Vc::float_v uint8MaxRec2((float*)uint8MaxRec2_val);
    Vc::float_v uint8Max((float*)uint8Max_val);
    Vc::float_v uint8MaxRec1((float*)uint8MaxRec1_val);

    src_alpha *= mask_vec * opacity_vec * uint8MaxRec2;
    dst_alpha = fetch_alpha_8(dst);

    Vc::float_v dst_c1;
    Vc::float_v dst_c2;
    Vc::float_v dst_c3;

    Vc::float_v new_alpha =
        dst_alpha + (uint8Max - dst_alpha) * src_alpha * uint8MaxRec1;

    Vc::float_v src_blend = src_alpha * Vc::reciprocal(new_alpha);
    //Vc::float_v src_blend = src_alpha / new_alpha;

    fetch_colors_8(dst, dst_c1, dst_c2, dst_c3);

    dst_c1 = src_blend * (src_c1 - dst_c1) + dst_c1;
    dst_c2 = src_blend * (src_c2 - dst_c2) + dst_c2;
    dst_c3 = src_blend * (src_c3 - dst_c3) + dst_c3;

    write_channels_8(dst, new_alpha, dst_c1, dst_c2, dst_c3);

//    src = dst;
//    qDebug() << "int  :" << src[0] << src[1] << src[2] << src[3] << src[4] << src[5] << src[6] << src[7];
//    float *f = (float*)&dst_c3;
//    qDebug() << "float:" << f[0] << f[1] << f[2] << f[3] << f[4] << f[5] << f[6] << f[7];
}

inline void compositePixelsAVXx2(quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    v8sf srcA = fetch_8_alpha(src);
    v8sf dstA = fetch_8_alpha(dst);
    v8sf maskA = load_8_bytes(mask);
    v8sf opacityA = repeat_byte_operand(opacity);

    // save the const
    //v8sf uint8Max = repeat_byte_operand(UINT8_MAX);
    v8sf uint8Max = *((v8sf*)uint8Max_val);
    v8sf uint8MaxRec1 = *((v8sf*)uint8MaxRec1_val);
    v8sf uint8MaxRec2 = *((v8sf*)uint8MaxRec2_val);


    srcA *= maskA;

    quint8 *src2 = src + 32;
    quint8 *dst2 = dst + 32;
    quint8 *mask2 = mask + 8;
    v8sf srcA2 = fetch_8_alpha(src2);

    srcA *= opacityA * uint8MaxRec2;

    v8sf dstA2 = fetch_8_alpha(dst2);

    v8sf newA = dstA + (uint8Max - dstA) * srcA * uint8MaxRec1;
    v8sf recNewA = __builtin_ia32_rcpps256(newA);
    v8sf srcBlend = srcA * recNewA;
    //v8sf srcBlend = srcA / newA; // normalized by 1


    v8sf maskA2 = load_8_bytes(mask2);
    srcA2 *= maskA2 * opacityA * uint8MaxRec2;

    quint8 *b = (quint8*)&srcBlend;
    quint8 *a = (quint8*)&newA;
    v8sf colors = process_2_pixels(src, dst, b);
    v8sf colors1 = process_2_pixels(src+8, dst+8, b+8);
    v8sf colors2 = process_2_pixels(src+16, dst+16, b+16);
    v8sf colors3 = process_2_pixels(src+24, dst+24, b+24);

    v8sf newA2 = dstA2 + (uint8Max - dstA2) * srcA2 * uint8MaxRec1;
    v8sf recNewA2 = __builtin_ia32_rcpps256(newA2);
    v8sf srcBlend2 = srcA2 * recNewA2;
    //v8sf srcBlend2 = srcA2 / newA2; // normalized by 1
    quint8 *b2 = (quint8*)&srcBlend2;
    quint8 *a2 = (quint8*)&newA2;


    write_2_pixels(dst, colors, a);
    write_2_pixels(dst+8, colors1, a+8);
    write_2_pixels(dst+16, colors2, a+16);
    write_2_pixels(dst+24, colors3, a+24);

    v8sf colors02 = process_2_pixels(src2, dst2, b2);
    v8sf colors12 = process_2_pixels(src2+8, dst2+8, b2+8);
    v8sf colors22 = process_2_pixels(src2+16, dst2+16, b2+16);
    v8sf colors32 = process_2_pixels(src2+24, dst2+24, b2+24);
    write_2_pixels(dst2, colors02, a2);
    write_2_pixels(dst2+8, colors12, a2+8);
    write_2_pixels(dst2+16, colors22, a2+16);
    write_2_pixels(dst2+24, colors32, a2+24);
}

const int numPixels = 32 * 1000000;

void TestAvxCompositeOverTest::testPerPixelComposition()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst = (quint8*)memalign(16, numPixels * 4);
    quint8 *mask = (quint8*)memalign(16, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels/2; i++) {
        compositePixelLegacy(s, d, m, opacity);
        s += 4*2;
        d += 4*2;
        m += 1*2;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testAVXComposition()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst = (quint8*)memalign(16, numPixels * 4);
    quint8 *mask = (quint8*)memalign(16, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVX(s, d, m, opacity);
        s += 32;
        d += 32;
        m += 8;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testAVXCompositionx2()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst = (quint8*)memalign(16, numPixels * 4);
    quint8 *mask = (quint8*)memalign(16, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels / 16; i++) {
        compositePixelsAVXx2(s, d, m, opacity);
        s += 64;
        d += 64;
        m += 16;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testAVXComposition_linear()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVX_linear(s, d, m, opacity);
        s += 32;
        d += 32;
        m += 8;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testAVXComposition_linear_uniform_color()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    Vc::float_v src_alpha(118.0f);
    Vc::float_v src_c1(130.0f);
    Vc::float_v src_c2(131.0f);
    Vc::float_v src_c3(132.0f);

    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVX_linear_uniform_color(src_alpha, src_c1, src_c2, src_c3, d, m, opacity);
        d += 32;
        m += 8;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testAVXComposition_linear_uniform_color_float_mask()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);
    float *floatMask = (float*)memalign(32, numPixels * 4);

    generateDataLine(1, numPixels, src, dst, mask);

    for (int i = 0; i < numPixels; i++) floatMask[i] = mask[i];


//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;
    float *fm = floatMask;

    Vc::float_v src_alpha(118.0f);
    Vc::float_v src_c1(130.0f);
    Vc::float_v src_c2(131.0f);
    Vc::float_v src_c3(132.0f);

    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVX_linear_uniform_color_float_mask(src_alpha, src_c1, src_c2, src_c3, d, fm, opacity);
        d += 32;
        fm += 8;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
    free(floatMask);
}

void TestAvxCompositeOverTest::testAVXComposition_float_mask_unpackpack()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);
    float *floatMask = (float*)memalign(32, numPixels * 4);

    QBENCHMARK_ONCE {
        quint8 *m = mask;
        float *fm = floatMask;

/*        for (int i = 0; i < numPixels / 8; i++) {
            Vc::float_v mm = fetch_mask_8(mask);
            *((Vc::float_v*)floatMask) = mm;
            m += 8;
            fm += 8;
            }*/

        asm ( "mfence" );

        m = mask;
        fm = floatMask;

        for (int i = 0; i < numPixels / 8; i++) {
            Vc::float_v mm(fm);
            Vc::uint_v ii(mm);

            v4si* ii_ptr = (v4si*)&ii;

            v8hi packed_words = __builtin_ia32_packusdw128(ii_ptr[0], ii_ptr[1]);
            v8qi packed_bytes = __builtin_ia32_packuswb(*((v4hi*)&packed_words), *(((v4hi*)&packed_words)+1));
            *((v8qi*)m) = packed_bytes;

//            for (int j = 0; j < 8; j++) {
//                *(m+j) = ii[j];
//            }
            m += 8;
            fm += 8;
        }
    }

    free(src);
    free(dst);
    free(mask);
    free(floatMask);
}

void TestAvxCompositeOverTest::testAVXCompositionAlphaDarken_linear()
{
    float opacity = 0.5; // not const
    float flow = 0.3; // not const as well

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(8, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVXAlphaDarken_linear(s, d, m, opacity, flow);
        s += 32;
        d += 32;
        m += 8;
    }
    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}


void TestAvxCompositeOverTest::testAVXComposition_linearx2()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(8/*numPixels*/, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    for (int i = 0; i < numPixels / 16; i++) {
        compositePixelsAVX_linear_x2(s, d, m, opacity);
        s += 64;
        d += 64;
        m += 16;
    }
    }

//    qDebug() << "After processing:";
//    printData(8/*numPixels*/, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

#include "cuda_code.h"

void TestAvxCompositeOverTest::testCudaOver()
{
        int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(8/*numPixels*/, src, dst, mask);

    int blockSize = 32 * 1000000;
    int numBlocks = numPixels / blockSize;
    int blockRest = numPixels % blockSize;

    initCuda(numPixels);

    QBENCHMARK_ONCE {
        quint8 *s = src;
        quint8 *d = dst;
        quint8 *m = mask;

        for (int i = 0; i < numBlocks; i++) {
            compositePixelsCUDA(blockSize, s, d, m, opacity);
            s += 4 * blockSize;
            d += 4 * blockSize;
            m += blockSize;
        }

        if (blockRest) {
            compositePixelsCUDA(blockRest, s, d, m, opacity);
        }
    }

    freeCuda();

//    qDebug() << "After processing:";
//    printData(8/*numPixels*/, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testCudaDataTransfers()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(8/*numPixels*/, src, dst, mask);

    int blockSize = 32 * 1000000;
    int numBlocks = numPixels / blockSize;
    int blockRest = numPixels % blockSize;

    initCuda(numPixels);

    QBENCHMARK_ONCE {
        quint8 *s = src;
        quint8 *d = dst;
        quint8 *m = mask;

        for (int i = 0; i < numBlocks; i++) {
            compositePixelsCUDADataTransfers(blockSize, s, d, m, opacity);
            s += 4 * blockSize;
            d += 4 * blockSize;
            m += blockSize;
        }

        if (blockRest) {
            compositePixelsCUDADataTransfers(blockRest, s, d, m, opacity);
        }
    }

    freeCuda();

//    qDebug() << "After processing:";
//    printData(8/*numPixels*/, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

void TestAvxCompositeOverTest::testMemcpy()
{
        int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(32, numPixels * 4);
    quint8 *dst = (quint8*)memalign(32, numPixels * 4);
    quint8 *mask = (quint8*)memalign(32, numPixels);

    generateDataLine(1, numPixels, src, dst, mask);

//    qDebug() << "Initial values:";
//    printData(numPixels, src, dst, mask);

    QBENCHMARK_ONCE {
    quint8 *s = src;
    quint8 *d = dst;
    quint8 *m = mask;

    memcpy(d, s, numPixels * 4);

    }

//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst);
    free(mask);
}

inline bool fuzzyCompare(quint8 a, quint8 b, quint8 prec) {
    return qAbs(a-b) <= prec;
}

inline bool comparePixels(quint8 *a, quint8 *b) {
    const quint8 prec = 2;
    return
        fuzzyCompare(a[0], b[0], prec) &&
        fuzzyCompare(a[1], b[1], prec) &&
        fuzzyCompare(a[2], b[2], prec) &&
        fuzzyCompare(a[3], b[3], prec);
}

inline void printPixels(quint8 *a, quint8 *b) {
    qDebug() << a[0] << a[1] << a[2] << a[3];
    qDebug() << b[0] << b[1] << b[2] << b[3];
}

void TestAvxCompositeOverTest::compareMethods()
{
    int opacity = 255; // not const

    quint8 *src = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst1 = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst2 = (quint8*)memalign(16, numPixels * 4);
    quint8 *dst3 = (quint8*)memalign(16, numPixels * 4);
    quint8 *mask = (quint8*)memalign(16, numPixels);

    generateDataLine(1, numPixels, src, dst1, mask);
    memcpy(dst2, dst1, numPixels * 4);
    memcpy(dst3, dst1, numPixels * 4);

    quint8 *s = src;
    quint8 *d = dst1;
    quint8 *m = mask;

    for (int i = 0; i < numPixels; i++) {
        compositePixelLegacy(s, d, m, opacity);
        s += 4;
        d += 4;
        m += 1;
    }

    s = src;
    d = dst2;
    m = mask;
    for (int i = 0; i < numPixels / 8; i++) {
        compositePixelsAVX(s, d, m, opacity);
        s += 32;
        d += 32;
        m += 8;
    }

    s = src;
    d = dst3;
    m = mask;
    for (int i = 0; i < numPixels / 16; i++) {
        compositePixelsAVXx2(s, d, m, opacity);
        s += 64;
        d += 64;
        m += 16;
    }

    for (int i = 0; i < numPixels; i++) {
        if(!comparePixels(dst1 + i * 4, dst2 + i * 4)) {
            qDebug() << i;
            printPixels(dst1 + i * 4, dst2 + i * 4);
            QFAIL("Failed to compare AVX pixels");
        }

        if(!comparePixels(dst1 + i * 4, dst3 + i * 4)) {
            qDebug() << i;
            printPixels(dst1 + i * 4, dst3 + i * 4);
            QFAIL("Failed to compare AVXx2 pixels");
        }
    }


//    qDebug() << "After processing:";
//    printData(numPixels, src, dst, mask);

    free(src);
    free(dst1);
    free(dst2);
    free(dst3);
    free(mask);
}

QTEST_APPLESS_MAIN(TestAvxCompositeOverTest);

#include "tst_testavxcompositeovertest.moc"
