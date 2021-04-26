// Copyright © 2018-2021 Platina Systems, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/platinasystems/xeth/v3/go/xeth"
)

var log = os.Stdout

func main() {
	var wg sync.WaitGroup
	stopch := make(chan struct{})
	sigch := make(chan os.Signal, 1)
	signal.Notify(sigch,
		syscall.SIGTERM,
		syscall.SIGINT,
		syscall.SIGHUP,
		syscall.SIGQUIT)
	defer signal.Stop(sigch)
	xidOfDst := make(map[string]xeth.Xid)

	if *flagLicense {
		log.WriteString(License[1:])
		return
	}
	if len(*flagLog) > 0 {
		f, err := os.Create(*flagLog)
		if err != nil {
			panic(err)
		}
		defer f.Close()
		log = f
	}

	task, err := xeth.Start(*flagMux, &wg, stopch)
	if err != nil {
		panic(err)
	}
	defer wg.Wait()

	task.DumpIfInfo()
selector:
	for {
		select {
		case <-sigch:
			close(stopch)
			break selector
		case <-task.Stop:
			break selector
		case buf, ok := <-task.RxCh:
			if !ok {
				close(stopch)
				if task.RxErr != nil {
					panic(task.RxErr)
				}
				break selector
			}
			msg := xeth.Parse(buf)
			switch t := msg.(type) {
			case xeth.Frame:
				xid, found := xidOfDst[t.Dst().String()]
				if found {
					t.Xid(xid)
					t.Loopback(task)
				}
			case xeth.Break:
				if *flagDumpFib {
					*flagDumpFib = false
					task.DumpFib()
				}
			case xeth.DevNew:
				xid := xeth.Xid(t)
				ha := xeth.LinkOf(xid).IfInfoHardwareAddr()
				xidOfDst[ha.String()] = xid
				verbose(msg)
			default:
				verbose(msg)
			}
			xeth.Pool(msg)
		}
	}
}
