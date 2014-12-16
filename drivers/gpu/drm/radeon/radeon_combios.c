/*
 * Copyright 2004 ATI Technologies Inc., Markham, Ontario
 * Copyright 2007-8 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 */
#include "drmP.h"
#include "radeon_drm.h"
#include "radeon.h"
#include "atom.h"

#ifdef CONFIG_PPC_PMAC
/* not sure which of these are needed */
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/prom.h>
#include <asm/pci-bridge.h>
#endif /* CONFIG_PPC_PMAC */

/* from radeon_encoder.c */
extern uint32_t
radeon_get_encoder_id(struct drm_device *dev, uint32_t supported_device,
		      uint8_t dac);
extern void radeon_link_encoder_connector(struct drm_device *dev);

/* from radeon_connector.c */
extern void
radeon_add_legacy_connector(struct drm_device *dev,
			    uint32_t connector_id,
			    uint32_t supported_device,
			    int connector_type,
			    struct radeon_i2c_bus_rec *i2c_bus,
			    uint16_t connector_object_id,
			    struct radeon_hpd *hpd);

/* from radeon_legacy_encoder.c */
extern void
radeon_add_legacy_encoder(struct drm_device *dev, uint32_t encoder_id,
			  uint32_t supported_device);

/* old legacy ATI BIOS routines */

/* COMBIOS table offsets */
enum radeon_combios_table_offset {
	/* absolute offset tables */
	COMBIOS_ASIC_INIT_1_TABLE,
	COMBIOS_BIOS_SUPPORT_TABLE,
	COMBIOS_DAC_PROGRAMMING_TABLE,
	COMBIOS_MAX_COLOR_DEPTH_TABLE,
	COMBIOS_CRTC_INFO_TABLE,
	COMBIOS_PLL_INFO_TABLE,
	COMBIOS_TV_INFO_TABLE,
	COMBIOS_DFP_INFO_TABLE,
	COMBIOS_HW_CONFIG_INFO_TABLE,
	COMBIOS_MULTIMEDIA_INFO_TABLE,
	COMBIOS_TV_STD_PATCH_TABLE,
	COMBIOS_LCD_INFO_TABLE,
	COMBIOS_MOBILE_INFO_TABLE,
	COMBIOS_PLL_INIT_TABLE,
	COMBIOS_MEM_CONFIG_TABLE,
	COMBIOS_SAVE_MASK_TABLE,
	COMBIOS_HARDCODED_EDID_TABLE,
	COMBIOS_ASIC_INIT_2_TABLE,
	COMBIOS_CONNECTOR_INFO_TABLE,
	COMBIOS_DYN_CLK_1_TABLE,
	COMBIOS_RESERVED_MEM_TABLE,
	COMBIOS_EXT_TMDS_INFO_TABLE,
	COMBIOS_MEM_CLK_INFO_TABLE,
	COMBIOS_EXT_DAC_INFO_TABLE,
	COMBIOS_MISC_INFO_TABLE,
	COMBIOS_CRT_INFO_TABLE,
	COMBIOS_INTEGRATED_SYSTEM_INFO_TABLE,
	COMBIOS_COMPONENT_VIDEO_INFO_TABLE,
	COMBIOS_FAN_SPEED_INFO_TABLE,
	COMBIOS_OVERDRIVE_INFO_TABLE,
	COMBIOS_OEM_INFO_TABLE,
	COMBIOS_DYN_CLK_2_TABLE,
	COMBIOS_POWER_CONNECTOR_INFO_TABLE,
	COMBIOS_I2C_INFO_TABLE,
	/* relative offset tables */
	COMBIOS_ASIC_INIT_3_TABLE,	/* offset from misc info */
	COMBIOS_ASIC_INIT_4_TABLE,	/* offset from misc info */
	COMBIOS_DETECTED_MEM_TABLE,	/* offset from misc info */
	COMBIOS_ASIC_INIT_5_TABLE,	/* offset from misc info */
	COMBIOS_RAM_RESET_TABLE,	/* offset from mem config */
	COMBIOS_POWERPLAY_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_GPIO_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_LCD_DDC_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_TMDS_POWER_TABLE,	/* offset from mobile info */
	COMBIOS_TMDS_POWER_ON_TABLE,	/* offset from tmds power */
	COMBIOS_TMDS_POWER_OFF_TABLE,	/* offset from tmds power */
};

enum radeon_combios_ddc {
	DDC_NONE_DETECTED,
	DDC_MONID,
	DDC_DVI,
	DDC_VGA,
	DDC_CRT2,
	DDC_LCD,
	DDC_GPIO,
};

enum radeon_combios_connector {
	CONNECTOR_NONE_LEGACY,
	CONNECTOR_PROPRIETARY_LEGACY,
	CONNECTOR_CRT_LEGACY,
	CONNECTOR_DVI_I_LEGACY,
	CONNECTOR_DVI_D_LEGACY,
	CONNECTOR_CTV_LEGACY,
	CONNECTOR_STV_LEGACY,
	CONNECTOR_UNSUPPORTED_LEGACY
};

const int legacy_connector_convert[] = {
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_VGA,
	DRM_MODE_CONNECTOR_DVII,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_Composite,
	DRM_MODE_CONNECTOR_SVIDEO,
	DRM_MODE_CONNECTOR_Unknown,
};

static uint16_t combios_get_table_offset(struct drm_device *dev,
					 enum radeon_combios_table_offset table)
{
	struct radeon_device *rdev = dev->dev_private;
	int rev;
	uint16_t offset = 0, check_offset;

	switch (table) {
		/* absolute offset tables */
	case COMBIOS_ASIC_INIT_1_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0xc);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_BIOS_SUPPORT_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x14);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DAC_PROGRAMMING_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MAX_COLOR_DEPTH_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CRTC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_PLL_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x30);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_TV_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x32);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DFP_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x34);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_HW_CONFIG_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x36);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MULTIMEDIA_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x38);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_TV_STD_PATCH_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x3e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_LCD_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x40);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MOBILE_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x42);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_PLL_INIT_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x46);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MEM_CONFIG_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x48);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_SAVE_MASK_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_HARDCODED_EDID_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_ASIC_INIT_2_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CONNECTOR_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x50);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DYN_CLK_1_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x52);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_RESERVED_MEM_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x54);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_EXT_TMDS_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x58);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MEM_CLK_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_EXT_DAC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MISC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CRT_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x60);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_INTEGRATED_SYSTEM_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x62);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_COMPONENT_VIDEO_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x64);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_FAN_SPEED_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x66);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_OVERDRIVE_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x68);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_OEM_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DYN_CLK_2_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_POWER_CONNECTOR_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_I2C_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x70);
		if (check_offset)
			offset = check_offset;
		break;
		/* relative offset tables */
	case COMBIOS_ASIC_INIT_3_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x3);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_ASIC_INIT_4_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x5);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_DETECTED_MEM_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x7);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_ASIC_INIT_5_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev == 2) {
				check_offset = RBIOS16(check_offset + 0x9);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_RAM_RESET_TABLE:	/* offset from mem config */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MEM_CONFIG_TABLE);
		if (check_offset) {
			while (RBIOS8(check_offset++));
			check_offset += 2;
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_POWERPLAY_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x11);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_GPIO_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x13);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_LCD_DDC_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x15);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x17);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_ON_TABLE:	/* offset from tmds power */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_TMDS_POWER_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x2);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_OFF_TABLE:	/* offset from tmds power */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_TMDS_POWER_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x4);
			if (check_offset)
				offset = check_offset;
		}
		break;
	default:
		break;
	}

	return offset;

}

