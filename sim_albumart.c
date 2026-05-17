/**
 * sim_albumart.c — minimal ID3v2 APIC parser + stb_image JPEG/PNG decoder
 *
 * Supports ID3v2.3 and ID3v2.4 (the two most common variants).
 * Only reads the first APIC frame found (any picture type).
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "sim_albumart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── helpers ────────────────────────────────────────────────────────────── */

static uint32_t synchsafe(const uint8_t *b)
{
    return ((uint32_t)b[0] << 21) | ((uint32_t)b[1] << 14)
         | ((uint32_t)b[2] <<  7) |  (uint32_t)b[3];
}

static uint32_t be32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
         | ((uint32_t)b[2] <<  8) |  (uint32_t)b[3];
}

/* ── public ─────────────────────────────────────────────────────────────── */

bool sim_albumart_load(const char *mp3_path,
                       uint8_t   **rgba_out,
                       int        *w_out,
                       int        *h_out)
{
    *rgba_out = NULL;
    *w_out = *h_out = 0;

    printf("[ALBUMART] loading from: %s\n", mp3_path);

    FILE *f = fopen(mp3_path, "rb");
    if (!f) { printf("[ALBUMART] fopen failed\n"); return false; }

    /* ── Read ID3v2 header (10 bytes) ─────────────────────────────── */
    uint8_t hdr[10];
    if (fread(hdr, 1, 10, f) != 10) {
        printf("[ALBUMART] too short to read header\n");
        fclose(f); return false;
    }

    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') {
        printf("[ALBUMART] no ID3v2 tag (magic: %02x %02x %02x)\n",
               hdr[0], hdr[1], hdr[2]);
        fclose(f);
        return false;
    }

    int  version    = hdr[3];
    bool has_ext    = (hdr[5] & 0x40) != 0;
    uint32_t taglen = synchsafe(hdr + 6);
    printf("[ALBUMART] ID3v2.%d tag, size=%u bytes, ext=%d\n",
           version, taglen, has_ext);

    if (taglen > 32 * 1024 * 1024) {
        printf("[ALBUMART] tag too large (%u), aborting\n", taglen);
        fclose(f); return false;
    }

    uint8_t *tag = malloc(taglen);
    if (!tag) { printf("[ALBUMART] malloc failed\n"); fclose(f); return false; }

    size_t got = fread(tag, 1, taglen, f);
    if (got != taglen) {
        printf("[ALBUMART] short read: got %zu / %u\n", got, taglen);
        free(tag); fclose(f); return false;
    }
    fclose(f);

    /* ── Skip extended header if present ─────────────────────────── */
    uint32_t pos = 0;
    if (has_ext && version >= 3) {
        if (pos + 4 > taglen) { free(tag); return false; }
        uint32_t ext_size = (version == 4) ? synchsafe(tag + pos)
                                           : be32(tag + pos);
        pos += ext_size;
    }

    /* ── Walk frames ──────────────────────────────────────────────── */
    int frame_count = 0;
    while (pos + 10 <= taglen) {
        if (tag[pos] == 0) { printf("[ALBUMART] padding at pos=%u\n", pos); break; }

        char fid[5] = {0};
        memcpy(fid, tag + pos, 4);
        pos += 4;

        if (pos + 6 > taglen) break;
        uint32_t fsz = (version >= 4) ? synchsafe(tag + pos)
                                      : be32(tag + pos);
        pos += 4;
        pos += 2; /* flags */

        frame_count++;
        if (strcmp(fid, "APIC") == 0)
            printf("[ALBUMART] found APIC frame, fsz=%u at pos=%u\n", fsz, pos);

        if (pos + fsz > taglen) {
            printf("[ALBUMART] frame %s overruns tag boundary, stopping\n", fid);
            break;
        }

        if (strcmp(fid, "APIC") == 0 && fsz > 4) {
            uint32_t fp  = pos;
            uint32_t end = pos + fsz;

            /* text encoding */
            uint8_t enc = tag[fp++];
            printf("[ALBUMART] APIC enc=%u\n", enc);

            /* MIME type */
            uint32_t mime_start = fp;
            while (fp < end && tag[fp] != 0) fp++;
            printf("[ALBUMART] APIC mime='%.*s'\n", (int)(fp - mime_start), tag + mime_start);
            if (fp >= end) { printf("[ALBUMART] APIC mime not null-terminated\n"); goto next_frame; }
            fp++; /* skip null */

            /* picture type byte */
            uint8_t pic_type = tag[fp++];
            printf("[ALBUMART] APIC pic_type=%u, img_offset_before_desc=%u\n", pic_type, fp);

            /* description */
            if (enc == 1 || enc == 2) {
                while (fp + 1 < end &&
                       (tag[fp] != 0 || tag[fp + 1] != 0)) fp += 2;
                fp += 2;
            } else {
                while (fp < end && tag[fp] != 0) fp++;
                fp++;
            }

            uint32_t img_bytes = end - fp;
            printf("[ALBUMART] image data at fp=%u, size=%u bytes\n", fp, img_bytes);

            /* the rest is raw image data */
            if (fp < end) {
                int w, h, comp;
                uint8_t *img = stbi_load_from_memory(tag + fp,
                                                     (int)(end - fp),
                                                     &w, &h, &comp, 4);
                if (!img)
                    printf("[ALBUMART] stbi_load failed: %s\n", stbi_failure_reason());
                free(tag);
                if (img) {
                    printf("[ALBUMART] decoded OK: %dx%d comp=%d\n", w, h, comp);
                    *rgba_out = img;
                    *w_out    = w;
                    *h_out    = h;
                    return true;
                }
                return false;
            }
        }

next_frame:
        pos += fsz;
    }

    printf("[ALBUMART] walked %d frames, no usable APIC found\n", frame_count);
    free(tag);
    return false;
}
