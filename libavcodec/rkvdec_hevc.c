/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "hevc_data.h"
#include "hevcdec.h"

#include "rkvdec.h"

typedef struct RKVDECHWRegsHEVC {
    struct swreg_id {
        uint32_t    minor_ver           : 8;
        uint32_t    level               : 1;
        uint32_t    dec_support         : 3;
        uint32_t    profile             : 1;
        uint32_t    reserve0            : 1;
        uint32_t    codec_flag          : 1;
        uint32_t    reserve1            : 1;
        uint32_t    prod_num            : 16;
    } sw_id;

    struct swreg_int {
        uint32_t    sw_dec_e            : 1;
        uint32_t    sw_dec_clkgate_e    : 1;
        uint32_t    reserve0            : 1;
        uint32_t    sw_timeout_mode     : 1;
        uint32_t    sw_dec_irq_dis      : 1;
        uint32_t    sw_dec_timeout_e    : 1;
        uint32_t    sw_buf_empty_en     : 1;
        uint32_t    sw_stmerror_waitdecfifo_empty : 1;
        uint32_t    sw_dec_irq          : 1;
        uint32_t    sw_dec_irq_raw      : 1;
        uint32_t    reserve1            : 2;
        uint32_t    sw_dec_rdy_sta      : 1;
        uint32_t    sw_dec_bus_sta      : 1;
        uint32_t    sw_dec_error_sta    : 1;
        uint32_t    sw_dec_timeout_sta  : 1;
        uint32_t    sw_dec_empty_sta    : 1;
        uint32_t    sw_colmv_ref_error_sta : 1;
        uint32_t    sw_cabu_end_sta     : 1;
        uint32_t    sw_h264orvp9_error_mode : 1;
        uint32_t    sw_softrst_en_p     : 1;
        uint32_t    sw_force_softreset_valid: 1;
        uint32_t    sw_softreset_rdy    : 1;
        uint32_t    sw_wr_ddr_align_en  : 1;
        uint32_t    sw_scl_down_en      : 1;
        uint32_t    sw_allow_not_wr_unref_bframe : 1;
    } sw_interrupt;

    struct swreg_sysctrl {
        uint32_t    sw_in_endian        : 1;
        uint32_t    sw_in_swap32_e      : 1;
        uint32_t    sw_in_swap64_e      : 1;
        uint32_t    sw_str_endian       : 1;
        uint32_t    sw_str_swap32_e     : 1;
        uint32_t    sw_str_swap64_e     : 1;
        uint32_t    sw_out_endian       : 1;
        uint32_t    sw_out_swap32_e     : 1;
        uint32_t    sw_out_cbcr_swap    : 1;
        uint32_t    reserve0            : 1;
        uint32_t    sw_rlc_mode_direct_write : 1;
        uint32_t    sw_rlc_mode         : 1;
        uint32_t    sw_strm_start_bit   : 7;
        uint32_t    reserve1            : 1;
        uint32_t    sw_dec_mode         : 2;
        uint32_t    reserve2            : 2;
        uint32_t    sw_h264_rps_mode    : 1;
        uint32_t    sw_h264_stream_mode : 1;
        uint32_t    sw_h264_stream_lastpacket : 1;
        uint32_t    sw_h264_firstslice_flag : 1;
        uint32_t    sw_h264_frame_orslice : 1;
        uint32_t    sw_buspr_slot_disable : 1;
        uint32_t    reserve3            : 2;
    } sw_sysctrl;

    struct swreg_pic {
        uint32_t    sw_y_hor_virstride  : 9;
        uint32_t    reserve             : 3;
        uint32_t    sw_uv_hor_virstride : 9;
        uint32_t    sw_slice_num        : 8;
    } sw_picparameter;

    uint32_t        sw_strm_rlc_base        ;
    uint32_t        sw_stream_len           ;
    uint32_t        sw_cabactbl_base        ;
    uint32_t        sw_decout_base          ;
    uint32_t        sw_y_virstride          ;
    uint32_t        sw_yuv_virstride        ;
    uint32_t        sw_refer_base[15]       ;
    int32_t         sw_refer_poc[15]        ;
    int32_t         sw_cur_poc              ;
    uint32_t        sw_rlcwrite_base        ;
    uint32_t        sw_pps_base             ;
    uint32_t        sw_rps_base             ;
    uint32_t        cabac_error_en          ;
    uint32_t        cabac_error_status      ;

    struct cabac_error_ctu      {
        uint32_t   sw_cabac_error_ctu_xoffset    : 8;
        uint32_t   sw_cabac_error_ctu_yoffset    : 8;
        uint32_t   sw_streamfifo_space2full      : 7;
        uint32_t   reversed0                     : 9;
    } cabac_error_ctu;

    struct sao_ctu_position     {
        uint32_t   sw_saowr_xoffset              : 9;
        uint32_t   reversed0                     : 7;
        uint32_t   sw_saowr_yoffset             : 10;
        uint32_t   reversed1                     : 6;
    } sao_ctu_position;

    uint32_t        sw_ref_valid            ;   //this is not same with hardware
    uint32_t        sw_refframe_index[15];

    uint32_t        performance_cycle;
    uint32_t        axi_ddr_rdata;
    uint32_t        axi_ddr_wdata;
    uint32_t        fpgadebug_reset;
    uint32_t        reserve[9];

    uint32_t extern_error_en;

} RKVDECHWRegsHEVC;

