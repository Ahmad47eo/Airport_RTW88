/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * AirPort_RTW88 1.0.1 — Ventura IO80211 AWDL/P2P virtual-interface bridge.
 *
 * This file deliberately implements only payload ABIs present in the pinned
 * MacKernelSDK. Verified Ventura payload ABIs are handled explicitly. Unknown AWDL/P2P
 * selectors stay unsupported rather than pretending a radio operation completed.
 */
#include "AirportRTW88.hpp"
#include "AirportRTW88Interface.hpp"
#include <net/bpf.h>
#include <sys/kpi_mbuf.h>

#define super IO80211Controller

static const char *rtw88VifRoleName(UInt role)
{
    return role == APPLE80211_VIF_AWDL ? "awdl" : "p2p";
}

IO80211VirtualInterface *AirportRTW88::createVirtualInterface(ether_addr *addr, UInt role)
{
    IOLog("AirPort_RTW88: createVirtualInterface role=%u\n", role);

    if (role < APPLE80211_VIF_P2P_DEVICE || role > APPLE80211_VIF_AWDL)
        return super::createVirtualInterface(addr, role);

    /* IO80211's peer-to-peer roles (including AWDL/AirLink) must be backed by
     * IO80211P2PInterface, not a bare IO80211VirtualInterface.  A bare VIF is
     * sufficient to allocate/publish an ifnet named awdl0, but Ventura does
     * not classify it as a peer-to-peer virtual interface: the result is the
     * exact ghost state we observed (BSD awdl0 exists, MTU 0/inactive, while
     * Apple80211GetVirtualIfListCopy reports zero interfaces).
     *
     * Real IO80211 traces show AWDL role 4 running both
     * IO80211VirtualInterface::init and IO80211P2PInterface::init.  Construct
     * the derived class so its virtual init performs the P2P/AWDL setup. */
    IO80211P2PInterface *p2p = new IO80211P2PInterface;
    if (!p2p)
        return nullptr;

    if (!p2p->init(this, addr, role, rtw88VifRoleName(role))) {
        IOLog("AirPort_RTW88: P2P virtual interface init failed role=%u\n", role);
        p2p->release();
        return nullptr;
    }

    IO80211VirtualInterface *interface = p2p;
    IOLog("AirPort_RTW88: P2P virtual interface created role=%u name=%s class=%s\n",
          role, rtw88VifRoleName(role), interface->getMetaClass()->getClassName());
    return interface;
}

SInt32 AirportRTW88::enableVirtualInterface(IO80211VirtualInterface *interface)
{
    if (!interface)
        return kIOReturnBadArgument;

    UInt role = (UInt)interface->getInterfaceRole();
    IOLog("AirPort_RTW88: enableVirtualInterface role=%u bsd=%s\n",
          role, interface->getBSDName() ? interface->getBSDName() : "?");

    SInt32 ret = super::enableVirtualInterface(interface);
    if (ret != kIOReturnSuccess)
        return ret;

    if (_awdlManager)
        _awdlManager->setVirtualInterface(role, interface);
    if (role == APPLE80211_VIF_AWDL && _ieee80211)
        (void)_ieee80211->setAWDLReceiveMode(true);

#if __IO80211_TARGET >= __MAC_13_0
    interface->setEnabledBySystem(true);
#endif
    interface->setLinkState(kIO80211NetworkLinkUp, 0);
    interface->postMessage(APPLE80211_M_LINK_CHANGED);
    return kIOReturnSuccess;
}

SInt32 AirportRTW88::disableVirtualInterface(IO80211VirtualInterface *interface)
{
    if (!interface)
        return kIOReturnBadArgument;

    UInt role = (UInt)interface->getInterfaceRole();
    IOLog("AirPort_RTW88: disableVirtualInterface role=%u bsd=%s\n",
          role, interface->getBSDName() ? interface->getBSDName() : "?");

    /* Match AirportItlwm: let IO80211 tear the VIF down first, then publish
     * link-down only after the superclass accepted the transition. */
    SInt32 ret = super::disableVirtualInterface(interface);
    if (ret != kIOReturnSuccess)
        return ret;

    interface->setLinkState(kIO80211NetworkLinkDown, 0);
    interface->postMessage(APPLE80211_M_LINK_CHANGED);
    if (role == APPLE80211_VIF_AWDL && _ieee80211)
        (void)_ieee80211->setAWDLReceiveMode(false);
    if (_awdlManager)
        _awdlManager->clearVirtualInterface(interface);
    return kIOReturnSuccess;
}

SInt32 AirportRTW88::apple80211VirtualRequest(UInt request_type, int request_number,
                                               IO80211VirtualInterface *interface,
                                               void *data)
{
    /*
     * AWDL/P2P request payloads are not part of the pinned Ventura SDK ABI.
     * Keep the virtual-interface request path explicitly unsupported rather
     * than interpreting unknown payload layouts in kernel space. Normal STA
     * Wi-Fi requests continue through apple80211Request().
     */
    (void)request_type;
    (void)request_number;
    (void)interface;
    (void)data;
    return kIOReturnUnsupported;
}

