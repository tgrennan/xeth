/* SPDX-License-Identifier: GPL-2.0
 * Copyright(c) 2018-2019 Platina Systems, Inc.
 *
 * Contact Information:
 * sw@platina.com
 * Platina Systems, 3180 Del La Cruz Blvd, Santa Clara, CA 95054
 */

#ifndef __NET_ETHERNET_XETH_H
#define __NET_ETHERNET_XETH_H

#include <linux/printk.h>
#include <linux/atomic.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/if_link.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <net/netevent.h>
#include <net/rtnetlink.h>

#include <generated/uapi/linux/version.h>

#if !defined(LINUX_VERSION_CODE) || \
	(LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))
#  define fib_notifier_info_with_net true
#else
#  define fib_notifier_info_without_net true
#endif

extern const char *xeth_mod_name;

#define xeth_debug(format, args...)					\
do {									\
	pr_debug(format "\n", ##args);					\
} while(0)

#define no_xeth_debug(format, args...)					\
do {									\
	no_printk(KERN_DEBUG pr_fmt(format), ##args);			\
} while(0)

#define xeth_nd_debug(nd, format, args...)				\
do {									\
	netdev_dbg(nd, format "\n", ##args);				\
} while(0)

#define no_xeth_nd_debug(nd, format, args...)				\
do {									\
	no_printk(KERN_DEBUG pr_fmt(format), ##args);			\
} while(0)

#define xeth_debug_skb(skb)						\
do {									\
	char _txt[64];							\
	snprintf(_txt, sizeof(_txt), "%s:%s:%s: ",			\
		 xeth_mod_name, __func__, netdev_name(skb->dev));	\
	print_hex_dump_bytes(_txt, DUMP_PREFIX_NONE,			\
			     skb->data,	skb->len);			\
} while(0)

#define no_xeth_debug_skb(skb)	do ; while(0)

#define xeth_debug_buf(buf, len)					\
do {									\
	char _txt[64];							\
	snprintf(_txt, sizeof(_txt), "%s:%s: ",				\
		 xeth_mod_name, __func__);				\
	print_hex_dump_bytes(_txt, DUMP_PREFIX_NONE, buf, len);		\
} while(0)

#define no_xeth_debug_buf(buf, len)	do ; while(0)

static inline void xeth_debug_test(void)
{
	struct net_device *lo;
	const char buf[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	size_t n = ARRAY_SIZE(buf);

	xeth_debug("begin debug test...");
	xeth_debug_buf(buf, n);
	lo = dev_get_by_name(&init_net, "lo");
	if (lo) {
		struct sk_buff *skb = netdev_alloc_skb(lo, n);
		if (skb) {
			xeth_nd_debug(lo, "test");
			skb_put(skb, n);
			memcpy(skb->data, buf, n);
			xeth_debug_skb(skb);
			kfree_skb(skb);
		}
		dev_put(lo);
	}
	xeth_debug("...end debug test");
}

static inline void no_xeth_debug_test(void) {}

#define xeth_err(format, args...)					\
do {									\
	pr_err("%s:%s: " format "\n", xeth_mod_name, __func__, ##args);	\
} while(0)

#define xeth_prefix_err(prefix, format, args...)			\
do {									\
	pr_err("%s:%s:%s: " format "\n",  xeth_mod_name, __func__,	\
	       prefix, ##args);						\
} while(0)

#define xeth_nd_err(nd, format, args...)				\
do {									\
	xeth_prefix_err(netdev_name(nd), format, ##args);		\
} while(0)

#define xeth_nd_prefix_err(nd, prefix, format, args...)			\
do {									\
	char _txt[80];							\
	snprintf(_txt, sizeof(_txt), "%s:%s", netdev_name(nd), prefix);	\
	xeth_prefix_err(_txt, format, ##args);				\
} while(0)

/* prif - PRint IF error */

#define xeth_prif_err(expr)						\
({									\
	int _err = (expr);						\
	if (_err < 0)							\
		xeth_prefix_err(#expr, "%d", _err);			\
	(_err);								\
})

#define xeth_prif_ptr_err(expr)						\
({									\
	void *_ptr = (expr);						\
	if (!_ptr) {							\
		xeth_prefix_err(#expr, "NULL");				\
		_ptr = ERR_PTR(-ENOMEM);				\
	} else if (IS_ERR(_ptr))					\
		xeth_prefix_err(#expr, "%ld", PTR_ERR(_ptr));		\
	(_ptr);								\
})

#define xeth_nd_prif_err(nd, expr)					\
({									\
	int _err = (expr);						\
	if (_err < 0)							\
		xeth_nd_prefix_err(nd, #expr, "%d", _err);		\
	(_err);								\
})

#define xeth_nd_prif_ptr_err(nd, expr)					\
({									\
	void *_ptr = (expr);						\
	if (!_ptr) {							\
		xeth_nd_prefix_err(nd, #expr, "NULL");			\
		_ptr = ERR_PTR(-ENOMEM);				\
	} else if (IS_ERR(_ptr))					\
		xeth_nd_prefix_err(nd, #expr, "%ld", PTR_ERR(_ptr));	\
	(_ptr);								\
})

#define xeth_container_of(ptr, type, member)				\
	xeth_prif_ptr_err(container_of(ptr, type, member))

#define xeth_prif_held_rcu(expr)					\
({									\
	typeof (expr) _v;						\
	bool held = rcu_read_lock_held();				\
	if (!held)							\
		rcu_read_lock();					\
	else								\
		xeth_prefix_err(#expr, "held rcu");			\
	_v = expr;							\
	if (!held)							\
		rcu_read_unlock();					\
	(_v);								\
})

static inline int _xeth_rcu_held_test(void)
{
	return 0;
}

#define xeth_prif_unheld_rcu(expr)					\
({									\
	typeof (expr) _v;						\
	bool held = rcu_read_lock_held();				\
	if (!held) {							\
		rcu_read_lock();					\
		xeth_prefix_err(#expr, "unheld rcu");			\
	}								\
	_v = expr;							\
	if (!held)							\
		rcu_read_unlock();					\
	(_v);								\
})

static inline int _xeth_rcu_unheld_test(void)
{
	return 0;
}

static inline void xeth_err_test(void)
{
	struct net_device *lo;

	xeth_err("begin error test...");
	xeth_prif_err(-EINVAL);
	xeth_prif_ptr_err(NULL);
	xeth_prif_ptr_err(ERR_PTR(-ENOMEM));
	lo = xeth_prif_ptr_err(dev_get_by_name(&init_net, "lo"));
	if (!IS_ERR(lo)) {
		xeth_nd_err(lo, "test");
		xeth_nd_prif_err(lo, -EINVAL);
		xeth_nd_prif_ptr_err(lo, NULL);
		xeth_nd_prif_ptr_err(lo, ERR_PTR(-ENOMEM));
		dev_put(lo);
	}
	if (rcu_read_lock_held()) {
		xeth_prif_held_rcu(_xeth_rcu_held_test());
	} else {
		xeth_prif_unheld_rcu(_xeth_rcu_unheld_test());
		rcu_read_lock();
		xeth_prif_held_rcu(_xeth_rcu_held_test());
		rcu_read_unlock();
	}
	xeth_err("...end error test");
}

static inline void no_xeth_err_test(void) {}

extern struct rtnl_link_ops xeth_bridge_lnko;

static inline bool is_xeth_bridge(struct net_device *nd)
{
	return nd->rtnl_link_ops == &xeth_bridge_lnko;
}

extern struct rtnl_link_ops xeth_lag_lnko;

static inline bool is_xeth_lag(struct net_device *nd)
{
	return nd->rtnl_link_ops == &xeth_lag_lnko;
}

extern struct rtnl_link_ops xeth_lb_lnko;

static inline bool is_xeth_lb(struct net_device *nd)
{
	return nd->rtnl_link_ops == &xeth_lb_lnko;
}

u8 xeth_lb_chan(struct net_device *nd);

static inline void xeth_link_stat_init(atomic64_t *t)
{
	enum xeth_link_stat s;
	for (s = 0; s < XETH_N_LINK_STAT; s++, t++)
		atomic64_set(t, 0LL);
}

#define xeth_link_stat_ops(NAME)					\
static inline long long xeth_get_##NAME(atomic64_t *t)			\
{									\
	return atomic64_read(&t[XETH_LINK_STAT_##NAME]);		\
}									\
static inline void xeth_add_##NAME(atomic64_t *t, s64 n)		\
{									\
	atomic64_add(n, &t[XETH_LINK_STAT_##NAME]);			\
}									\
static inline void xeth_inc_##NAME(atomic64_t *t)			\
{									\
	atomic64_inc(&t[XETH_LINK_STAT_##NAME]);			\
}									\
static inline void xeth_set_##NAME(atomic64_t *t, s64 n)		\
{									\
	atomic64_set(&t[XETH_LINK_STAT_##NAME], n);			\
}

xeth_link_stat_ops(RX_PACKETS)
xeth_link_stat_ops(TX_PACKETS)
xeth_link_stat_ops(RX_BYTES)
xeth_link_stat_ops(TX_BYTES)
xeth_link_stat_ops(RX_ERRORS)
xeth_link_stat_ops(TX_ERRORS)
xeth_link_stat_ops(RX_DROPPED)
xeth_link_stat_ops(TX_DROPPED)
xeth_link_stat_ops(MULTICAST)
xeth_link_stat_ops(COLLISIONS)
xeth_link_stat_ops(RX_LENGTH_ERRORS)
xeth_link_stat_ops(RX_OVER_ERRORS)
xeth_link_stat_ops(RX_CRC_ERRORS)
xeth_link_stat_ops(RX_FRAME_ERRORS)
xeth_link_stat_ops(RX_FIFO_ERRORS)
xeth_link_stat_ops(RX_MISSED_ERRORS)
xeth_link_stat_ops(TX_ABORTED_ERRORS)
xeth_link_stat_ops(TX_CARRIER_ERRORS)
xeth_link_stat_ops(TX_FIFO_ERRORS)
xeth_link_stat_ops(TX_HEARTBEAT_ERRORS)
xeth_link_stat_ops(TX_WINDOW_ERRORS)
xeth_link_stat_ops(RX_COMPRESSED)
xeth_link_stat_ops(TX_COMPRESSED)
xeth_link_stat_ops(RX_NOHANDLER)

static inline void xeth_link_stats(struct rtnl_link_stats64 *dst,
				   atomic64_t *src)
{
	dst->rx_packets = xeth_get_RX_PACKETS(src);
	dst->tx_packets = xeth_get_TX_PACKETS(src);
	dst->rx_bytes = xeth_get_RX_BYTES(src);
	dst->tx_bytes = xeth_get_TX_BYTES(src);
	dst->rx_errors = xeth_get_RX_ERRORS(src);
	dst->tx_errors = xeth_get_TX_ERRORS(src);
	dst->rx_dropped = xeth_get_RX_DROPPED(src);
	dst->tx_dropped = xeth_get_TX_DROPPED(src);
	dst->multicast = xeth_get_MULTICAST(src);
	dst->collisions = xeth_get_COLLISIONS(src);
	dst->rx_length_errors = xeth_get_RX_LENGTH_ERRORS(src);
	dst->rx_over_errors = xeth_get_RX_OVER_ERRORS(src);
	dst->rx_crc_errors = xeth_get_RX_CRC_ERRORS(src);
	dst->rx_frame_errors = xeth_get_RX_FRAME_ERRORS(src);
	dst->rx_fifo_errors = xeth_get_RX_FIFO_ERRORS(src);
	dst->rx_missed_errors = xeth_get_RX_MISSED_ERRORS(src);
	dst->tx_aborted_errors = xeth_get_TX_ABORTED_ERRORS(src);
	dst->tx_carrier_errors = xeth_get_TX_CARRIER_ERRORS(src);
	dst->tx_fifo_errors = xeth_get_TX_FIFO_ERRORS(src);
	dst->tx_heartbeat_errors = xeth_get_TX_HEARTBEAT_ERRORS(src);
	dst->tx_window_errors = xeth_get_TX_WINDOW_ERRORS(src);
	dst->rx_compressed = xeth_get_RX_COMPRESSED(src);
	dst->tx_compressed = xeth_get_TX_COMPRESSED(src);
	dst->rx_nohandler = xeth_get_RX_NOHANDLER(src);
}

enum {
	xeth_mux_max_flags = 8,
	xeth_mux_max_stats = 512,
};

extern struct platform_driver xeth_mux_driver;
extern struct rtnl_link_ops xeth_mux_lnko;
extern const struct net_device_ops xeth_mux_ndo;

static inline bool is_xeth_mux(struct net_device *nd)
{
	return nd->netdev_ops == &xeth_mux_ndo;
}

void xeth_mux_ifname(struct device *dev, char ifname[]);
enum xeth_encap xeth_mux_encap(struct net_device *mux);
u8 xeth_mux_base_port(struct net_device *mux);
u16 xeth_mux_ports(struct net_device *mux);

netdev_tx_t xeth_mux_encap_xmit(struct sk_buff *, struct net_device *proxy);

size_t xeth_mux_n_priv_flags(struct net_device *mux);
void xeth_mux_priv_flag_names(struct net_device *mux, char *buf);

size_t xeth_mux_n_stats(struct net_device *mux);
void xeth_mux_stat_names(struct net_device *mux, char *buf);

atomic64_t *xeth_mux_counters(struct net_device *mux);
volatile unsigned long *xeth_mux_flags(struct net_device *mux);

void xeth_mux_change_carrier(struct net_device *mux, struct net_device *nd,
			     bool on);
void xeth_mux_check_lower_carrier(struct net_device *mux);
void xeth_mux_del_vlans(struct net_device *mux, struct net_device *nd,
			struct list_head *unregq);
void xeth_mux_dump_all_ifinfo(struct net_device *);

const unsigned short * const xeth_mux_qsfp_i2c_addrs(struct net_device *mux);
struct gpio_desc *xeth_mux_qsfp_absent_gpio(struct net_device *mux, size_t prt);
struct gpio_desc *xeth_mux_qsfp_intr_gpio(struct net_device *mux, size_t prt);
struct gpio_desc *xeth_mux_qsfp_lpmode_gpio(struct net_device *mux, size_t prt);
struct gpio_desc *xeth_mux_qsfp_reset_gpio(struct net_device *mux, size_t prt);

int xeth_mux_qsfp_bus(struct net_device *mux, size_t port);

enum xeth_mux_counter {
	xeth_mux_counter_ex_frames,
	xeth_mux_counter_ex_bytes,
	xeth_mux_counter_sb_connections,
	xeth_mux_counter_sbex_invalid,
	xeth_mux_counter_sbex_dropped,
	xeth_mux_counter_sbrx_invalid,
	xeth_mux_counter_sbrx_no_dev,
	xeth_mux_counter_sbrx_no_mem,
	xeth_mux_counter_sbrx_msgs,
	xeth_mux_counter_sbrx_ticks,
	xeth_mux_counter_sbtx_msgs,
	xeth_mux_counter_sbtx_retries,
	xeth_mux_counter_sbtx_no_mem,
	xeth_mux_counter_sbtx_queued,
	xeth_mux_counter_sbtx_free,
	xeth_mux_counter_sbtx_ticks,
	xeth_mux_n_counters,
};

#define xeth_mux_counter_name(name)	[xeth_mux_counter_##name] = #name

#define xeth_mux_counter_names()					\
	xeth_mux_counter_name(ex_frames),				\
	xeth_mux_counter_name(ex_bytes),				\
	xeth_mux_counter_name(sb_connections),				\
	xeth_mux_counter_name(sbex_invalid),				\
	xeth_mux_counter_name(sbex_dropped),				\
	xeth_mux_counter_name(sbrx_invalid),				\
	xeth_mux_counter_name(sbrx_no_dev),				\
	xeth_mux_counter_name(sbrx_no_mem),				\
	xeth_mux_counter_name(sbrx_msgs),				\
	xeth_mux_counter_name(sbrx_ticks),				\
	xeth_mux_counter_name(sbtx_msgs),				\
	xeth_mux_counter_name(sbtx_retries),				\
	xeth_mux_counter_name(sbtx_no_mem),				\
	xeth_mux_counter_name(sbtx_queued),				\
	xeth_mux_counter_name(sbtx_free),				\
	xeth_mux_counter_name(sbtx_ticks),				\
	[xeth_mux_n_counters] = NULL

static inline void xeth_mux_counter_init(atomic64_t *t)
{
	enum xeth_mux_counter c;
	for (c = 0; c < xeth_mux_n_counters; c++, t++)
		atomic64_set(t, 0LL);
}

#define xeth_mux_counter_ops(name)					\
static inline long long							\
xeth_mux_get__##name(atomic64_t *t)					\
{									\
	return atomic64_read(&t[xeth_mux_counter_##name]);		\
}									\
static inline long long							\
xeth_mux_get_##name(struct net_device *mux)				\
{									\
	return xeth_mux_get__##name(xeth_mux_counters(mux));		\
}									\
static inline void							\
xeth_mux_add__##name(atomic64_t *t, s64 n)				\
{									\
	atomic64_add(n, &t[xeth_mux_counter_##name]);			\
}									\
static inline void							\
xeth_mux_add_##name(struct net_device *mux, s64 n)			\
{									\
	xeth_mux_add__##name(xeth_mux_counters(mux), n);		\
}									\
static inline void							\
xeth_mux_dec__##name(atomic64_t *t)					\
{									\
	atomic64_dec(&t[xeth_mux_counter_##name]);			\
}									\
static inline void							\
xeth_mux_dec_##name(struct net_device *mux)				\
{									\
	xeth_mux_dec__##name(xeth_mux_counters(mux));			\
}									\
static inline void							\
xeth_mux_inc__##name(atomic64_t *t)					\
{									\
	atomic64_inc(&t[xeth_mux_counter_##name]);			\
}									\
static inline void							\
xeth_mux_inc_##name(struct net_device *mux)				\
{									\
	xeth_mux_inc__##name(xeth_mux_counters(mux));			\
}									\
static inline void							\
xeth_mux_set__##name(atomic64_t *t, s64 n)				\
{									\
	atomic64_set(&t[xeth_mux_counter_##name], n);			\
}									\
static inline void							\
xeth_mux_set_##name(struct net_device *mux, s64 n)			\
{									\
	xeth_mux_set__##name(xeth_mux_counters(mux), n);		\
}

xeth_mux_counter_ops(ex_frames)
xeth_mux_counter_ops(ex_bytes)
xeth_mux_counter_ops(sb_connections)
xeth_mux_counter_ops(sbex_invalid)
xeth_mux_counter_ops(sbex_dropped)
xeth_mux_counter_ops(sbrx_invalid)
xeth_mux_counter_ops(sbrx_no_dev)
xeth_mux_counter_ops(sbrx_no_mem)
xeth_mux_counter_ops(sbrx_msgs)
xeth_mux_counter_ops(sbrx_ticks)
xeth_mux_counter_ops(sbtx_msgs)
xeth_mux_counter_ops(sbtx_retries)
xeth_mux_counter_ops(sbtx_no_mem)
xeth_mux_counter_ops(sbtx_queued)
xeth_mux_counter_ops(sbtx_free)
xeth_mux_counter_ops(sbtx_ticks)

enum xeth_mux_flag {
	xeth_mux_flag_main_task,
	xeth_mux_flag_sb_listen,
	xeth_mux_flag_sb_connection,
	xeth_mux_flag_sbrx_task,
	xeth_mux_flag_fib_notifier,
	xeth_mux_flag_inetaddr_notifier,
	xeth_mux_flag_inet6addr_notifier,
	xeth_mux_flag_netdevice_notifier,
	xeth_mux_flag_netevent_notifier,
	xeth_mux_n_flags,
};

#define xeth_mux_flag_name(name)	[xeth_mux_flag_##name] = #name

#define xeth_mux_flag_names()						\
	xeth_mux_flag_name(main_task),					\
	xeth_mux_flag_name(sb_listen),					\
	xeth_mux_flag_name(sb_connection),				\
	xeth_mux_flag_name(sbrx_task),					\
	xeth_mux_flag_name(inetaddr_notifier),				\
	xeth_mux_flag_name(inet6addr_notifier),				\
	xeth_mux_flag_name(netdevice_notifier),				\
	xeth_mux_flag_name(netevent_notifier),				\
	[xeth_mux_n_flags] = NULL,

#define xeth_mux_flag_ops(name)						\
static inline bool xeth_mux_has__##name(volatile unsigned long *flags)	\
{									\
	bool flag;							\
	smp_mb__before_atomic();					\
	flag = arch_test_bit(xeth_mux_flag_##name, flags);		\
	smp_mb__after_atomic();						\
	return flag;							\
}									\
static inline bool xeth_mux_has_##name(struct net_device *mux)		\
{									\
	return xeth_mux_has__##name(xeth_mux_flags(mux));		\
}									\
static inline void xeth_mux_clear__##name(volatile unsigned long *flags) \
{									\
	smp_mb__before_atomic();					\
	clear_bit(xeth_mux_flag_##name, flags);				\
	smp_mb__after_atomic();						\
}									\
static inline void xeth_mux_clear_##name(struct net_device *mux)	\
{									\
	xeth_mux_clear__##name(xeth_mux_flags(mux));			\
}									\
static inline void xeth_mux_set__##name(volatile unsigned long *flags)	\
{									\
	smp_mb__before_atomic();					\
	set_bit(xeth_mux_flag_##name, flags);				\
	smp_mb__after_atomic();						\
}									\
static inline void xeth_mux_set_##name(struct net_device *mux)		\
{									\
	xeth_mux_set__##name(xeth_mux_flags(mux));			\
}									\

xeth_mux_flag_ops(main_task)
xeth_mux_flag_ops(sb_listen)
xeth_mux_flag_ops(sb_connection)
xeth_mux_flag_ops(sbrx_task)
xeth_mux_flag_ops(fib_notifier)
xeth_mux_flag_ops(inetaddr_notifier)
xeth_mux_flag_ops(inet6addr_notifier)
xeth_mux_flag_ops(netdevice_notifier)
xeth_mux_flag_ops(netevent_notifier)

struct xeth_fibmuxnet {
	struct list_head list;
	struct notifier_block fib;
	struct net_device *mux;
	struct net *net;
};

static inline void xeth_mux_fib_cb(struct notifier_block *fib)
{
	struct xeth_fibmuxnet *fmn = container_of(fib, typeof(*fmn), fib);
	xeth_nd_debug(fmn->mux, "registered fib notifier");
}

static inline struct net *
xeth_fibmuxnet_net(struct xeth_fibmuxnet *fmn,
		   struct fib_notifier_info *info)
{
	return
#if defined(fib_notifier_info_with_net)
	info->net;
#else
	fmn->net;
#endif
}

static inline int xeth_fibmuxnet_register(struct xeth_fibmuxnet *fmn)
{
	return
#if defined(fib_notifier_info_with_net)
	register_fib_notifier(&fmn->fib, xeth_mux_fib_cb);
#else
	register_fib_notifier(fmn->net, &fmn->fib, xeth_mux_fib_cb, NULL);
#endif
}

static inline void xeth_fibmuxnet_unregister(struct xeth_fibmuxnet *fmn)
{
#if defined(fib_notifier_info_with_net)
	unregister_fib_notifier(&fmn->fib);
#else
	unregister_fib_notifier(fmn->net, &fmn->fib);
#endif
}

struct xeth_nb {
	struct list_head fibs;
	struct notifier_block inetaddr;
	struct notifier_block inet6addr;
	struct notifier_block netdevice;
	struct notifier_block netevent;
};

struct xeth_nb *xeth_mux_nb(struct net_device *mux);
struct net_device *xeth_mux_of_nb(struct xeth_nb *);

int xeth_nb_start_new_fib(struct net_device *mux, struct net *net);
int xeth_nb_start_all_fib(struct net_device *mux);
int xeth_nb_start_inetaddr(struct net_device *mux);
int xeth_nb_start_inet6addr(struct net_device *mux);
int xeth_nb_start_netdevice(struct net_device *mux);
int xeth_nb_start_netevent(struct net_device *mux);

void xeth_nb_stop_all_fib(struct net_device *mux);
void xeth_nb_stop_inetaddr(struct net_device *mux);
void xeth_nb_stop_inet6addr(struct net_device *mux);
void xeth_nb_stop_netdevice(struct net_device *mux);
void xeth_nb_stop_netevent(struct net_device *mux);

extern struct platform_driver xeth_port_driver;
extern struct rtnl_link_ops xeth_port_lnko;

extern const struct net_device_ops xeth_port_ndo;

static inline bool is_xeth_port(struct net_device *nd)
{
	return nd->netdev_ops == &xeth_port_ndo;
}

int xeth_port_of(struct net_device *nd);
int xeth_port_subport(struct net_device *nd);

u32 xeth_port_ethtool_priv_flags(struct net_device *nd);
const struct ethtool_link_ksettings *
	xeth_port_ethtool_ksettings(struct net_device *nd);
void xeth_port_ethtool_stat(struct net_device *nd, u32 index, u64 count);
void xeth_port_link_stat(struct net_device *nd, u32 index, u64 count);
void xeth_port_speed(struct net_device *nd, u32 mbps);

void xeth_port_reset_ethtool_stats(struct net_device *);

/**
 * struct xeth_proxy -	first member of each xeth proxy device priv
 *			{ port, vlan, bridge, lag }
 */
struct xeth_proxy {
	struct net_device *nd, *mux;
	/* @node: XID hash entry */
	struct hlist_node __rcu	node;
	/* @kin: other proxies of the same kind */
	struct list_head __rcu	kin;
	/* @quit: pending quit from lag or bridge */
	struct list_head quit;
	atomic64_t link_stats[XETH_N_LINK_STAT];
	enum xeth_dev_kind kind;
	u32 xid;
};

#define xeth_proxy_of_kin(ptr)						\
	container_of(ptr, struct xeth_proxy, kin)

#define xeth_proxy_of_quit(ptr)						\
	container_of(ptr, struct xeth_proxy, quit)

struct xeth_proxy *xeth_mux_proxy_of_xid(struct net_device *mux, u32 xid);
struct xeth_proxy *xeth_mux_proxy_of_nd(struct net_device *mux,
					struct net_device *nd);

void xeth_mux_add_proxy(struct xeth_proxy *);
void xeth_mux_del_proxy(struct xeth_proxy *);

void xeth_proxy_dump_ifa(struct xeth_proxy *);
void xeth_proxy_dump_ifa6(struct xeth_proxy *);
void xeth_proxy_dump_ifinfo(struct xeth_proxy *);

static inline void xeth_proxy_reset_link_stats(struct xeth_proxy *proxy)
{
	xeth_link_stat_init(proxy->link_stats);
}

static inline void xeth_proxy_setup(struct net_device *nd)
{
	struct xeth_proxy *proxy = netdev_priv(nd);
	INIT_LIST_HEAD(&proxy->kin);
	xeth_link_stat_init(proxy->link_stats);
}

int xeth_proxy_init(struct net_device *nd);
void xeth_proxy_uninit(struct net_device *nd);
int xeth_proxy_open(struct net_device *nd);
int xeth_proxy_stop(struct net_device *nd);
netdev_tx_t xeth_proxy_start_xmit(struct sk_buff *skb, struct net_device *nd);
int xeth_proxy_get_iflink(const struct net_device *nd);
int xeth_proxy_change_mtu(struct net_device *nd, int mtu);
void xeth_proxy_link_stat(struct net_device *nd, u32 index, u64 count);
void xeth_proxy_get_stats64(struct net_device *, struct rtnl_link_stats64 *);
netdev_features_t xeth_proxy_fix_features(struct net_device *,
					  netdev_features_t);
int xeth_proxy_set_features(struct net_device *, netdev_features_t);

int xeth_qsfp_get_module_info(struct i2c_client *qsfp,
			      struct ethtool_modinfo *emi);
int xeth_qsfp_get_module_eeprom(struct i2c_client *qsfp,
				struct ethtool_eeprom *ee, u8 *data);
/**
 * xeth_qsfp_client()
 * @nr: bus number
 * @addrs: a I2C_CLIENT_END terminated list
 */
struct i2c_client *xeth_qsfp_client(int nr, const unsigned short *addrs);

extern struct rtnl_link_ops xeth_vlan_lnko;

static inline int xeth_rtnl_unlock(int val)
{
	rtnl_unlock();
	return val;
}

int xeth_sbrx_msg(struct net_device *mux, void *v, size_t n);

struct xeth_sbtxb {
	struct list_head list;
	size_t len, sz;
};

enum {
	xeth_sbtxb_size	= ALIGN(sizeof(struct xeth_sbtxb), 32),
};

static inline void *xeth_sbtxb_data(const struct xeth_sbtxb *sbtxb)
{
	return (char *)sbtxb + xeth_sbtxb_size;
}

static inline void xeth_sbtxb_zero(const struct xeth_sbtxb *sbtxb)
{
	memset(xeth_sbtxb_data(sbtxb), 0, sbtxb->len);
}

struct xeth_sbtxb *xeth_mux_alloc_sbtxb(struct net_device *mux, size_t);
void xeth_mux_queue_sbtx(struct net_device *mux, struct xeth_sbtxb *);

int xeth_sbtx_break(struct net_device *);
int xeth_sbtx_change_upper(struct net_device *, u32 upper_xid, u32 lower_xid,
			   bool linking);
int xeth_sbtx_et_flags(struct net_device *, u32 xid, u32 flags);
int xeth_sbtx_et_settings(struct net_device *, u32 xid,
			  const struct ethtool_link_ksettings *);
int xeth_sbtx_fib_entry(struct net_device *, struct net *net,
			struct fib_entry_notifier_info *feni,
			unsigned long event);
int xeth_sbtx_fib6_entry(struct net_device *, struct net *net,
			 struct fib6_entry_notifier_info *feni,
			 unsigned long event);
int xeth_sbtx_ifa(struct net_device *, struct in_ifaddr *ifa,
		  unsigned long event, u32 xid);
int xeth_sbtx_ifa6(struct net_device *, struct inet6_ifaddr *ifa,
		   unsigned long event, u32 xid);
int xeth_sbtx_ifinfo(struct xeth_proxy *, unsigned iff,
		     enum xeth_msg_ifinfo_reason);
int xeth_sbtx_neigh_update(struct net_device *, struct neighbour *neigh);
int xeth_sbtx_netns(struct net_device *, u64 ns_inum, bool add);

#if !defined(XETH_VERSION)
#define XETH_VERSION "undefined"
#endif

extern const char xeth_version[];

extern struct rtnl_link_ops xeth_vlan_lnko;

static inline bool is_xeth_vlan(struct net_device *nd)
{
	return nd->rtnl_link_ops == &xeth_vlan_lnko;
}

bool xeth_vlan_has_link(const struct net_device *nd,
			const struct net_device *link);

#endif /* __NET_ETHERNET_XETH_H */