typedef struct RKVDECPictureHEVC {
    unsigned            slice_count;
    const uint8_t       *bitstream;
    unsigned            bitstream_size;
    RKVDECHWRegsHEVC    hw_regs;
} RKVDECPictureHEVC;

static int rkvdec_hevc_start_frame(AVCodecContext *avctx,
                                  av_unused const uint8_t *buffer,
                                  av_unused uint32_t size)
{
    const HEVCContext *h = avctx->priv_data;
    RKVDECPictureHEVC *pic = h->ref->hwaccel_picture_private;

    av_log(avctx, AV_LOG_DEBUG, "%s: avctx=%p pic=%p\n", __func__, avctx, pic);

    pic->slice_count    = 0;
    pic->bitstream_size = 0;
    pic->bitstream      = NULL;

    return 0;
}

static int rkvdec_hevc_decode_slice(AVCodecContext *avctx,
                                   const uint8_t *buffer,
                                   uint32_t size)
{
    const HEVCContext *h = avctx->priv_data;
    RKVDECPictureHEVC *pic = h->ref->hwaccel_picture_private;

    av_log(avctx, AV_LOG_INFO, "%s: avctx=%p pic=%p slice_count=%u size=%u vps=%p sps=%p pps=%p\n", __func__, avctx, pic, pic->slice_count, size, h->ps.vps, h->ps.sps, h->ps.pps);

    if (!pic->bitstream)
        pic->bitstream = buffer;
    pic->bitstream_size += size;

    pic->slice_count++;

    return 0;
}

static int rkvdec_hevc_end_frame(AVCodecContext *avctx)
{
    const HEVCContext *h = avctx->priv_data;
    RKVDECPictureHEVC *pic = h->ref->hwaccel_picture_private;

    av_log(avctx, AV_LOG_DEBUG, "%s: avctx=%p pic=%p slice_count=%u bitstream_size=%u\n", __func__, avctx, pic, pic->slice_count, pic->bitstream_size);

    if (pic->slice_count <= 0 || pic->bitstream_size <= 0)
        return -1;

    return 0;
}

AVHWAccel ff_hevc_rkvdec_hwaccel = {
    .name           = "hevc_rkvdec",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_HEVC,
    .pix_fmt        = AV_PIX_FMT_DRM_PRIME,
    .init           = &ff_rkvdec_decode_init,
    .uninit         = &ff_rkvdec_decode_uninit,
    .start_frame    = &rkvdec_hevc_start_frame,
    .decode_slice   = &rkvdec_hevc_decode_slice,
    .end_frame      = &rkvdec_hevc_end_frame,
    .frame_priv_data_size = sizeof(RKVDECPictureHEVC),
    .priv_data_size = sizeof(RKVDECContext),
};