static int rtw88SendActionFrame(RTW88IEEE80211 *backend, mbuf_t m)
{
    if (!m) return kIOReturnBadArgument;
    if (!backend) { mbuf_freem(m); return kIOReturnNotReady; }

    size_t len = mbuf_pkthdr_len(m);
    if (!len) len = mbuf_len(m);
    if (len < 24 || len > 4096) {
        mbuf_freem(m);
        return kIOReturnBadArgument;
    }

    uint8_t *frame = (uint8_t *)IOMalloc(len);
    if (!frame) { mbuf_freem(m); return kIOReturnNoMemory; }
    errno_t err = mbuf_copydata(m, 0, len, frame);
    mbuf_freem(m);
    if (err) { IOFree(frame, len); return kIOReturnError; }

    bool ok = backend->txRawManagementFrame(frame, (uint32_t)len);
    IOFree(frame, len);
    return ok ? kIOReturnSuccess : kIOReturnError;
}

int AirportRTW88::outputActionFrame(IO80211Interface *interface, mbuf_t m)
{
    (void)interface;
    IOLog("AirPort_RTW88: infrastructure action TX len=%zu\n", m ? mbuf_pkthdr_len(m) : 0);
    return rtw88SendActionFrame(_ieee80211, m);
}

int AirportRTW88::bpfOutputPacket(OSObject *object, UInt dltType, mbuf_t m)
{
    IO80211VirtualInterface *vif = OSDynamicCast(IO80211VirtualInterface, object);
    IOLog("AirPort_RTW88: VIF bpfOutput dlt=%u role=%d len=%zu\n",
          dltType, vif ? vif->getInterfaceRole() : 0, m ? mbuf_pkthdr_len(m) : 0);

    if (dltType == DLT_RAW || dltType == DLT_IEEE802_11)
        return rtw88SendActionFrame(_ieee80211, m);

    if (dltType == DLT_IEEE802_11_RADIO && m) {
        /* Minimal radiotap decapsulation: bytes 2..3 are the little-endian
         * radiotap header length.  AWDL action frames are then ordinary
         * native 802.11 management frames for the rtw88 MGMT queue. */
        uint8_t hdr[4] = {};
        if (mbuf_pkthdr_len(m) < sizeof(hdr) ||
            mbuf_copydata(m, 0, sizeof(hdr), hdr) != 0) {
            mbuf_freem(m);
            return kIOReturnBadArgument;
        }
        uint16_t rtlen = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
        size_t total = mbuf_pkthdr_len(m);
        if (rtlen < 4 || rtlen >= total) {
            mbuf_freem(m);
            return kIOReturnBadArgument;
        }
        mbuf_adj(m, (int)rtlen);
        return rtw88SendActionFrame(_ieee80211, m);
    }

    if (m) mbuf_freem(m);
    return kIOReturnUnsupported;
}

void AirportRTW88::requestPacketTx(void *object, UInt options)
{
    IO80211VirtualInterface *vif = OSDynamicCast(IO80211VirtualInterface, (OSObject *)object);
    if (!vif || !_ieee80211 || vif->getInterfaceRole() != APPLE80211_VIF_AWDL)
        return;

    /* Ventura's IO80211VirtualInterface has its own private output queues.
     * requestPacketTx is the controller's opportunity to drain them. The
     * private method returns bool with undocumented semantics, so packetHead
     * (initialized to null) is authoritative rather than the return value. */
    static const IOMbufServiceClass classes[] = {
        kIOMbufServiceClassCTL, kIOMbufServiceClassVO, kIOMbufServiceClassVI,
        kIOMbufServiceClassRV, kIOMbufServiceClassAV, kIOMbufServiceClassOAM,
        kIOMbufServiceClassRD, kIOMbufServiceClassBE, kIOMbufServiceClassBK,
        kIOMbufServiceClassBKSYS
    };

    UInt32 sent = 0;
    for (unsigned c = 0; c < sizeof(classes) / sizeof(classes[0]); c++) {
        /* Bound each callback so one chatty class cannot monopolize the
         * IO80211 workloop. IO80211 will signal us again if more remains. */
        mbuf_t head = nullptr, tail = nullptr;
        UInt count = 0;
        unsigned long long bytes = 0;
        (void)vif->dequeueOutputPacketsWithServiceClass(32, classes[c],
                                                        &head, &tail,
                                                        &count, &bytes);
        mbuf_t m = head;
        while (m) {
            mbuf_t next = mbuf_nextpkt(m);
            mbuf_setnextpkt(m, nullptr);
            if (_ieee80211->txAWDLDataFrame(m))
                sent++;
            /* txAWDLDataFrame consumes m on both success and failure. */
            m = next;
        }
    }

    if (sent || options)
        IOLog("AirPort_RTW88: requestPacketTx AWDL sent=%u options=%u\n", sent, options);
}