static struct radeon_i2c_bus_rec combios_setup_i2c_bus(struct radeon_device *rdev,
						       int ddc_line)
{
	struct radeon_i2c_bus_rec i2c;

	if (ddc_line == RADEON_GPIOPAD_MASK) {
		i2c.mask_clk_reg = RADEON_GPIOPAD_MASK;
		i2c.mask_data_reg = RADEON_GPIOPAD_MASK;
		i2c.a_clk_reg = RADEON_GPIOPAD_A;
		i2c.a_data_reg = RADEON_GPIOPAD_A;
		i2c.en_clk_reg = RADEON_GPIOPAD_EN;
		i2c.en_data_reg = RADEON_GPIOPAD_EN;
		i2c.y_clk_reg = RADEON_GPIOPAD_Y;
		i2c.y_data_reg = RADEON_GPIOPAD_Y;
	} else if (ddc_line == RADEON_MDGPIO_MASK) {
		i2c.mask_clk_reg = RADEON_MDGPIO_MASK;
		i2c.mask_data_reg = RADEON_MDGPIO_MASK;
		i2c.a_clk_reg = RADEON_MDGPIO_A;
		i2c.a_data_reg = RADEON_MDGPIO_A;
		i2c.en_clk_reg = RADEON_MDGPIO_EN;
		i2c.en_data_reg = RADEON_MDGPIO_EN;
		i2c.y_clk_reg = RADEON_MDGPIO_Y;
		i2c.y_data_reg = RADEON_MDGPIO_Y;
	} else {
		i2c.mask_clk_mask = RADEON_GPIO_EN_1;
		i2c.mask_data_mask = RADEON_GPIO_EN_0;
		i2c.a_clk_mask = RADEON_GPIO_A_1;
		i2c.a_data_mask = RADEON_GPIO_A_0;
		i2c.en_clk_mask = RADEON_GPIO_EN_1;
		i2c.en_data_mask = RADEON_GPIO_EN_0;
		i2c.y_clk_mask = RADEON_GPIO_Y_1;
		i2c.y_data_mask = RADEON_GPIO_Y_0;

		i2c.mask_clk_reg = ddc_line;
		i2c.mask_data_reg = ddc_line;
		i2c.a_clk_reg = ddc_line;
		i2c.a_data_reg = ddc_line;
		i2c.en_clk_reg = ddc_line;
		i2c.en_data_reg = ddc_line;
		i2c.y_clk_reg = ddc_line;
		i2c.y_data_reg = ddc_line;
	}

	if (rdev->family < CHIP_R200)
		i2c.hw_capable = false;
	else {
		switch (ddc_line) {
		case RADEON_GPIO_VGA_DDC:
		case RADEON_GPIO_DVI_DDC:
			i2c.hw_capable = true;
			break;
		case RADEON_GPIO_MONID:
			/* hw i2c on RADEON_GPIO_MONID doesn't seem to work
			 * reliably on some pre-r4xx hardware; not sure why.
			 */
			i2c.hw_capable = false;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
	}
	i2c.mm_i2c = false;
	i2c.i2c_id = 0;

	if (ddc_line)
		i2c.valid = true;
	else
		i2c.valid = false;

	return i2c;
}

bool radeon_combios_get_clock_info(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint16_t pll_info;
	struct radeon_pll *p1pll = &rdev->clock.p1pll;
	struct radeon_pll *p2pll = &rdev->clock.p2pll;
	struct radeon_pll *spll = &rdev->clock.spll;
	struct radeon_pll *mpll = &rdev->clock.mpll;
	int8_t rev;
	uint16_t sclk, mclk;

	if (rdev->bios == NULL)
		return false;

	pll_info = combios_get_table_offset(dev, COMBIOS_PLL_INFO_TABLE);
	if (pll_info) {
		rev = RBIOS8(pll_info);

		/* pixel clocks */
		p1pll->reference_freq = RBIOS16(pll_info + 0xe);
		p1pll->reference_div = RBIOS16(pll_info + 0x10);
		p1pll->pll_out_min = RBIOS32(pll_info + 0x12);
		p1pll->pll_out_max = RBIOS32(pll_info + 0x16);

		if (rev > 9) {
			p1pll->pll_in_min = RBIOS32(pll_info + 0x36);
			p1pll->pll_in_max = RBIOS32(pll_info + 0x3a);
		} else {
			p1pll->pll_in_min = 40;
			p1pll->pll_in_max = 500;
		}
		*p2pll = *p1pll;

		/* system clock */
		spll->reference_freq = RBIOS16(pll_info + 0x1a);
		spll->reference_div = RBIOS16(pll_info + 0x1c);
		spll->pll_out_min = RBIOS32(pll_info + 0x1e);
		spll->pll_out_max = RBIOS32(pll_info + 0x22);

		if (rev > 10) {
			spll->pll_in_min = RBIOS32(pll_info + 0x48);
			spll->pll_in_max = RBIOS32(pll_info + 0x4c);
		} else {
			/* ??? */
			spll->pll_in_min = 40;
			spll->pll_in_max = 500;
		}

		/* memory clock */
		mpll->reference_freq = RBIOS16(pll_info + 0x26);
		mpll->reference_div = RBIOS16(pll_info + 0x28);
		mpll->pll_out_min = RBIOS32(pll_info + 0x2a);
		mpll->pll_out_max = RBIOS32(pll_info + 0x2e);

		if (rev > 10) {
			mpll->pll_in_min = RBIOS32(pll_info + 0x5a);
			mpll->pll_in_max = RBIOS32(pll_info + 0x5e);
		} else {
			/* ??? */
			mpll->pll_in_min = 40;
			mpll->pll_in_max = 500;
		}

		/* default sclk/mclk */
		sclk = RBIOS16(pll_info + 0xa);
		mclk = RBIOS16(pll_info + 0x8);
		if (sclk == 0)
			sclk = 200 * 100;
		if (mclk == 0)
			mclk = 200 * 100;

		rdev->clock.default_sclk = sclk;
		rdev->clock.default_mclk = mclk;

		return true;
	}
	return false;
}

struct radeon_encoder_primary_dac *radeon_combios_get_primary_dac_info(struct
								       radeon_encoder
								       *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t dac_info;
	uint8_t rev, bg, dac;
	struct radeon_encoder_primary_dac *p_dac = NULL;

	if (rdev->bios == NULL)
		return NULL;

	/* check CRT table */
	dac_info = combios_get_table_offset(dev, COMBIOS_CRT_INFO_TABLE);
	if (dac_info) {
		p_dac =
		    kzalloc(sizeof(struct radeon_encoder_primary_dac),
			    GFP_KERNEL);

		if (!p_dac)
			return NULL;

		rev = RBIOS8(dac_info) & 0x3;
		if (rev < 2) {
			bg = RBIOS8(dac_info + 0x2) & 0xf;
			dac = (RBIOS8(dac_info + 0x2) >> 4) & 0xf;
			p_dac->ps2_pdac_adj = (bg << 8) | (dac);
		} else {
			bg = RBIOS8(dac_info + 0x2) & 0xf;
			dac = RBIOS8(dac_info + 0x3) & 0xf;
			p_dac->ps2_pdac_adj = (bg << 8) | (dac);
		}

	}

	return p_dac;
}

static enum radeon_tv_std
radeon_combios_get_tv_info(struct radeon_encoder *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t tv_info;
	enum radeon_tv_std tv_std = TV_STD_NTSC;

	tv_info = combios_get_table_offset(dev, COMBIOS_TV_INFO_TABLE);
	if (tv_info) {
		if (RBIOS8(tv_info + 6) == 'T') {
			switch (RBIOS8(tv_info + 7) & 0xf) {
			case 1:
				tv_std = TV_STD_NTSC;
				DRM_INFO("Default TV standard: NTSC\n");
				break;
			case 2:
				tv_std = TV_STD_PAL;
				DRM_INFO("Default TV standard: PAL\n");
				break;
			case 3:
				tv_std = TV_STD_PAL_M;
				DRM_INFO("Default TV standard: PAL-M\n");
				break;
			case 4:
				tv_std = TV_STD_PAL_60;
				DRM_INFO("Default TV standard: PAL-60\n");
				break;
			case 5:
				tv_std = TV_STD_NTSC_J;
				DRM_INFO("Default TV standard: NTSC-J\n");
				break;
			case 6:
				tv_std = TV_STD_SCART_PAL;
				DRM_INFO("Default TV standard: SCART-PAL\n");
				break;
			default:
				tv_std = TV_STD_NTSC;
				DRM_INFO
				    ("Unknown TV standard; defaulting to NTSC\n");
				break;
			}

			switch ((RBIOS8(tv_info + 9) >> 2) & 0x3) {
			case 0:
				DRM_INFO("29.498928713 MHz TV ref clk\n");
				break;
			case 1:
				DRM_INFO("28.636360000 MHz TV ref clk\n");
				break;
			case 2:
				DRM_INFO("14.318180000 MHz TV ref clk\n");
				break;
			case 3:
				DRM_INFO("27.000000000 MHz TV ref clk\n");
				break;
			default:
				break;
			}
		}
	}
	return tv_std;
}

static const uint32_t default_tvdac_adj[CHIP_LAST] = {
	0x00000000,		/* r100  */
	0x00280000,		/* rv100 */
	0x00000000,		/* rs100 */
	0x00880000,		/* rv200 */
	0x00000000,		/* rs200 */
	0x00000000,		/* r200  */
	0x00770000,		/* rv250 */
	0x00290000,		/* rs300 */
	0x00560000,		/* rv280 */
	0x00780000,		/* r300  */
	0x00770000,		/* r350  */
	0x00780000,		/* rv350 */
	0x00780000,		/* rv380 */
	0x01080000,		/* r420  */
	0x01080000,		/* r423  */
	0x01080000,		/* rv410 */
	0x00780000,		/* rs400 */
	0x00780000,		/* rs480 */
};

static void radeon_legacy_get_tv_dac_info_from_table(struct radeon_device *rdev,
						     struct radeon_encoder_tv_dac *tv_dac)
{
	tv_dac->ps2_tvdac_adj = default_tvdac_adj[rdev->family];
	if ((rdev->flags & RADEON_IS_MOBILITY) && (rdev->family == CHIP_RV250))
		tv_dac->ps2_tvdac_adj = 0x00880000;
	tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
	tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
	return;
}

struct radeon_encoder_tv_dac *radeon_combios_get_tv_dac_info(struct
							     radeon_encoder
							     *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t dac_info;
	uint8_t rev, bg, dac;
	struct radeon_encoder_tv_dac *tv_dac = NULL;
	int found = 0;

	tv_dac = kzalloc(sizeof(struct radeon_encoder_tv_dac), GFP_KERNEL);
	if (!tv_dac)
		return NULL;

	if (rdev->bios == NULL)
		goto out;

	/* first check TV table */
	dac_info = combios_get_table_offset(dev, COMBIOS_TV_INFO_TABLE);
	if (dac_info) {
		rev = RBIOS8(dac_info + 0x3);
		if (rev > 4) {
			bg = RBIOS8(dac_info + 0xc) & 0xf;
			dac = RBIOS8(dac_info + 0xd) & 0xf;
			tv_dac->ps2_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xe) & 0xf;
			dac = RBIOS8(dac_info + 0xf) & 0xf;
			tv_dac->pal_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0x10) & 0xf;
			dac = RBIOS8(dac_info + 0x11) & 0xf;
			tv_dac->ntsc_tvdac_adj = (bg << 16) | (dac << 20);
			found = 1;
		} else if (rev > 1) {
			bg = RBIOS8(dac_info + 0xc) & 0xf;
			dac = (RBIOS8(dac_info + 0xc) >> 4) & 0xf;
			tv_dac->ps2_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xd) & 0xf;
			dac = (RBIOS8(dac_info + 0xd) >> 4) & 0xf;
			tv_dac->pal_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xe) & 0xf;
			dac = (RBIOS8(dac_info + 0xe) >> 4) & 0xf;
			tv_dac->ntsc_tvdac_adj = (bg << 16) | (dac << 20);
			found = 1;
		}
		tv_dac->tv_std = radeon_combios_get_tv_info(encoder);
	}
	if (!found) {
		/* then check CRT table */
		dac_info =
		    combios_get_table_offset(dev, COMBIOS_CRT_INFO_TABLE);
		if (dac_info) {
			rev = RBIOS8(dac_info) & 0x3;
			if (rev < 2) {
				bg = RBIOS8(dac_info + 0x3) & 0xf;
				dac = (RBIOS8(dac_info + 0x3) >> 4) & 0xf;
				tv_dac->ps2_tvdac_adj =
				    (bg << 16) | (dac << 20);
				tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
				tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
				found = 1;
			} else {
				bg = RBIOS8(dac_info + 0x4) & 0xf;
				dac = RBIOS8(dac_info + 0x5) & 0xf;
				tv_dac->ps2_tvdac_adj =
				    (bg << 16) | (dac << 20);
				tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
				tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
				found = 1;
			}
		} else {
			DRM_INFO("No TV DAC info found in BIOS\n");
		}
	}

out:
	if (!found) /* fallback to defaults */
		radeon_legacy_get_tv_dac_info_from_table(rdev, tv_dac);

	return tv_dac;
}

static struct radeon_encoder_lvds *radeon_legacy_get_lvds_info_from_regs(struct
									 radeon_device
									 *rdev)
{
	struct radeon_encoder_lvds *lvds = NULL;
	uint32_t fp_vert_stretch, fp_horz_stretch;
	uint32_t ppll_div_sel, ppll_val;
	uint32_t lvds_ss_gen_cntl = RREG32(RADEON_LVDS_SS_GEN_CNTL);

	lvds = kzalloc(sizeof(struct radeon_encoder_lvds), GFP_KERNEL);

	if (!lvds)
		return NULL;

	fp_vert_stretch = RREG32(RADEON_FP_VERT_STRETCH);
	fp_horz_stretch = RREG32(RADEON_FP_HORZ_STRETCH);

	/* These should be fail-safe defaults, fingers crossed */
	lvds->panel_pwr_delay = 200;
	lvds->panel_vcc_delay = 2000;

	lvds->lvds_gen_cntl = RREG32(RADEON_LVDS_GEN_CNTL);
	lvds->panel_digon_delay = (lvds_ss_gen_cntl >> RADEON_LVDS_PWRSEQ_DELAY1_SHIFT) & 0xf;
	lvds->panel_blon_delay = (lvds_ss_gen_cntl >> RADEON_LVDS_PWRSEQ_DELAY2_SHIFT) & 0xf;

	if (fp_vert_stretch & RADEON_VERT_STRETCH_ENABLE)
		lvds->native_mode.vdisplay =
		    ((fp_vert_stretch & RADEON_VERT_PANEL_SIZE) >>
		     RADEON_VERT_PANEL_SHIFT) + 1;
	else
		lvds->native_mode.vdisplay =
		    (RREG32(RADEON_CRTC_V_TOTAL_DISP) >> 16) + 1;

	if (fp_horz_stretch & RADEON_HORZ_STRETCH_ENABLE)
		lvds->native_mode.hdisplay =
		    (((fp_horz_stretch & RADEON_HORZ_PANEL_SIZE) >>
		      RADEON_HORZ_PANEL_SHIFT) + 1) * 8;
	else
		lvds->native_mode.hdisplay =
		    ((RREG32(RADEON_CRTC_H_TOTAL_DISP) >> 16) + 1) * 8;

	if ((lvds->native_mode.hdisplay < 640) ||
	    (lvds->native_mode.vdisplay < 480)) {
		lvds->native_mode.hdisplay = 640;
		lvds->native_mode.vdisplay = 480;
	}

	ppll_div_sel = RREG8(RADEON_CLOCK_CNTL_INDEX + 1) & 0x3;
	ppll_val = RREG32_PLL(RADEON_PPLL_DIV_0 + ppll_div_sel);
	if ((ppll_val & 0x000707ff) == 0x1bb)
		lvds->use_bios_dividers = false;
	else {
		lvds->panel_ref_divider =
		    RREG32_PLL(RADEON_PPLL_REF_DIV) & 0x3ff;
		lvds->panel_post_divider = (ppll_val >> 16) & 0x7;
		lvds->panel_fb_divider = ppll_val & 0x7ff;

		if ((lvds->panel_ref_divider != 0) &&
		    (lvds->panel_fb_divider > 3))
			lvds->use_bios_dividers = true;
	}
	lvds->panel_vcc_delay = 200;

	DRM_INFO("Panel info derived from registers\n");
	DRM_INFO("Panel Size %dx%d\n", lvds->native_mode.hdisplay,
		 lvds->native_mode.vdisplay);

	return lvds;
}

