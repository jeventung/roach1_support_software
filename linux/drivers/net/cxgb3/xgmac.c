/*
 * Copyright (c) 2005-2007 Chelsio, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "common.h"
#include "regs.h"

/*
 * # of exact address filters.  The first one is used for the station address,
 * the rest are available for multicast addresses.
 */
#define EXACT_ADDR_FILTERS 8

static inline int macidx(const struct cmac *mac)
{
	return mac->offset / (XGMAC0_1_BASE_ADDR - XGMAC0_0_BASE_ADDR);
}

static void xaui_serdes_reset(struct cmac *mac)
{
	static const unsigned int clear[] = {
		F_PWRDN0 | F_PWRDN1, F_RESETPLL01, F_RESET0 | F_RESET1,
		F_PWRDN2 | F_PWRDN3, F_RESETPLL23, F_RESET2 | F_RESET3
	};

	int i;
	struct adapter *adap = mac->adapter;
	u32 ctrl = A_XGM_SERDES_CTRL0 + mac->offset;

	t3_write_reg(adap, ctrl, adap->params.vpd.xauicfg[macidx(mac)] |
		     F_RESET3 | F_RESET2 | F_RESET1 | F_RESET0 |
		     F_PWRDN3 | F_PWRDN2 | F_PWRDN1 | F_PWRDN0 |
		     F_RESETPLL23 | F_RESETPLL01);
	t3_read_reg(adap, ctrl);
	udelay(15);

	for (i = 0; i < ARRAY_SIZE(clear); i++) {
		t3_set_reg_field(adap, ctrl, clear[i], 0);
		udelay(15);
	}
}

void t3b_pcs_reset(struct cmac *mac)
{
	t3_set_reg_field(mac->adapter, A_XGM_RESET_CTRL + mac->offset,
			 F_PCS_RESET_, 0);
	udelay(20);
	t3_set_reg_field(mac->adapter, A_XGM_RESET_CTRL + mac->offset, 0,
			 F_PCS_RESET_);
}

int t3_mac_reset(struct cmac *mac)
{
	static const struct addr_val_pair mac_reset_avp[] = {
		{A_XGM_TX_CTRL, 0},
		{A_XGM_RX_CTRL, 0},
		{A_XGM_RX_CFG, F_DISPAUSEFRAMES | F_EN1536BFRAMES |
		 F_RMFCS | F_ENJUMBO | F_ENHASHMCAST},
		{A_XGM_RX_HASH_LOW, 0},
		{A_XGM_RX_HASH_HIGH, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_1, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_2, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_3, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_4, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_5, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_6, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_7, 0},
		{A_XGM_RX_EXACT_MATCH_LOW_8, 0},
		{A_XGM_STAT_CTRL, F_CLRSTATS}
	};
	u32 val;
	struct adapter *adap = mac->adapter;
	unsigned int oft = mac->offset;

	t3_write_reg(adap, A_XGM_RESET_CTRL + oft, F_MAC_RESET_);
	t3_read_reg(adap, A_XGM_RESET_CTRL + oft);	/* flush */

	t3_write_regs(adap, mac_reset_avp, ARRAY_SIZE(mac_reset_avp), oft);
	t3_set_reg_field(adap, A_XGM_RXFIFO_CFG + oft,
			 F_RXSTRFRWRD | F_DISERRFRAMES,
			 uses_xaui(adap) ? 0 : F_RXSTRFRWRD);

	if (uses_xaui(adap)) {
		if (adap->params.rev == 0) {
			t3_set_reg_field(adap, A_XGM_SERDES_CTRL + oft, 0,
					 F_RXENABLE | F_TXENABLE);
			if (t3_wait_op_done(adap, A_XGM_SERDES_STATUS1 + oft,
					    F_CMULOCK, 1, 5, 2)) {
				CH_ERR(adap,
				       "MAC %d XAUI SERDES CMU lock failed\n",
				       macidx(mac));
				return -1;
			}
			t3_set_reg_field(adap, A_XGM_SERDES_CTRL + oft, 0,
					 F_SERDESRESET_);
		} else
			xaui_serdes_reset(mac);
	}

	if (adap->params.rev > 0)
		t3_write_reg(adap, A_XGM_PAUSE_TIMER + oft, 0xf000);

	val = F_MAC_RESET_;
	if (is_10G(adap))
		val |= F_PCS_RESET_;
	else if (uses_xaui(adap))
		val |= F_PCS_RESET_ | F_XG2G_RESET_;
	else
		val |= F_RGMII_RESET_ | F_XG2G_RESET_;
	t3_write_reg(adap, A_XGM_RESET_CTRL + oft, val);
	t3_read_reg(adap, A_XGM_RESET_CTRL + oft);	/* flush */
	if ((val & F_PCS_RESET_) && adap->params.rev) {
		msleep(1);
		t3b_pcs_reset(mac);
	}

	memset(&mac->stats, 0, sizeof(mac->stats));
	return 0;
}

