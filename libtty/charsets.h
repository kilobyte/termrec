extern unsigned short charset_vt100[128];
extern unsigned short charset_cp437[256];

// tf8, the "tf" in "utf8"
#define tf8(vb, uv)     do                      \
{                                               \
    if ((uv)<0x80)                              \
    {                                           \
        *(vb)++=(uv);                           \
    }                                           \
    else if ((uv) < 0x800) {                    \
        *(vb)++ = ( (uv) >>  6)         | 0xc0; \
        *(vb)++ = ( (uv)        & 0x3f) | 0x80; \
    }                                           \
    else if ((uv) < 0x10000) {                  \
        *(vb)++ = ( (uv) >> 12)         | 0xe0; \
        *(vb)++ = (((uv) >>  6) & 0x3f) | 0x80; \
        *(vb)++ = ( (uv)        & 0x3f) | 0x80; \
    }                                           \
    else /*if ((uv) < 0x200000)*/ {             \
        *(vb)++ = ( (uv) >> 18)         | 0xf0; \
        *(vb)++ = (((uv) >> 12) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >>  6) & 0x3f) | 0x80; \
        *(vb)++ = ( (uv)        & 0x3f) | 0x80; \
    }                                           \
} while (0)
#if 0 // we do only Unicode, not full TF8
    else if ((uv) < 0x4000000) {                \
        *(vb)++ = ( (uv) >> 24)         | 0xf8; \
        *(vb)++ = (((uv) >> 18) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >> 12) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >>  6) & 0x3f) | 0x80; \
        *(vb)++ = ( (uv)        & 0x3f) | 0x80; \
    }                                           \
    else if ((uv) < 0x80000000) {               \
        *(vb)++ = ( (uv) >> 30)         | 0xfc; \
        *(vb)++ = (((uv) >> 24) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >> 18) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >> 12) & 0x3f) | 0x80; \
        *(vb)++ = (((uv) >>  6) & 0x3f) | 0x80; \
        *(vb)++ = ( (uv)        & 0x3f) | 0x80; \
    }
#endif