struct radeon_encoder_lvds *radeon_combios_get_lvds_info(struct radeon_encoder
							 *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t lcd_info;
	uint32_t panel_setup;
	char stmp[30];
	int tmp, i;
	struct radeon_encoder_lvds *lvds = NULL;

	if (rdev->bios == NULL) {
		lvds = radeon_legacy_get_lvds_info_from_regs(rdev);
		goto out;
	}

	lcd_info = combios_get_table_offset(dev, COMBIOS_LCD_INFO_TABLE);

	if (lcd_info) {
		lvds = kzalloc(sizeof(struct radeon_encoder_lvds), GFP_KERNEL);

		if (!lvds)
			return NULL;

		for (i = 0; i < 24; i++)
			stmp[i] = RBIOS8(lcd_info + i + 1);
		stmp[24] = 0;

		DRM_INFO("Panel ID String: %s\n", stmp);

		lvds->native_mode.hdisplay = RBIOS16(lcd_info + 0x19);
		lvds->native_mode.vdisplay = RBIOS16(lcd_info + 0x1b);

		DRM_INFO("Panel Size %dx%d\n", lvds->native_mode.hdisplay,
			 lvds->native_mode.vdisplay);

		lvds->panel_vcc_delay = RBIOS16(lcd_info + 0x2c);
		if (lvds->panel_vcc_delay > 2000 || lvds->panel_vcc_delay < 0)
			lvds->panel_vcc_delay = 2000;

		lvds->panel_pwr_delay = RBIOS8(lcd_info + 0x24);
		lvds->panel_digon_delay = RBIOS16(lcd_info + 0x38) & 0xf;
		lvds->panel_blon_delay = (RBIOS16(lcd_info + 0x38) >> 4) & 0xf;

		lvds->panel_ref_divider = RBIOS16(lcd_info + 0x2e);
		lvds->panel_post_divider = RBIOS8(lcd_info + 0x30);
		lvds->panel_fb_divider = RBIOS16(lcd_info + 0x31);
		if ((lvds->panel_ref_divider != 0) &&
		    (lvds->panel_fb_divider > 3))
			lvds->use_bios_dividers = true;

		panel_setup = RBIOS32(lcd_info + 0x39);
		lvds->lvds_gen_cntl = 0xff00;
		if (panel_setup & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_PANEL_FORMAT;

		if ((panel_setup >> 4) & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_PANEL_TYPE;

		switch ((panel_setup >> 8) & 0x7) {
		case 0:
			lvds->lvds_gen_cntl |= RADEON_LVDS_NO_FM;
			break;
		case 1:
			lvds->lvds_gen_cntl |= RADEON_LVDS_2_GREY;
			break;
		case 2:
			lvds->lvds_gen_cntl |= RADEON_LVDS_4_GREY;
			break;
		default:
			break;
		}

		if ((panel_setup >> 16) & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_FP_POL_LOW;

		if ((panel_setup >> 17) & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_LP_POL_LOW;

		if ((panel_setup >> 18) & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_DTM_POL_LOW;

		if ((panel_setup >> 23) & 0x1)
			lvds->lvds_gen_cntl |= RADEON_LVDS_BL_CLK_SEL;

		lvds->lvds_gen_cntl |= (panel_setup & 0xf0000000);

		for (i = 0; i < 32; i++) {
			tmp = RBIOS16(lcd_info + 64 + i * 2);
			if (tmp == 0)
				break;

			if ((RBIOS16(tmp) == lvds->native_mode.hdisplay) &&
			    (RBIOS16(tmp + 2) ==
			     lvds->native_mode.vdisplay)) {
				lvds->native_mode.htotal = RBIOS16(tmp + 17) * 8;
				lvds->native_mode.hsync_start = RBIOS16(tmp + 21) * 8;
				lvds->native_mode.hsync_end = (RBIOS8(tmp + 23) +
							       RBIOS16(tmp + 21)) * 8;

				lvds->native_mode.vtotal = RBIOS16(tmp + 24);
				lvds->native_mode.vsync_start = RBIOS16(tmp + 28) & 0x7ff;
				lvds->native_mode.vsync_end =
					((RBIOS16(tmp + 28) & 0xf800) >> 11) +
					(RBIOS16(tmp + 28) & 0x7ff);

				lvds->native_mode.clock = RBIOS16(tmp + 9) * 10;
				lvds->native_mode.flags = 0;
				/* set crtc values */
				drm_mode_set_crtcinfo(&lvds->native_mode, CRTC_INTERLACE_HALVE_V);

			}
		}
	} else {
		DRM_INFO("No panel info found in BIOS\n");
		lvds = radeon_legacy_get_lvds_info_from_regs(rdev);
	}
out:
	if (lvds)
		encoder->native_mode = lvds->native_mode;
	return lvds;
}

static const struct radeon_tmds_pll default_tmds_pll[CHIP_LAST][4] = {
	{{12000, 0xa1b}, {0xffffffff, 0xa3f}, {0, 0}, {0, 0}},	/* CHIP_R100  */
	{{12000, 0xa1b}, {0xffffffff, 0xa3f}, {0, 0}, {0, 0}},	/* CHIP_RV100 */
	{{0, 0}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_RS100 */
	{{15000, 0xa1b}, {0xffffffff, 0xa3f}, {0, 0}, {0, 0}},	/* CHIP_RV200 */
	{{12000, 0xa1b}, {0xffffffff, 0xa3f}, {0, 0}, {0, 0}},	/* CHIP_RS200 */
	{{15000, 0xa1b}, {0xffffffff, 0xa3f}, {0, 0}, {0, 0}},	/* CHIP_R200  */
	{{15500, 0x81b}, {0xffffffff, 0x83f}, {0, 0}, {0, 0}},	/* CHIP_RV250 */
	{{0, 0}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_RS300 */
	{{13000, 0x400f4}, {15000, 0x400f7}, {0xffffffff, 0x40111}, {0, 0}},	/* CHIP_RV280 */
	{{0xffffffff, 0xb01cb}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_R300  */
	{{0xffffffff, 0xb01cb}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_R350  */
	{{15000, 0xb0155}, {0xffffffff, 0xb01cb}, {0, 0}, {0, 0}},	/* CHIP_RV350 */
	{{15000, 0xb0155}, {0xffffffff, 0xb01cb}, {0, 0}, {0, 0}},	/* CHIP_RV380 */
	{{0xffffffff, 0xb01cb}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_R420  */
	{{0xffffffff, 0xb01cb}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_R423  */
	{{0xffffffff, 0xb01cb}, {0, 0}, {0, 0}, {0, 0}},	/* CHIP_RV410 */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0} },	/* CHIP_RS400 */
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0} },	/* CHIP_RS480 */
};

bool radeon_legacy_get_tmds_info_from_table(struct radeon_encoder *encoder,
					    struct radeon_encoder_int_tmds *tmds)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	int i;

	for (i = 0; i < 4; i++) {
		tmds->tmds_pll[i].value =
			default_tmds_pll[rdev->family][i].value;
		tmds->tmds_pll[i].freq = default_tmds_pll[rdev->family][i].freq;
	}

	return true;
}

bool radeon_legacy_get_tmds_info_from_combios(struct radeon_encoder *encoder,
					      struct radeon_encoder_int_tmds *tmds)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t tmds_info;
	int i, n;
	uint8_t ver;

	if (rdev->bios == NULL)
		return false;

	tmds_info = combios_get_table_offset(dev, COMBIOS_DFP_INFO_TABLE);

	if (tmds_info) {
		ver = RBIOS8(tmds_info);
		DRM_INFO("DFP table revision: %d\n", ver);
		if (ver == 3) {
			n = RBIOS8(tmds_info + 5) + 1;
			if (n > 4)
				n = 4;
			for (i = 0; i < n; i++) {
				tmds->tmds_pll[i].value =
				    RBIOS32(tmds_info + i * 10 + 0x08);
				tmds->tmds_pll[i].freq =
				    RBIOS16(tmds_info + i * 10 + 0x10);
				DRM_DEBUG("TMDS PLL From COMBIOS %u %x\n",
					  tmds->tmds_pll[i].freq,
					  tmds->tmds_pll[i].value);
			}
		} else if (ver == 4) {
			int stride = 0;
			n = RBIOS8(tmds_info + 5) + 1;
			if (n > 4)
				n = 4;
			for (i = 0; i < n; i++) {
				tmds->tmds_pll[i].value =
				    RBIOS32(tmds_info + stride + 0x08);
				tmds->tmds_pll[i].freq =
				    RBIOS16(tmds_info + stride + 0x10);
				if (i == 0)
					stride += 10;
				else
					stride += 6;
				DRM_DEBUG("TMDS PLL From COMBIOS %u %x\n",
					  tmds->tmds_pll[i].freq,
					  tmds->tmds_pll[i].value);
			}
		}
	} else {
		DRM_INFO("No TMDS info found in BIOS\n");
		return false;
	}
	return true;
}

bool radeon_legacy_get_ext_tmds_info_from_table(struct radeon_encoder *encoder,
						struct radeon_encoder_ext_tmds *tmds)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_i2c_bus_rec i2c_bus;

	/* default for macs */
	i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
	tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");

	/* XXX some macs have duallink chips */
	switch (rdev->mode_info.connector_table) {
	case CT_POWERBOOK_EXTERNAL:
	case CT_MINI_EXTERNAL:
	default:
		tmds->dvo_chip = DVO_SIL164;
		tmds->slave_addr = 0x70 >> 1; /* 7 bit addressing */
		break;
	}

	return true;
}

bool radeon_legacy_get_ext_tmds_info_from_combios(struct radeon_encoder *encoder,
						  struct radeon_encoder_ext_tmds *tmds)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t offset;
	uint8_t ver, id, blocks, clk, data;
	int i;
	enum radeon_combios_ddc gpio;
	struct radeon_i2c_bus_rec i2c_bus;

	if (rdev->bios == NULL)
		return false;

	tmds->i2c_bus = NULL;
	if (rdev->flags & RADEON_IS_IGP) {
		offset = combios_get_table_offset(dev, COMBIOS_I2C_INFO_TABLE);
		if (offset) {
			ver = RBIOS8(offset);
			DRM_INFO("GPIO Table revision: %d\n", ver);
			blocks = RBIOS8(offset + 2);
			for (i = 0; i < blocks; i++) {
				id = RBIOS8(offset + 3 + (i * 5) + 0);
				if (id == 136) {
					clk = RBIOS8(offset + 3 + (i * 5) + 3);
					data = RBIOS8(offset + 3 + (i * 5) + 4);
					i2c_bus.valid = true;
					i2c_bus.mask_clk_mask = (1 << clk);
					i2c_bus.mask_data_mask = (1 << data);
					i2c_bus.a_clk_mask = (1 << clk);
					i2c_bus.a_data_mask = (1 << data);
					i2c_bus.en_clk_mask = (1 << clk);
					i2c_bus.en_data_mask = (1 << data);
					i2c_bus.y_clk_mask = (1 << clk);
					i2c_bus.y_data_mask = (1 << data);
					i2c_bus.mask_clk_reg = RADEON_GPIOPAD_MASK;
					i2c_bus.mask_data_reg = RADEON_GPIOPAD_MASK;
					i2c_bus.a_clk_reg = RADEON_GPIOPAD_A;
					i2c_bus.a_data_reg = RADEON_GPIOPAD_A;
					i2c_bus.en_clk_reg = RADEON_GPIOPAD_EN;
					i2c_bus.en_data_reg = RADEON_GPIOPAD_EN;
					i2c_bus.y_clk_reg = RADEON_GPIOPAD_Y;
					i2c_bus.y_data_reg = RADEON_GPIOPAD_Y;
					tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");
					tmds->dvo_chip = DVO_SIL164;
					tmds->slave_addr = 0x70 >> 1; /* 7 bit addressing */
					break;
				}
			}
		}
	} else {
		offset = combios_get_table_offset(dev, COMBIOS_EXT_TMDS_INFO_TABLE);
		if (offset) {
			ver = RBIOS8(offset);
			DRM_INFO("External TMDS Table revision: %d\n", ver);
			tmds->slave_addr = RBIOS8(offset + 4 + 2);
			tmds->slave_addr >>= 1; /* 7 bit addressing */
			gpio = RBIOS8(offset + 4 + 3);
			switch (gpio) {
			case DDC_MONID:
				i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
				tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");
				break;
			case DDC_DVI:
				i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
				tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");
				break;
			case DDC_VGA:
				i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
				tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");
				break;
			case DDC_CRT2:
				/* R3xx+ chips don't have GPIO_CRT2_DDC gpio pad */
				if (rdev->family >= CHIP_R300)
					i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
				else
					i2c_bus = combios_setup_i2c_bus(rdev, RADEON_GPIO_CRT2_DDC);
				tmds->i2c_bus = radeon_i2c_create(dev, &i2c_bus, "DVO");
				break;
			case DDC_LCD: /* MM i2c */
				DRM_ERROR("MM i2c requires hw i2c engine\n");
				break;
			default:
				DRM_ERROR("Unsupported gpio %d\n", gpio);
				break;
			}
		}
	}

	if (!tmds->i2c_bus) {
		DRM_INFO("No valid Ext TMDS info found in BIOS\n");
		return false;
	}

	return true;
}

bool radeon_get_legacy_connector_info_from_table(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_i2c_bus_rec ddc_i2c;
	struct radeon_hpd hpd;

	rdev->mode_info.connector_table = radeon_connector_table;
	if (rdev->mode_info.connector_table == CT_NONE) {
#ifdef CONFIG_PPC_PMAC
		if (machine_is_compatible("PowerBook3,3")) {
			/* powerbook with VGA */
			rdev->mode_info.connector_table = CT_POWERBOOK_VGA;
		} else if (machine_is_compatible("PowerBook3,4") ||
			   machine_is_compatible("PowerBook3,5")) {
			/* powerbook with internal tmds */
			rdev->mode_info.connector_table = CT_POWERBOOK_INTERNAL;
		} else if (machine_is_compatible("PowerBook5,1") ||
			   machine_is_compatible("PowerBook5,2") ||
			   machine_is_compatible("PowerBook5,3") ||
			   machine_is_compatible("PowerBook5,4") ||
			   machine_is_compatible("PowerBook5,5")) {
			/* powerbook with external single link tmds (sil164) */
			rdev->mode_info.connector_table = CT_POWERBOOK_EXTERNAL;
		} else if (machine_is_compatible("PowerBook5,6")) {
			/* powerbook with external dual or single link tmds */
			rdev->mode_info.connector_table = CT_POWERBOOK_EXTERNAL;
		} else if (machine_is_compatible("PowerBook5,7") ||
			   machine_is_compatible("PowerBook5,8") ||
			   machine_is_compatible("PowerBook5,9")) {
			/* PowerBook6,2 ? */
			/* powerbook with external dual link tmds (sil1178?) */
			rdev->mode_info.connector_table = CT_POWERBOOK_EXTERNAL;
		} else if (machine_is_compatible("PowerBook4,1") ||
			   machine_is_compatible("PowerBook4,2") ||
			   machine_is_compatible("PowerBook4,3") ||
			   machine_is_compatible("PowerBook6,3") ||
			   machine_is_compatible("PowerBook6,5") ||
			   machine_is_compatible("PowerBook6,7")) {
			/* ibook */
			rdev->mode_info.connector_table = CT_IBOOK;
		} else if (machine_is_compatible("PowerMac4,4")) {
			/* emac */
			rdev->mode_info.connector_table = CT_EMAC;
		} else if (machine_is_compatible("PowerMac10,1")) {
			/* mini with internal tmds */
			rdev->mode_info.connector_table = CT_MINI_INTERNAL;
		} else if (machine_is_compatible("PowerMac10,2")) {
			/* mini with external tmds */
			rdev->mode_info.connector_table = CT_MINI_EXTERNAL;
		} else if (machine_is_compatible("PowerMac12,1")) {
			/* PowerMac8,1 ? */
			/* imac g5 isight */
			rdev->mode_info.connector_table = CT_IMAC_G5_ISIGHT;
		} else
#endif /* CONFIG_PPC_PMAC */
			rdev->mode_info.connector_table = CT_GENERIC;
	}

	switch (rdev->mode_info.connector_table) {
	case CT_GENERIC:
		DRM_INFO("Connector Table: %d (generic)\n",
			 rdev->mode_info.connector_table);
		/* these are the most common settings */
		if (rdev->flags & RADEON_SINGLE_CRTC) {
			/* VGA - primary dac */
			ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_CRT1_SUPPORT,
									1),
						  ATOM_DEVICE_CRT1_SUPPORT);
			radeon_add_legacy_connector(dev, 0,
						    ATOM_DEVICE_CRT1_SUPPORT,
						    DRM_MODE_CONNECTOR_VGA,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_VGA,
						    &hpd);
		} else if (rdev->flags & RADEON_IS_MOBILITY) {
			/* LVDS */
			ddc_i2c = combios_setup_i2c_bus(rdev, 0);
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_LCD1_SUPPORT,
									0),
						  ATOM_DEVICE_LCD1_SUPPORT);
			radeon_add_legacy_connector(dev, 0,
						    ATOM_DEVICE_LCD1_SUPPORT,
						    DRM_MODE_CONNECTOR_LVDS,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_LVDS,
						    &hpd);

			/* VGA - primary dac */
			ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_CRT1_SUPPORT,
									1),
						  ATOM_DEVICE_CRT1_SUPPORT);
			radeon_add_legacy_connector(dev, 1,
						    ATOM_DEVICE_CRT1_SUPPORT,
						    DRM_MODE_CONNECTOR_VGA,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_VGA,
						    &hpd);
		} else {
			/* DVI-I - tv dac, int tmds */
			ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
			hpd.hpd = RADEON_HPD_1;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_DFP1_SUPPORT,
									0),
						  ATOM_DEVICE_DFP1_SUPPORT);
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_CRT2_SUPPORT,
									2),
						  ATOM_DEVICE_CRT2_SUPPORT);
			radeon_add_legacy_connector(dev, 0,
						    ATOM_DEVICE_DFP1_SUPPORT |
						    ATOM_DEVICE_CRT2_SUPPORT,
						    DRM_MODE_CONNECTOR_DVII,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I,
						    &hpd);

			/* VGA - primary dac */
			ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_CRT1_SUPPORT,
									1),
						  ATOM_DEVICE_CRT1_SUPPORT);
			radeon_add_legacy_connector(dev, 1,
						    ATOM_DEVICE_CRT1_SUPPORT,
						    DRM_MODE_CONNECTOR_VGA,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_VGA,
						    &hpd);
		}

		if (rdev->family != CHIP_R100 && rdev->family != CHIP_R200) {
			/* TV - tv dac */
			ddc_i2c.valid = false;
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_TV1_SUPPORT,
									2),
						  ATOM_DEVICE_TV1_SUPPORT);
			radeon_add_legacy_connector(dev, 2,
						    ATOM_DEVICE_TV1_SUPPORT,
						    DRM_MODE_CONNECTOR_SVIDEO,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_SVIDEO,
						    &hpd);
		}
		break;
	case CT_IBOOK:
		DRM_INFO("Connector Table: %d (ibook)\n",
			 rdev->mode_info.connector_table);
		/* LVDS */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_LCD1_SUPPORT,
								0),
					  ATOM_DEVICE_LCD1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_LCD1_SUPPORT,
					    DRM_MODE_CONNECTOR_LVDS, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_LVDS,
					    &hpd);
		/* VGA - TV DAC */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT2_SUPPORT,
								2),
					  ATOM_DEVICE_CRT2_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_CRT2_SUPPORT,
					    DRM_MODE_CONNECTOR_VGA, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_VGA,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_POWERBOOK_EXTERNAL:
		DRM_INFO("Connector Table: %d (powerbook external tmds)\n",
			 rdev->mode_info.connector_table);
		/* LVDS */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_LCD1_SUPPORT,
								0),
					  ATOM_DEVICE_LCD1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_LCD1_SUPPORT,
					    DRM_MODE_CONNECTOR_LVDS, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_LVDS,
					    &hpd);
		/* DVI-I - primary dac, ext tmds */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
		hpd.hpd = RADEON_HPD_2; /* ??? */
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_DFP2_SUPPORT,
								0),
					  ATOM_DEVICE_DFP2_SUPPORT);
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT1_SUPPORT,
								1),
					  ATOM_DEVICE_CRT1_SUPPORT);
		/* XXX some are SL */
		radeon_add_legacy_connector(dev, 1,
					    ATOM_DEVICE_DFP2_SUPPORT |
					    ATOM_DEVICE_CRT1_SUPPORT,
					    DRM_MODE_CONNECTOR_DVII, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_POWERBOOK_INTERNAL:
		DRM_INFO("Connector Table: %d (powerbook internal tmds)\n",
			 rdev->mode_info.connector_table);
		/* LVDS */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_LCD1_SUPPORT,
								0),
					  ATOM_DEVICE_LCD1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_LCD1_SUPPORT,
					    DRM_MODE_CONNECTOR_LVDS, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_LVDS,
					    &hpd);
		/* DVI-I - primary dac, int tmds */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
		hpd.hpd = RADEON_HPD_1; /* ??? */
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_DFP1_SUPPORT,
								0),
					  ATOM_DEVICE_DFP1_SUPPORT);
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT1_SUPPORT,
								1),
					  ATOM_DEVICE_CRT1_SUPPORT);
		radeon_add_legacy_connector(dev, 1,
					    ATOM_DEVICE_DFP1_SUPPORT |
					    ATOM_DEVICE_CRT1_SUPPORT,
					    DRM_MODE_CONNECTOR_DVII, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_POWERBOOK_VGA:
		DRM_INFO("Connector Table: %d (powerbook vga)\n",
			 rdev->mode_info.connector_table);
		/* LVDS */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_LCD1_SUPPORT,
								0),
					  ATOM_DEVICE_LCD1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_LCD1_SUPPORT,
					    DRM_MODE_CONNECTOR_LVDS, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_LVDS,
					    &hpd);
		/* VGA - primary dac */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT1_SUPPORT,
								1),
					  ATOM_DEVICE_CRT1_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_CRT1_SUPPORT,
					    DRM_MODE_CONNECTOR_VGA, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_VGA,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_MINI_EXTERNAL:
		DRM_INFO("Connector Table: %d (mini external tmds)\n",
			 rdev->mode_info.connector_table);
		/* DVI-I - tv dac, ext tmds */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_CRT2_DDC);
		hpd.hpd = RADEON_HPD_2; /* ??? */
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_DFP2_SUPPORT,
								0),
					  ATOM_DEVICE_DFP2_SUPPORT);
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT2_SUPPORT,
								2),
					  ATOM_DEVICE_CRT2_SUPPORT);
		/* XXX are any DL? */
		radeon_add_legacy_connector(dev, 0,
					    ATOM_DEVICE_DFP2_SUPPORT |
					    ATOM_DEVICE_CRT2_SUPPORT,
					    DRM_MODE_CONNECTOR_DVII, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_MINI_INTERNAL:
		DRM_INFO("Connector Table: %d (mini internal tmds)\n",
			 rdev->mode_info.connector_table);
		/* DVI-I - tv dac, int tmds */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_CRT2_DDC);
		hpd.hpd = RADEON_HPD_1; /* ??? */
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_DFP1_SUPPORT,
								0),
					  ATOM_DEVICE_DFP1_SUPPORT);
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT2_SUPPORT,
								2),
					  ATOM_DEVICE_CRT2_SUPPORT);
		radeon_add_legacy_connector(dev, 0,
					    ATOM_DEVICE_DFP1_SUPPORT |
					    ATOM_DEVICE_CRT2_SUPPORT,
					    DRM_MODE_CONNECTOR_DVII, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_IMAC_G5_ISIGHT:
		DRM_INFO("Connector Table: %d (imac g5 isight)\n",
			 rdev->mode_info.connector_table);
		/* DVI-D - int tmds */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
		hpd.hpd = RADEON_HPD_1; /* ??? */
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_DFP1_SUPPORT,
								0),
					  ATOM_DEVICE_DFP1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_DFP1_SUPPORT,
					    DRM_MODE_CONNECTOR_DVID, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D,
					    &hpd);
		/* VGA - tv dac */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT2_SUPPORT,
								2),
					  ATOM_DEVICE_CRT2_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_CRT2_SUPPORT,
					    DRM_MODE_CONNECTOR_VGA, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_VGA,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	case CT_EMAC:
		DRM_INFO("Connector Table: %d (emac)\n",
			 rdev->mode_info.connector_table);
		/* VGA - primary dac */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT1_SUPPORT,
								1),
					  ATOM_DEVICE_CRT1_SUPPORT);
		radeon_add_legacy_connector(dev, 0, ATOM_DEVICE_CRT1_SUPPORT,
					    DRM_MODE_CONNECTOR_VGA, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_VGA,
					    &hpd);
		/* VGA - tv dac */
		ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_CRT2_DDC);
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_CRT2_SUPPORT,
								2),
					  ATOM_DEVICE_CRT2_SUPPORT);
		radeon_add_legacy_connector(dev, 1, ATOM_DEVICE_CRT2_SUPPORT,
					    DRM_MODE_CONNECTOR_VGA, &ddc_i2c,
					    CONNECTOR_OBJECT_ID_VGA,
					    &hpd);
		/* TV - TV DAC */
		ddc_i2c.valid = false;
		hpd.hpd = RADEON_HPD_NONE;
		radeon_add_legacy_encoder(dev,
					  radeon_get_encoder_id(dev,
								ATOM_DEVICE_TV1_SUPPORT,
								2),
					  ATOM_DEVICE_TV1_SUPPORT);
		radeon_add_legacy_connector(dev, 2, ATOM_DEVICE_TV1_SUPPORT,
					    DRM_MODE_CONNECTOR_SVIDEO,
					    &ddc_i2c,
					    CONNECTOR_OBJECT_ID_SVIDEO,
					    &hpd);
		break;
	default:
		DRM_INFO("Connector table: %d (invalid)\n",
			 rdev->mode_info.connector_table);
		return false;
	}

	radeon_link_encoder_connector(dev);

	return true;
}