/*
 * Set the exact match register 'idx' to recognize the given Ethernet address.
 */
static void set_addr_filter(struct cmac *mac, int idx, const u8 * addr)
{
	u32 addr_lo, addr_hi;
	unsigned int oft = mac->offset + idx * 8;

	addr_lo = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	addr_hi = (addr[5] << 8) | addr[4];

	t3_write_reg(mac->adapter, A_XGM_RX_EXACT_MATCH_LOW_1 + oft, addr_lo);
	t3_write_reg(mac->adapter, A_XGM_RX_EXACT_MATCH_HIGH_1 + oft, addr_hi);
}

/* Set one of the station's unicast MAC addresses. */
int t3_mac_set_address(struct cmac *mac, unsigned int idx, u8 addr[6])
{
	if (idx >= mac->nucast)
		return -EINVAL;
	set_addr_filter(mac, idx, addr);
	return 0;
}

/*
 * Specify the number of exact address filters that should be reserved for
 * unicast addresses.  Caller should reload the unicast and multicast addresses
 * after calling this.
 */
int t3_mac_set_num_ucast(struct cmac *mac, int n)
{
	if (n > EXACT_ADDR_FILTERS)
		return -EINVAL;
	mac->nucast = n;
	return 0;
}

/* Calculate the RX hash filter index of an Ethernet address */
static int hash_hw_addr(const u8 * addr)
{
	int hash = 0, octet, bit, i = 0, c;

	for (octet = 0; octet < 6; ++octet)
		for (c = addr[octet], bit = 0; bit < 8; c >>= 1, ++bit) {
			hash ^= (c & 1) << i;
			if (++i == 6)
				i = 0;
		}
	return hash;
}

int t3_mac_set_rx_mode(struct cmac *mac, struct t3_rx_mode *rm)
{
	u32 val, hash_lo, hash_hi;
	struct adapter *adap = mac->adapter;
	unsigned int oft = mac->offset;

	val = t3_read_reg(adap, A_XGM_RX_CFG + oft) & ~F_COPYALLFRAMES;
	if (rm->dev->flags & IFF_PROMISC)
		val |= F_COPYALLFRAMES;
	t3_write_reg(adap, A_XGM_RX_CFG + oft, val);

	if (rm->dev->flags & IFF_ALLMULTI)
		hash_lo = hash_hi = 0xffffffff;
	else {
		u8 *addr;
		int exact_addr_idx = mac->nucast;

		hash_lo = hash_hi = 0;
		while ((addr = t3_get_next_mcaddr(rm)))
			if (exact_addr_idx < EXACT_ADDR_FILTERS)
				set_addr_filter(mac, exact_addr_idx++, addr);
			else {
				int hash = hash_hw_addr(addr);

				if (hash < 32)
					hash_lo |= (1 << hash);
				else
					hash_hi |= (1 << (hash - 32));
			}
	}

	t3_write_reg(adap, A_XGM_RX_HASH_LOW + oft, hash_lo);
	t3_write_reg(adap, A_XGM_RX_HASH_HIGH + oft, hash_hi);
	return 0;
}

