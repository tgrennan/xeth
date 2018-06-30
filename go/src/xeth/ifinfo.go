/* Copyright(c) 2018 Platina Systems, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * sw@platina.com
 * Platina Systems, 3180 Del La Cruz Blvd, Santa Clara, CA 95054
 */

package xeth

import (
	"fmt"
	"net"
)

func (info *MsgIfinfo) HardwareAddr() net.HardwareAddr {
	return net.HardwareAddr(info.Addr[:])
}

func (info *MsgIfinfo) String() string {
	kind := Kind(info.Kind)
	ifname := (*Ifname)(&info.Ifname)
	iflink := InterfaceByIndex(info.Iflinkindex).Name
	iff := Iff(info.Flags)
	devtype := DevType(info.Devtype)
	return fmt.Sprint(kind, " ", ifname, "[", info.Ifindex, "]",
		"@", iflink,
		" <", iff, ">",
		" id=", info.Id,
		" addr=", info.HardwareAddr(),
		" port=", info.Portindex,
		" subport=", info.Subportindex,
		" devtype=", devtype,
		" netns=", Netns(info.Net),
		"\n")
}