static bool radeon_apply_legacy_quirks(struct drm_device *dev,
				       int bios_index,
				       enum radeon_combios_connector
				       *legacy_connector,
				       struct radeon_i2c_bus_rec *ddc_i2c,
				       struct radeon_hpd *hpd)
{
	struct radeon_device *rdev = dev->dev_private;

	/* XPRESS DDC quirks */
	if ((rdev->family == CHIP_RS400 ||
	     rdev->family == CHIP_RS480) &&
	    ddc_i2c->mask_clk_reg == RADEON_GPIO_CRT2_DDC)
		*ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
	else if ((rdev->family == CHIP_RS400 ||
		  rdev->family == CHIP_RS480) &&
		 ddc_i2c->mask_clk_reg == RADEON_GPIO_MONID) {
		*ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIOPAD_MASK);
		ddc_i2c->mask_clk_mask = (0x20 << 8);
		ddc_i2c->mask_data_mask = 0x80;
		ddc_i2c->a_clk_mask = (0x20 << 8);
		ddc_i2c->a_data_mask = 0x80;
		ddc_i2c->en_clk_mask = (0x20 << 8);
		ddc_i2c->en_data_mask = 0x80;
		ddc_i2c->y_clk_mask = (0x20 << 8);
		ddc_i2c->y_data_mask = 0x80;
	}

	/* R3xx+ chips don't have GPIO_CRT2_DDC gpio pad */
	if ((rdev->family >= CHIP_R300) &&
	    ddc_i2c->mask_clk_reg == RADEON_GPIO_CRT2_DDC)
		*ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);

	/* Certain IBM chipset RN50s have a BIOS reporting two VGAs,
	   one with VGA DDC and one with CRT2 DDC. - kill the CRT2 DDC one */
	if (dev->pdev->device == 0x515e &&
	    dev->pdev->subsystem_vendor == 0x1014) {
		if (*legacy_connector == CONNECTOR_CRT_LEGACY &&
		    ddc_i2c->mask_clk_reg == RADEON_GPIO_CRT2_DDC)
			return false;
	}

	/* Some RV100 cards with 2 VGA ports show up with DVI+VGA */
	if (dev->pdev->device == 0x5159 &&
	    dev->pdev->subsystem_vendor == 0x1002 &&
	    dev->pdev->subsystem_device == 0x013a) {
		if (*legacy_connector == CONNECTOR_DVI_I_LEGACY)
			*legacy_connector = CONNECTOR_CRT_LEGACY;

	}

	/* X300 card with extra non-existent DVI port */
	if (dev->pdev->device == 0x5B60 &&
	    dev->pdev->subsystem_vendor == 0x17af &&
	    dev->pdev->subsystem_device == 0x201e && bios_index == 2) {
		if (*legacy_connector == CONNECTOR_DVI_I_LEGACY)
			return false;
	}

	return true;
}