int t3_mac_set_mtu(struct cmac *mac, unsigned int mtu)
{
	int hwm, lwm;
	unsigned int thres, v;
	struct adapter *adap = mac->adapter;

	/*
	 * MAX_FRAME_SIZE inludes header + FCS, mtu doesn't.  The HW max
	 * packet size register includes header, but not FCS.
	 */
	mtu += 14;
	if (mtu > MAX_FRAME_SIZE - 4)
		return -EINVAL;
	t3_write_reg(adap, A_XGM_RX_MAX_PKT_SIZE + mac->offset, mtu);

	/*
	 * Adjust the PAUSE frame watermarks.  We always set the LWM, and the
	 * HWM only if flow-control is enabled.
	 */
	hwm = max(MAC_RXFIFO_SIZE - 3 * mtu, MAC_RXFIFO_SIZE / 2U);
	hwm = min(hwm, 3 * MAC_RXFIFO_SIZE / 4 + 1024);
	lwm = hwm - 1024;
	v = t3_read_reg(adap, A_XGM_RXFIFO_CFG + mac->offset);
	v &= ~V_RXFIFOPAUSELWM(M_RXFIFOPAUSELWM);
	v |= V_RXFIFOPAUSELWM(lwm / 8);
	if (G_RXFIFOPAUSEHWM(v))
		v = (v & ~V_RXFIFOPAUSEHWM(M_RXFIFOPAUSEHWM)) |
		    V_RXFIFOPAUSEHWM(hwm / 8);
	t3_write_reg(adap, A_XGM_RXFIFO_CFG + mac->offset, v);

	/* Adjust the TX FIFO threshold based on the MTU */
	thres = (adap->params.vpd.cclk * 1000) / 15625;
	thres = (thres * mtu) / 1000;
	if (is_10G(adap))
		thres /= 10;
	thres = mtu > thres ? (mtu - thres + 7) / 8 : 0;
	thres = max(thres, 8U);	/* need at least 8 */
	t3_set_reg_field(adap, A_XGM_TXFIFO_CFG + mac->offset,
			 V_TXFIFOTHRESH(M_TXFIFOTHRESH), V_TXFIFOTHRESH(thres));
	return 0;
}

int t3_mac_set_speed_duplex_fc(struct cmac *mac, int speed, int duplex, int fc)
{
	u32 val;
	struct adapter *adap = mac->adapter;
	unsigned int oft = mac->offset;

	if (duplex >= 0 && duplex != DUPLEX_FULL)
		return -EINVAL;
	if (speed >= 0) {
		if (speed == SPEED_10)
			val = V_PORTSPEED(0);
		else if (speed == SPEED_100)
			val = V_PORTSPEED(1);
		else if (speed == SPEED_1000)
			val = V_PORTSPEED(2);
		else if (speed == SPEED_10000)
			val = V_PORTSPEED(3);
		else
			return -EINVAL;

		t3_set_reg_field(adap, A_XGM_PORT_CFG + oft,
				 V_PORTSPEED(M_PORTSPEED), val);
	}

	val = t3_read_reg(adap, A_XGM_RXFIFO_CFG + oft);
	val &= ~V_RXFIFOPAUSEHWM(M_RXFIFOPAUSEHWM);
	if (fc & PAUSE_TX)
		val |= V_RXFIFOPAUSEHWM(G_RXFIFOPAUSELWM(val) + 128);	/* +1KB */
	t3_write_reg(adap, A_XGM_RXFIFO_CFG + oft, val);

	t3_set_reg_field(adap, A_XGM_TX_CFG + oft, F_TXPAUSEEN,
			 (fc & PAUSE_RX) ? F_TXPAUSEEN : 0);
	return 0;
}

int t3_mac_enable(struct cmac *mac, int which)
{
	int idx = macidx(mac);
	struct adapter *adap = mac->adapter;
	unsigned int oft = mac->offset;

	if (which & MAC_DIRECTION_TX) {
		t3_write_reg(adap, A_XGM_TX_CTRL + oft, F_TXEN);
		t3_write_reg(adap, A_TP_PIO_ADDR, A_TP_TX_DROP_CFG_CH0 + idx);
		t3_write_reg(adap, A_TP_PIO_DATA, 0xbf000001);
		t3_write_reg(adap, A_TP_PIO_ADDR, A_TP_TX_DROP_MODE);
		t3_set_reg_field(adap, A_TP_PIO_DATA, 1 << idx, 1 << idx);
	}
	if (which & MAC_DIRECTION_RX)
		t3_write_reg(adap, A_XGM_RX_CTRL + oft, F_RXEN);
	return 0;
}

int t3_mac_disable(struct cmac *mac, int which)
{
	int idx = macidx(mac);
	struct adapter *adap = mac->adapter;

	if (which & MAC_DIRECTION_TX) {
		t3_write_reg(adap, A_XGM_TX_CTRL + mac->offset, 0);
		t3_write_reg(adap, A_TP_PIO_ADDR, A_TP_TX_DROP_CFG_CH0 + idx);
		t3_write_reg(adap, A_TP_PIO_DATA, 0xc000001f);
		t3_write_reg(adap, A_TP_PIO_ADDR, A_TP_TX_DROP_MODE);
		t3_set_reg_field(adap, A_TP_PIO_DATA, 1 << idx, 0);
	}
	if (which & MAC_DIRECTION_RX)
		t3_write_reg(adap, A_XGM_RX_CTRL + mac->offset, 0);
	return 0;
}