static bool radeon_apply_legacy_tv_quirks(struct drm_device *dev)
{
	/* Acer 5102 has non-existent TV port */
	if (dev->pdev->device == 0x5975 &&
	    dev->pdev->subsystem_vendor == 0x1025 &&
	    dev->pdev->subsystem_device == 0x009f)
		return false;

	/* HP dc5750 has non-existent TV port */
	if (dev->pdev->device == 0x5974 &&
	    dev->pdev->subsystem_vendor == 0x103c &&
	    dev->pdev->subsystem_device == 0x280a)
		return false;

	/* MSI S270 has non-existent TV port */
	if (dev->pdev->device == 0x5955 &&
	    dev->pdev->subsystem_vendor == 0x1462 &&
	    dev->pdev->subsystem_device == 0x0131)
		return false;

	return true;
}

static uint16_t combios_check_dl_dvi(struct drm_device *dev, int is_dvi_d)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t ext_tmds_info;

	if (rdev->flags & RADEON_IS_IGP) {
		if (is_dvi_d)
			return CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D;
		else
			return CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I;
	}
	ext_tmds_info = combios_get_table_offset(dev, COMBIOS_EXT_TMDS_INFO_TABLE);
	if (ext_tmds_info) {
		uint8_t rev = RBIOS8(ext_tmds_info);
		uint8_t flags = RBIOS8(ext_tmds_info + 4 + 5);
		if (rev >= 3) {
			if (is_dvi_d)
				return CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D;
			else
				return CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I;
		} else {
			if (flags & 1) {
				if (is_dvi_d)
					return CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D;
				else
					return CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I;
			}
		}
	}
	if (is_dvi_d)
		return CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D;
	else
		return CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I;
}

bool radeon_get_legacy_connector_info_from_bios(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t conn_info, entry, devices;
	uint16_t tmp, connector_object_id;
	enum radeon_combios_ddc ddc_type;
	enum radeon_combios_connector connector;
	int i = 0;
	struct radeon_i2c_bus_rec ddc_i2c;
	struct radeon_hpd hpd;

	if (rdev->bios == NULL)
		return false;

	conn_info = combios_get_table_offset(dev, COMBIOS_CONNECTOR_INFO_TABLE);
	if (conn_info) {
		for (i = 0; i < 4; i++) {
			entry = conn_info + 2 + i * 2;

			if (!RBIOS16(entry))
				break;

			tmp = RBIOS16(entry);

			connector = (tmp >> 12) & 0xf;

			ddc_type = (tmp >> 8) & 0xf;
			switch (ddc_type) {
			case DDC_MONID:
				ddc_i2c =
					combios_setup_i2c_bus(rdev, RADEON_GPIO_MONID);
				break;
			case DDC_DVI:
				ddc_i2c =
					combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
				break;
			case DDC_VGA:
				ddc_i2c =
					combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
				break;
			case DDC_CRT2:
				ddc_i2c =
					combios_setup_i2c_bus(rdev, RADEON_GPIO_CRT2_DDC);
				break;
			default:
				break;
			}

			switch (connector) {
			case CONNECTOR_PROPRIETARY_LEGACY:
			case CONNECTOR_DVI_I_LEGACY:
			case CONNECTOR_DVI_D_LEGACY:
				if ((tmp >> 4) & 0x1)
					hpd.hpd = RADEON_HPD_2;
				else
					hpd.hpd = RADEON_HPD_1;
				break;
			default:
				hpd.hpd = RADEON_HPD_NONE;
				break;
			}

			if (!radeon_apply_legacy_quirks(dev, i, &connector,
							&ddc_i2c, &hpd))
				continue;

			switch (connector) {
			case CONNECTOR_PROPRIETARY_LEGACY:
				if ((tmp >> 4) & 0x1)
					devices = ATOM_DEVICE_DFP2_SUPPORT;
				else
					devices = ATOM_DEVICE_DFP1_SUPPORT;
				radeon_add_legacy_encoder(dev,
							  radeon_get_encoder_id
							  (dev, devices, 0),
							  devices);
				radeon_add_legacy_connector(dev, i, devices,
							    legacy_connector_convert
							    [connector],
							    &ddc_i2c,
							    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D,
							    &hpd);
				break;
			case CONNECTOR_CRT_LEGACY:
				if (tmp & 0x1) {
					devices = ATOM_DEVICE_CRT2_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_CRT2_SUPPORT,
								   2),
								  ATOM_DEVICE_CRT2_SUPPORT);
				} else {
					devices = ATOM_DEVICE_CRT1_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_CRT1_SUPPORT,
								   1),
								  ATOM_DEVICE_CRT1_SUPPORT);
				}
				radeon_add_legacy_connector(dev,
							    i,
							    devices,
							    legacy_connector_convert
							    [connector],
							    &ddc_i2c,
							    CONNECTOR_OBJECT_ID_VGA,
							    &hpd);
				break;
			case CONNECTOR_DVI_I_LEGACY:
				devices = 0;
				if (tmp & 0x1) {
					devices |= ATOM_DEVICE_CRT2_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_CRT2_SUPPORT,
								   2),
								  ATOM_DEVICE_CRT2_SUPPORT);
				} else {
					devices |= ATOM_DEVICE_CRT1_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_CRT1_SUPPORT,
								   1),
								  ATOM_DEVICE_CRT1_SUPPORT);
				}
				if ((tmp >> 4) & 0x1) {
					devices |= ATOM_DEVICE_DFP2_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_DFP2_SUPPORT,
								   0),
								  ATOM_DEVICE_DFP2_SUPPORT);
					connector_object_id = combios_check_dl_dvi(dev, 0);
				} else {
					devices |= ATOM_DEVICE_DFP1_SUPPORT;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_DFP1_SUPPORT,
								   0),
								  ATOM_DEVICE_DFP1_SUPPORT);
					connector_object_id = CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I;
				}
				radeon_add_legacy_connector(dev,
							    i,
							    devices,
							    legacy_connector_convert
							    [connector],
							    &ddc_i2c,
							    connector_object_id,
							    &hpd);
				break;
			case CONNECTOR_DVI_D_LEGACY:
				if ((tmp >> 4) & 0x1) {
					devices = ATOM_DEVICE_DFP2_SUPPORT;
					connector_object_id = combios_check_dl_dvi(dev, 1);
				} else {
					devices = ATOM_DEVICE_DFP1_SUPPORT;
					connector_object_id = CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I;
				}
				radeon_add_legacy_encoder(dev,
							  radeon_get_encoder_id
							  (dev, devices, 0),
							  devices);
				radeon_add_legacy_connector(dev, i, devices,
							    legacy_connector_convert
							    [connector],
							    &ddc_i2c,
							    connector_object_id,
							    &hpd);
				break;
			case CONNECTOR_CTV_LEGACY:
			case CONNECTOR_STV_LEGACY:
				radeon_add_legacy_encoder(dev,
							  radeon_get_encoder_id
							  (dev,
							   ATOM_DEVICE_TV1_SUPPORT,
							   2),
							  ATOM_DEVICE_TV1_SUPPORT);
				radeon_add_legacy_connector(dev, i,
							    ATOM_DEVICE_TV1_SUPPORT,
							    legacy_connector_convert
							    [connector],
							    &ddc_i2c,
							    CONNECTOR_OBJECT_ID_SVIDEO,
							    &hpd);
				break;
			default:
				DRM_ERROR("Unknown connector type: %d\n",
					  connector);
				continue;
			}

		}
	} else {
		uint16_t tmds_info =
		    combios_get_table_offset(dev, COMBIOS_DFP_INFO_TABLE);
		if (tmds_info) {
			DRM_DEBUG("Found DFP table, assuming DVI connector\n");

			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_CRT1_SUPPORT,
									1),
						  ATOM_DEVICE_CRT1_SUPPORT);
			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_DFP1_SUPPORT,
									0),
						  ATOM_DEVICE_DFP1_SUPPORT);

			ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_DVI_DDC);
			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_connector(dev,
						    0,
						    ATOM_DEVICE_CRT1_SUPPORT |
						    ATOM_DEVICE_DFP1_SUPPORT,
						    DRM_MODE_CONNECTOR_DVII,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I,
						    &hpd);
		} else {
			uint16_t crt_info =
				combios_get_table_offset(dev, COMBIOS_CRT_INFO_TABLE);
			DRM_DEBUG("Found CRT table, assuming VGA connector\n");
			if (crt_info) {
				radeon_add_legacy_encoder(dev,
							  radeon_get_encoder_id(dev,
										ATOM_DEVICE_CRT1_SUPPORT,
										1),
							  ATOM_DEVICE_CRT1_SUPPORT);
				ddc_i2c = combios_setup_i2c_bus(rdev, RADEON_GPIO_VGA_DDC);
				hpd.hpd = RADEON_HPD_NONE;
				radeon_add_legacy_connector(dev,
							    0,
							    ATOM_DEVICE_CRT1_SUPPORT,
							    DRM_MODE_CONNECTOR_VGA,
							    &ddc_i2c,
							    CONNECTOR_OBJECT_ID_VGA,
							    &hpd);
			} else {
				DRM_DEBUG("No connector info found\n");
				return false;
			}
		}
	}

	if (rdev->flags & RADEON_IS_MOBILITY || rdev->flags & RADEON_IS_IGP) {
		uint16_t lcd_info =
		    combios_get_table_offset(dev, COMBIOS_LCD_INFO_TABLE);
		if (lcd_info) {
			uint16_t lcd_ddc_info =
			    combios_get_table_offset(dev,
						     COMBIOS_LCD_DDC_INFO_TABLE);

			radeon_add_legacy_encoder(dev,
						  radeon_get_encoder_id(dev,
									ATOM_DEVICE_LCD1_SUPPORT,
									0),
						  ATOM_DEVICE_LCD1_SUPPORT);

			if (lcd_ddc_info) {
				ddc_type = RBIOS8(lcd_ddc_info + 2);
				switch (ddc_type) {
				case DDC_MONID:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_GPIO_MONID);
					break;
				case DDC_DVI:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_GPIO_DVI_DDC);
					break;
				case DDC_VGA:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_GPIO_VGA_DDC);
					break;
				case DDC_CRT2:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_GPIO_CRT2_DDC);
					break;
				case DDC_LCD:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_GPIOPAD_MASK);
					ddc_i2c.mask_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.mask_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.a_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.a_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.en_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.en_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.y_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.y_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					break;
				case DDC_GPIO:
					ddc_i2c =
					    combios_setup_i2c_bus
						(rdev, RADEON_MDGPIO_MASK);
					ddc_i2c.mask_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.mask_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.a_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.a_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.en_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.en_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					ddc_i2c.y_clk_mask =
					    RBIOS32(lcd_ddc_info + 3);
					ddc_i2c.y_data_mask =
					    RBIOS32(lcd_ddc_info + 7);
					break;
				default:
					ddc_i2c.valid = false;
					break;
				}
				DRM_DEBUG("LCD DDC Info Table found!\n");
			} else
				ddc_i2c.valid = false;

			hpd.hpd = RADEON_HPD_NONE;
			radeon_add_legacy_connector(dev,
						    5,
						    ATOM_DEVICE_LCD1_SUPPORT,
						    DRM_MODE_CONNECTOR_LVDS,
						    &ddc_i2c,
						    CONNECTOR_OBJECT_ID_LVDS,
						    &hpd);
		}
	}

	/* check TV table */
	if (rdev->family != CHIP_R100 && rdev->family != CHIP_R200) {
		uint32_t tv_info =
		    combios_get_table_offset(dev, COMBIOS_TV_INFO_TABLE);
		if (tv_info) {
			if (RBIOS8(tv_info + 6) == 'T') {
				if (radeon_apply_legacy_tv_quirks(dev)) {
					hpd.hpd = RADEON_HPD_NONE;
					radeon_add_legacy_encoder(dev,
								  radeon_get_encoder_id
								  (dev,
								   ATOM_DEVICE_TV1_SUPPORT,
								   2),
								  ATOM_DEVICE_TV1_SUPPORT);
					radeon_add_legacy_connector(dev, 6,
								    ATOM_DEVICE_TV1_SUPPORT,
								    DRM_MODE_CONNECTOR_SVIDEO,
								    &ddc_i2c,
								    CONNECTOR_OBJECT_ID_SVIDEO,
								    &hpd);
				}
			}
		}
	}

	radeon_link_encoder_connector(dev);

	return true;
}

void radeon_external_tmds_setup(struct drm_encoder *encoder)
{
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder_ext_tmds *tmds = radeon_encoder->enc_priv;

	if (!tmds)
		return;

	switch (tmds->dvo_chip) {
	case DVO_SIL164:
		/* sil 164 */
		radeon_i2c_do_lock(tmds->i2c_bus, 1);
		radeon_i2c_sw_put_byte(tmds->i2c_bus,
				       tmds->slave_addr,
				       0x08, 0x30);
		radeon_i2c_sw_put_byte(tmds->i2c_bus,
				       tmds->slave_addr,
				       0x09, 0x00);
		radeon_i2c_sw_put_byte(tmds->i2c_bus,
				       tmds->slave_addr,
				       0x0a, 0x90);
		radeon_i2c_sw_put_byte(tmds->i2c_bus,
				       tmds->slave_addr,
				       0x0c, 0x89);
		radeon_i2c_sw_put_byte(tmds->i2c_bus,
				       tmds->slave_addr,
				       0x08, 0x3b);
		radeon_i2c_do_lock(tmds->i2c_bus, 0);
		break;
	case DVO_SIL1178:
		/* sil 1178 - untested */
		/*
		 * 0x0f, 0x44
		 * 0x0f, 0x4c
		 * 0x0e, 0x01
		 * 0x0a, 0x80
		 * 0x09, 0x30
		 * 0x0c, 0xc9
		 * 0x0d, 0x70
		 * 0x08, 0x32
		 * 0x08, 0x33
		 */
		break;
	default:
		break;
	}

}