/*
 * This function is called periodically to accumulate the current values of the
 * RMON counters into the port statistics.  Since the packet counters are only
 * 32 bits they can overflow in ~286 secs at 10G, so the function should be
 * called more frequently than that.  The byte counters are 45-bit wide, they
 * would overflow in ~7.8 hours.
 */
const struct mac_stats *t3_mac_update_stats(struct cmac *mac)
{
#define RMON_READ(mac, addr) t3_read_reg(mac->adapter, addr + mac->offset)
#define RMON_UPDATE(mac, name, reg) \
	(mac)->stats.name += (u64)RMON_READ(mac, A_XGM_STAT_##reg)
#define RMON_UPDATE64(mac, name, reg_lo, reg_hi) \
	(mac)->stats.name += RMON_READ(mac, A_XGM_STAT_##reg_lo) + \
			     ((u64)RMON_READ(mac, A_XGM_STAT_##reg_hi) << 32)

	u32 v, lo;

	RMON_UPDATE64(mac, rx_octets, RX_BYTES_LOW, RX_BYTES_HIGH);
	RMON_UPDATE64(mac, rx_frames, RX_FRAMES_LOW, RX_FRAMES_HIGH);
	RMON_UPDATE(mac, rx_mcast_frames, RX_MCAST_FRAMES);
	RMON_UPDATE(mac, rx_bcast_frames, RX_BCAST_FRAMES);
	RMON_UPDATE(mac, rx_fcs_errs, RX_CRC_ERR_FRAMES);
	RMON_UPDATE(mac, rx_pause, RX_PAUSE_FRAMES);
	RMON_UPDATE(mac, rx_jabber, RX_JABBER_FRAMES);
	RMON_UPDATE(mac, rx_short, RX_SHORT_FRAMES);
	RMON_UPDATE(mac, rx_symbol_errs, RX_SYM_CODE_ERR_FRAMES);

	RMON_UPDATE(mac, rx_too_long, RX_OVERSIZE_FRAMES);
	mac->stats.rx_too_long += RMON_READ(mac, A_XGM_RX_MAX_PKT_SIZE_ERR_CNT);

	RMON_UPDATE(mac, rx_frames_64, RX_64B_FRAMES);
	RMON_UPDATE(mac, rx_frames_65_127, RX_65_127B_FRAMES);
	RMON_UPDATE(mac, rx_frames_128_255, RX_128_255B_FRAMES);
	RMON_UPDATE(mac, rx_frames_256_511, RX_256_511B_FRAMES);
	RMON_UPDATE(mac, rx_frames_512_1023, RX_512_1023B_FRAMES);
	RMON_UPDATE(mac, rx_frames_1024_1518, RX_1024_1518B_FRAMES);
	RMON_UPDATE(mac, rx_frames_1519_max, RX_1519_MAXB_FRAMES);

	RMON_UPDATE64(mac, tx_octets, TX_BYTE_LOW, TX_BYTE_HIGH);
	RMON_UPDATE64(mac, tx_frames, TX_FRAME_LOW, TX_FRAME_HIGH);
	RMON_UPDATE(mac, tx_mcast_frames, TX_MCAST);
	RMON_UPDATE(mac, tx_bcast_frames, TX_BCAST);
	RMON_UPDATE(mac, tx_pause, TX_PAUSE);
	/* This counts error frames in general (bad FCS, underrun, etc). */
	RMON_UPDATE(mac, tx_underrun, TX_ERR_FRAMES);

	RMON_UPDATE(mac, tx_frames_64, TX_64B_FRAMES);
	RMON_UPDATE(mac, tx_frames_65_127, TX_65_127B_FRAMES);
	RMON_UPDATE(mac, tx_frames_128_255, TX_128_255B_FRAMES);
	RMON_UPDATE(mac, tx_frames_256_511, TX_256_511B_FRAMES);
	RMON_UPDATE(mac, tx_frames_512_1023, TX_512_1023B_FRAMES);
	RMON_UPDATE(mac, tx_frames_1024_1518, TX_1024_1518B_FRAMES);
	RMON_UPDATE(mac, tx_frames_1519_max, TX_1519_MAXB_FRAMES);

	/* The next stat isn't clear-on-read. */
	t3_write_reg(mac->adapter, A_TP_MIB_INDEX, mac->offset ? 51 : 50);
	v = t3_read_reg(mac->adapter, A_TP_MIB_RDATA);
	lo = (u32) mac->stats.rx_cong_drops;
	mac->stats.rx_cong_drops += (u64) (v - lo);

	return &mac->stats;
}