bool radeon_combios_external_tmds_setup(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	uint16_t offset;
	uint8_t blocks, slave_addr, rev;
	uint32_t index, id;
	uint32_t reg, val, and_mask, or_mask;
	struct radeon_encoder_ext_tmds *tmds = radeon_encoder->enc_priv;

	if (rdev->bios == NULL)
		return false;

	if (!tmds)
		return false;

	if (rdev->flags & RADEON_IS_IGP) {
		offset = combios_get_table_offset(dev, COMBIOS_TMDS_POWER_ON_TABLE);
		rev = RBIOS8(offset);
		if (offset) {
			rev = RBIOS8(offset);
			if (rev > 1) {
				blocks = RBIOS8(offset + 3);
				index = offset + 4;
				while (blocks > 0) {
					id = RBIOS16(index);
					index += 2;
					switch (id >> 13) {
					case 0:
						reg = (id & 0x1fff) * 4;
						val = RBIOS32(index);
						index += 4;
						WREG32(reg, val);
						break;
					case 2:
						reg = (id & 0x1fff) * 4;
						and_mask = RBIOS32(index);
						index += 4;
						or_mask = RBIOS32(index);
						index += 4;
						val = RREG32(reg);
						val = (val & and_mask) | or_mask;
						WREG32(reg, val);
						break;
					case 3:
						val = RBIOS16(index);
						index += 2;
						udelay(val);
						break;
					case 4:
						val = RBIOS16(index);
						index += 2;
						udelay(val * 1000);
						break;
					case 6:
						slave_addr = id & 0xff;
						slave_addr >>= 1; /* 7 bit addressing */
						index++;
						reg = RBIOS8(index);
						index++;
						val = RBIOS8(index);
						index++;
						radeon_i2c_do_lock(tmds->i2c_bus, 1);
						radeon_i2c_sw_put_byte(tmds->i2c_bus,
								       slave_addr,
								       reg, val);
						radeon_i2c_do_lock(tmds->i2c_bus, 0);
						break;
					default:
						DRM_ERROR("Unknown id %d\n", id >> 13);
						break;
					}
					blocks--;
				}
				return true;
			}
		}
	} else {
		offset = combios_get_table_offset(dev, COMBIOS_EXT_TMDS_INFO_TABLE);
		if (offset) {
			index = offset + 10;
			id = RBIOS16(index);
			while (id != 0xffff) {
				index += 2;
				switch (id >> 13) {
				case 0:
					reg = (id & 0x1fff) * 4;
					val = RBIOS32(index);
					WREG32(reg, val);
					break;
				case 2:
					reg = (id & 0x1fff) * 4;
					and_mask = RBIOS32(index);
					index += 4;
					or_mask = RBIOS32(index);
					index += 4;
					val = RREG32(reg);
					val = (val & and_mask) | or_mask;
					WREG32(reg, val);
					break;
				case 4:
					val = RBIOS16(index);
					index += 2;
					udelay(val);
					break;
				case 5:
					reg = id & 0x1fff;
					and_mask = RBIOS32(index);
					index += 4;
					or_mask = RBIOS32(index);
					index += 4;
					val = RREG32_PLL(reg);
					val = (val & and_mask) | or_mask;
					WREG32_PLL(reg, val);
					break;
				case 6:
					reg = id & 0x1fff;
					val = RBIOS8(index);
					index += 1;
					radeon_i2c_do_lock(tmds->i2c_bus, 1);
					radeon_i2c_sw_put_byte(tmds->i2c_bus,
							       tmds->slave_addr,
							       reg, val);
					radeon_i2c_do_lock(tmds->i2c_bus, 0);
					break;
				default:
					DRM_ERROR("Unknown id %d\n", id >> 13);
					break;
				}
				id = RBIOS16(index);
			}
			return true;
		}
	}
	return false;
}

static void combios_parse_mmio_table(struct drm_device *dev, uint16_t offset)
{
	struct radeon_device *rdev = dev->dev_private;

	if (offset) {
		while (RBIOS16(offset)) {
			uint16_t cmd = ((RBIOS16(offset) & 0xe000) >> 13);
			uint32_t addr = (RBIOS16(offset) & 0x1fff);
			uint32_t val, and_mask, or_mask;
			uint32_t tmp;

			offset += 2;
			switch (cmd) {
			case 0:
				val = RBIOS32(offset);
				offset += 4;
				WREG32(addr, val);
				break;
			case 1:
				val = RBIOS32(offset);
				offset += 4;
				WREG32(addr, val);
				break;
			case 2:
				and_mask = RBIOS32(offset);
				offset += 4;
				or_mask = RBIOS32(offset);
				offset += 4;
				tmp = RREG32(addr);
				tmp &= and_mask;
				tmp |= or_mask;
				WREG32(addr, tmp);
				break;
			case 3:
				and_mask = RBIOS32(offset);
				offset += 4;
				or_mask = RBIOS32(offset);
				offset += 4;
				tmp = RREG32(addr);
				tmp &= and_mask;
				tmp |= or_mask;
				WREG32(addr, tmp);
				break;
			case 4:
				val = RBIOS16(offset);
				offset += 2;
				udelay(val);
				break;
			case 5:
				val = RBIOS16(offset);
				offset += 2;
				switch (addr) {
				case 8:
					while (val--) {
						if (!
						    (RREG32_PLL
						     (RADEON_CLK_PWRMGT_CNTL) &
						     RADEON_MC_BUSY))
							break;
					}
					break;
				case 9:
					while (val--) {
						if ((RREG32(RADEON_MC_STATUS) &
						     RADEON_MC_IDLE))
							break;
					}
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
	}
}

static void combios_parse_pll_table(struct drm_device *dev, uint16_t offset)
{
	struct radeon_device *rdev = dev->dev_private;

	if (offset) {
		while (RBIOS8(offset)) {
			uint8_t cmd = ((RBIOS8(offset) & 0xc0) >> 6);
			uint8_t addr = (RBIOS8(offset) & 0x3f);
			uint32_t val, shift, tmp;
			uint32_t and_mask, or_mask;

			offset++;
			switch (cmd) {
			case 0:
				val = RBIOS32(offset);
				offset += 4;
				WREG32_PLL(addr, val);
				break;
			case 1:
				shift = RBIOS8(offset) * 8;
				offset++;
				and_mask = RBIOS8(offset) << shift;
				and_mask |= ~(0xff << shift);
				offset++;
				or_mask = RBIOS8(offset) << shift;
				offset++;
				tmp = RREG32_PLL(addr);
				tmp &= and_mask;
				tmp |= or_mask;
				WREG32_PLL(addr, tmp);
				break;
			case 2:
			case 3:
				tmp = 1000;
				switch (addr) {
				case 1:
					udelay(150);
					break;
				case 2:
					udelay(1000);
					break;
				case 3:
					while (tmp--) {
						if (!
						    (RREG32_PLL
						     (RADEON_CLK_PWRMGT_CNTL) &
						     RADEON_MC_BUSY))
							break;
					}
					break;
				case 4:
					while (tmp--) {
						if (RREG32_PLL
						    (RADEON_CLK_PWRMGT_CNTL) &
						    RADEON_DLL_READY)
							break;
					}
					break;
				case 5:
					tmp =
					    RREG32_PLL(RADEON_CLK_PWRMGT_CNTL);
					if (tmp & RADEON_CG_NO1_DEBUG_0) {
#if 0
						uint32_t mclk_cntl =
						    RREG32_PLL
						    (RADEON_MCLK_CNTL);
						mclk_cntl &= 0xffff0000;
						/*mclk_cntl |= 0x00001111;*//* ??? */
						WREG32_PLL(RADEON_MCLK_CNTL,
							   mclk_cntl);
						udelay(10000);
#endif
						WREG32_PLL
						    (RADEON_CLK_PWRMGT_CNTL,
						     tmp &
						     ~RADEON_CG_NO1_DEBUG_0);
						udelay(10000);
					}
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
	}
}

static void combios_parse_ram_reset_table(struct drm_device *dev,
					  uint16_t offset)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t tmp;

	if (offset) {
		uint8_t val = RBIOS8(offset);
		while (val != 0xff) {
			offset++;

			if (val == 0x0f) {
				uint32_t channel_complete_mask;

				if (ASIC_IS_R300(rdev))
					channel_complete_mask =
					    R300_MEM_PWRUP_COMPLETE;
				else
					channel_complete_mask =
					    RADEON_MEM_PWRUP_COMPLETE;
				tmp = 20000;
				while (tmp--) {
					if ((RREG32(RADEON_MEM_STR_CNTL) &
					     channel_complete_mask) ==
					    channel_complete_mask)
						break;
				}
			} else {
				uint32_t or_mask = RBIOS16(offset);
				offset += 2;

				tmp = RREG32(RADEON_MEM_SDRAM_MODE_REG);
				tmp &= RADEON_SDRAM_MODE_MASK;
				tmp |= or_mask;
				WREG32(RADEON_MEM_SDRAM_MODE_REG, tmp);

				or_mask = val << 24;
				tmp = RREG32(RADEON_MEM_SDRAM_MODE_REG);
				tmp &= RADEON_B3MEM_RESET_MASK;
				tmp |= or_mask;
				WREG32(RADEON_MEM_SDRAM_MODE_REG, tmp);
			}
			val = RBIOS8(offset);
		}
	}
}

static uint32_t combios_detect_ram(struct drm_device *dev, int ram,
				   int mem_addr_mapping)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t mem_cntl;
	uint32_t mem_size;
	uint32_t addr = 0;

	mem_cntl = RREG32(RADEON_MEM_CNTL);
	if (mem_cntl & RV100_HALF_MODE)
		ram /= 2;
	mem_size = ram;
	mem_cntl &= ~(0xff << 8);
	mem_cntl |= (mem_addr_mapping & 0xff) << 8;
	WREG32(RADEON_MEM_CNTL, mem_cntl);
	RREG32(RADEON_MEM_CNTL);

	/* sdram reset ? */

	/* something like this????  */
	while (ram--) {
		addr = ram * 1024 * 1024;
		/* write to each page */
		WREG32(RADEON_MM_INDEX, (addr) | RADEON_MM_APER);
		WREG32(RADEON_MM_DATA, 0xdeadbeef);
		/* read back and verify */
		WREG32(RADEON_MM_INDEX, (addr) | RADEON_MM_APER);
		if (RREG32(RADEON_MM_DATA) != 0xdeadbeef)
			return 0;
	}

	return mem_size;
}

static void combios_write_ram_size(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint8_t rev;
	uint16_t offset;
	uint32_t mem_size = 0;
	uint32_t mem_cntl = 0;

	/* should do something smarter here I guess... */
	if (rdev->flags & RADEON_IS_IGP)
		return;

	/* first check detected mem table */
	offset = combios_get_table_offset(dev, COMBIOS_DETECTED_MEM_TABLE);
	if (offset) {
		rev = RBIOS8(offset);
		if (rev < 3) {
			mem_cntl = RBIOS32(offset + 1);
			mem_size = RBIOS16(offset + 5);
			if (((rdev->flags & RADEON_FAMILY_MASK) < CHIP_R200) &&
			    ((dev->pdev->device != 0x515e)
			     && (dev->pdev->device != 0x5969)))
				WREG32(RADEON_MEM_CNTL, mem_cntl);
		}
	}

	if (!mem_size) {
		offset =
		    combios_get_table_offset(dev, COMBIOS_MEM_CONFIG_TABLE);
		if (offset) {
			rev = RBIOS8(offset - 1);
			if (rev < 1) {
				if (((rdev->flags & RADEON_FAMILY_MASK) <
				     CHIP_R200)
				    && ((dev->pdev->device != 0x515e)
					&& (dev->pdev->device != 0x5969))) {
					int ram = 0;
					int mem_addr_mapping = 0;

					while (RBIOS8(offset)) {
						ram = RBIOS8(offset);
						mem_addr_mapping =
						    RBIOS8(offset + 1);
						if (mem_addr_mapping != 0x25)
							ram *= 2;
						mem_size =
						    combios_detect_ram(dev, ram,
								       mem_addr_mapping);
						if (mem_size)
							break;
						offset += 2;
					}
				} else
					mem_size = RBIOS8(offset);
			} else {
				mem_size = RBIOS8(offset);
				mem_size *= 2;	/* convert to MB */
			}
		}
	}

	mem_size *= (1024 * 1024);	/* convert to bytes */
	WREG32(RADEON_CONFIG_MEMSIZE, mem_size);
}

void radeon_combios_dyn_clk_setup(struct drm_device *dev, int enable)
{
	uint16_t dyn_clk_info =
	    combios_get_table_offset(dev, COMBIOS_DYN_CLK_1_TABLE);

	if (dyn_clk_info)
		combios_parse_pll_table(dev, dyn_clk_info);
}

void radeon_combios_asic_init(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint16_t table;

	/* port hardcoded mac stuff from radeonfb */
	if (rdev->bios == NULL)
		return;

	/* ASIC INIT 1 */
	table = combios_get_table_offset(dev, COMBIOS_ASIC_INIT_1_TABLE);
	if (table)
		combios_parse_mmio_table(dev, table);

	/* PLL INIT */
	table = combios_get_table_offset(dev, COMBIOS_PLL_INIT_TABLE);
	if (table)
		combios_parse_pll_table(dev, table);

	/* ASIC INIT 2 */
	table = combios_get_table_offset(dev, COMBIOS_ASIC_INIT_2_TABLE);
	if (table)
		combios_parse_mmio_table(dev, table);

	if (!(rdev->flags & RADEON_IS_IGP)) {
		/* ASIC INIT 4 */
		table =
		    combios_get_table_offset(dev, COMBIOS_ASIC_INIT_4_TABLE);
		if (table)
			combios_parse_mmio_table(dev, table);

		/* RAM RESET */
		table = combios_get_table_offset(dev, COMBIOS_RAM_RESET_TABLE);
		if (table)
			combios_parse_ram_reset_table(dev, table);

		/* ASIC INIT 3 */
		table =
		    combios_get_table_offset(dev, COMBIOS_ASIC_INIT_3_TABLE);
		if (table)
			combios_parse_mmio_table(dev, table);

		/* write CONFIG_MEMSIZE */
		combios_write_ram_size(dev);
	}

	/* DYN CLK 1 */
	table = combios_get_table_offset(dev, COMBIOS_DYN_CLK_1_TABLE);
	if (table)
		combios_parse_pll_table(dev, table);

}

void radeon_combios_initialize_bios_scratch_regs(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t bios_0_scratch, bios_6_scratch, bios_7_scratch;

	bios_0_scratch = RREG32(RADEON_BIOS_0_SCRATCH);
	bios_6_scratch = RREG32(RADEON_BIOS_6_SCRATCH);
	bios_7_scratch = RREG32(RADEON_BIOS_7_SCRATCH);

	/* let the bios control the backlight */
	bios_0_scratch &= ~RADEON_DRIVER_BRIGHTNESS_EN;

	/* tell the bios not to handle mode switching */
	bios_6_scratch |= (RADEON_DISPLAY_SWITCHING_DIS |
			   RADEON_ACC_MODE_CHANGE);

	/* tell the bios a driver is loaded */
	bios_7_scratch |= RADEON_DRV_LOADED;

	WREG32(RADEON_BIOS_0_SCRATCH, bios_0_scratch);
	WREG32(RADEON_BIOS_6_SCRATCH, bios_6_scratch);
	WREG32(RADEON_BIOS_7_SCRATCH, bios_7_scratch);
}

void radeon_combios_output_lock(struct drm_encoder *encoder, bool lock)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	uint32_t bios_6_scratch;

	bios_6_scratch = RREG32(RADEON_BIOS_6_SCRATCH);

	if (lock)
		bios_6_scratch |= RADEON_DRIVER_CRITICAL;
	else
		bios_6_scratch &= ~RADEON_DRIVER_CRITICAL;

	WREG32(RADEON_BIOS_6_SCRATCH, bios_6_scratch);
}

void
radeon_combios_connected_scratch_regs(struct drm_connector *connector,
				      struct drm_encoder *encoder,
				      bool connected)
{
	struct drm_device *dev = connector->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_connector *radeon_connector =
	    to_radeon_connector(connector);
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	uint32_t bios_4_scratch = RREG32(RADEON_BIOS_4_SCRATCH);
	uint32_t bios_5_scratch = RREG32(RADEON_BIOS_5_SCRATCH);

	if ((radeon_encoder->devices & ATOM_DEVICE_TV1_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_TV1_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("TV1 connected\n");
			/* fix me */
			bios_4_scratch |= RADEON_TV1_ATTACHED_SVIDEO;
			/*save->bios_4_scratch |= RADEON_TV1_ATTACHED_COMP; */
			bios_5_scratch |= RADEON_TV1_ON;
			bios_5_scratch |= RADEON_ACC_REQ_TV1;
		} else {
			DRM_DEBUG("TV1 disconnected\n");
			bios_4_scratch &= ~RADEON_TV1_ATTACHED_MASK;
			bios_5_scratch &= ~RADEON_TV1_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_TV1;
		}
	}
	if ((radeon_encoder->devices & ATOM_DEVICE_LCD1_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_LCD1_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("LCD1 connected\n");
			bios_4_scratch |= RADEON_LCD1_ATTACHED;
			bios_5_scratch |= RADEON_LCD1_ON;
			bios_5_scratch |= RADEON_ACC_REQ_LCD1;
		} else {
			DRM_DEBUG("LCD1 disconnected\n");
			bios_4_scratch &= ~RADEON_LCD1_ATTACHED;
			bios_5_scratch &= ~RADEON_LCD1_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_LCD1;
		}
	}
	if ((radeon_encoder->devices & ATOM_DEVICE_CRT1_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_CRT1_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("CRT1 connected\n");
			bios_4_scratch |= RADEON_CRT1_ATTACHED_COLOR;
			bios_5_scratch |= RADEON_CRT1_ON;
			bios_5_scratch |= RADEON_ACC_REQ_CRT1;
		} else {
			DRM_DEBUG("CRT1 disconnected\n");
			bios_4_scratch &= ~RADEON_CRT1_ATTACHED_MASK;
			bios_5_scratch &= ~RADEON_CRT1_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_CRT1;
		}
	}
	if ((radeon_encoder->devices & ATOM_DEVICE_CRT2_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_CRT2_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("CRT2 connected\n");
			bios_4_scratch |= RADEON_CRT2_ATTACHED_COLOR;
			bios_5_scratch |= RADEON_CRT2_ON;
			bios_5_scratch |= RADEON_ACC_REQ_CRT2;
		} else {
			DRM_DEBUG("CRT2 disconnected\n");
			bios_4_scratch &= ~RADEON_CRT2_ATTACHED_MASK;
			bios_5_scratch &= ~RADEON_CRT2_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_CRT2;
		}
	}
	if ((radeon_encoder->devices & ATOM_DEVICE_DFP1_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_DFP1_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("DFP1 connected\n");
			bios_4_scratch |= RADEON_DFP1_ATTACHED;
			bios_5_scratch |= RADEON_DFP1_ON;
			bios_5_scratch |= RADEON_ACC_REQ_DFP1;
		} else {
			DRM_DEBUG("DFP1 disconnected\n");
			bios_4_scratch &= ~RADEON_DFP1_ATTACHED;
			bios_5_scratch &= ~RADEON_DFP1_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_DFP1;
		}
	}
	if ((radeon_encoder->devices & ATOM_DEVICE_DFP2_SUPPORT) &&
	    (radeon_connector->devices & ATOM_DEVICE_DFP2_SUPPORT)) {
		if (connected) {
			DRM_DEBUG("DFP2 connected\n");
			bios_4_scratch |= RADEON_DFP2_ATTACHED;
			bios_5_scratch |= RADEON_DFP2_ON;
			bios_5_scratch |= RADEON_ACC_REQ_DFP2;
		} else {
			DRM_DEBUG("DFP2 disconnected\n");
			bios_4_scratch &= ~RADEON_DFP2_ATTACHED;
			bios_5_scratch &= ~RADEON_DFP2_ON;
			bios_5_scratch &= ~RADEON_ACC_REQ_DFP2;
		}
	}
	WREG32(RADEON_BIOS_4_SCRATCH, bios_4_scratch);
	WREG32(RADEON_BIOS_5_SCRATCH, bios_5_scratch);
}

void
radeon_combios_encoder_crtc_scratch_regs(struct drm_encoder *encoder, int crtc)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	uint32_t bios_5_scratch = RREG32(RADEON_BIOS_5_SCRATCH);

	if (radeon_encoder->devices & ATOM_DEVICE_TV1_SUPPORT) {
		bios_5_scratch &= ~RADEON_TV1_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_TV1_CRTC_SHIFT);
	}
	if (radeon_encoder->devices & ATOM_DEVICE_CRT1_SUPPORT) {
		bios_5_scratch &= ~RADEON_CRT1_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_CRT1_CRTC_SHIFT);
	}
	if (radeon_encoder->devices & ATOM_DEVICE_CRT2_SUPPORT) {
		bios_5_scratch &= ~RADEON_CRT2_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_CRT2_CRTC_SHIFT);
	}
	if (radeon_encoder->devices & ATOM_DEVICE_LCD1_SUPPORT) {
		bios_5_scratch &= ~RADEON_LCD1_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_LCD1_CRTC_SHIFT);
	}
	if (radeon_encoder->devices & ATOM_DEVICE_DFP1_SUPPORT) {
		bios_5_scratch &= ~RADEON_DFP1_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_DFP1_CRTC_SHIFT);
	}
	if (radeon_encoder->devices & ATOM_DEVICE_DFP2_SUPPORT) {
		bios_5_scratch &= ~RADEON_DFP2_CRTC_MASK;
		bios_5_scratch |= (crtc << RADEON_DFP2_CRTC_SHIFT);
	}
	WREG32(RADEON_BIOS_5_SCRATCH, bios_5_scratch);
}

void
radeon_combios_encoder_dpms_scratch_regs(struct drm_encoder *encoder, bool on)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	uint32_t bios_6_scratch = RREG32(RADEON_BIOS_6_SCRATCH);

	if (radeon_encoder->devices & (ATOM_DEVICE_TV_SUPPORT)) {
		if (on)
			bios_6_scratch |= RADEON_TV_DPMS_ON;
		else
			bios_6_scratch &= ~RADEON_TV_DPMS_ON;
	}
	if (radeon_encoder->devices & (ATOM_DEVICE_CRT_SUPPORT)) {
		if (on)
			bios_6_scratch |= RADEON_CRT_DPMS_ON;
		else
			bios_6_scratch &= ~RADEON_CRT_DPMS_ON;
	}
	if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
		if (on)
			bios_6_scratch |= RADEON_LCD_DPMS_ON;
		else
			bios_6_scratch &= ~RADEON_LCD_DPMS_ON;
	}
	if (radeon_encoder->devices & (ATOM_DEVICE_DFP_SUPPORT)) {
		if (on)
			bios_6_scratch |= RADEON_DFP_DPMS_ON;
		else
			bios_6_scratch &= ~RADEON_DFP_DPMS_ON;
	}
	WREG32(RADEON_BIOS_6_SCRATCH, bios_6_scratch);
}
