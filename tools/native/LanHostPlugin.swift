import Foundation
import UIKit
import Capacitor
import Network
import CryptoKit

/// خادم LAN داخل تطبيق iOS: نفس بروتوكول إضافة الأندرويد (WebSocket + /api/info) ليدخل عليه PS4 وأندرويد وآيفون آخر.
@objc(LanHostPlugin)
public class LanHostPlugin: CAPPlugin, CAPBridgedPlugin {
    public let identifier = "LanHostPlugin"
    public let jsName = "LanHost"
    public let pluginMethods: [CAPPluginMethod] = [
        CAPPluginMethod(name: "start", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "stop", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "send", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "sendBatch", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "closeClient", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "setInfo", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "localIps", returnType: CAPPluginReturnPromise),
    ]

    private let q = DispatchQueue(label: "wz.lan")
    private var listener: NWListener?
    private var running = false
    private var port = 3000
    private var info = "{\"app\":\"warzone\",\"players\":0,\"phase\":\"lobby\"}"
    private var clients: [Int: WSClient] = [:]
    private var nextCid = 1
    private var timer: DispatchSourceTimer?
    private let GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    private let MAXP = 256 * 1024

    // ---------- أساليب الإضافة ----------
    @objc func start(_ call: CAPPluginCall) {
        let want = call.getInt("port") ?? 3000
        DispatchQueue.main.async { UIApplication.shared.isIdleTimerDisabled = true }
        q.async {
            if self.running { call.resolve(self.status()); return }
            self.tryPort(want, want + 10, call)
        }
    }

    private func tryPort(_ p: Int, _ end: Int, _ call: CAPPluginCall) {
        if p >= end { call.reject("تعذّر فتح المنفذ"); return }
        let params = NWParameters.tcp
        params.allowLocalEndpointReuse = true
        if let tcp = params.defaultProtocolStack.transportProtocol as? NWProtocolTCP.Options { tcp.noDelay = true; tcp.enableKeepalive = true }
        guard let np = NWEndpoint.Port(rawValue: UInt16(p)), let l = try? NWListener(using: params, on: np) else { tryPort(p + 1, end, call); return }
        var done = false
        l.newConnectionHandler = { [weak self] c in self?.accept(c) }
        l.stateUpdateHandler = { [weak self, weak l] st in
            guard let self = self, let l = l else { return }
            switch st {
            case .ready:
                if !done { done = true; self.listener = l; self.port = p; self.running = true; self.startTimer(); call.resolve(self.status()) }
            case .failed(_):
                l.cancel()
                if !done { done = true; self.tryPort(p + 1, end, call) }
                else if self.running && self.listener === l { self.running = false }
            default: break
            }
        }
        l.start(queue: q)
    }

    @objc func stop(_ call: CAPPluginCall) {
        q.async { self.shutdown(); call.resolve() }
    }

    @objc func send(_ call: CAPPluginCall) {
        let cid = call.getInt("cid") ?? -1
        let d = call.getString("data")
        q.async { if let c = self.clients[cid], let d = d { c.enqueue(self.frame(0x1, Array(d.utf8))) } }
        call.resolve()
    }

    @objc func sendBatch(_ call: CAPPluginCall) {
        var items: [(Int, String)] = []
        for it in call.getArray("items") ?? [] {
            if let o = it as? [String: Any], let c = (o["c"] as? NSNumber)?.intValue, let d = o["d"] as? String { items.append((c, d)) }
        }
        q.async { for (c, d) in items { if let cl = self.clients[c] { cl.enqueue(self.frame(0x1, Array(d.utf8))) } } }
        call.resolve()
    }

    @objc func closeClient(_ call: CAPPluginCall) {
        let cid = call.getInt("cid") ?? -1
        q.async { self.clients[cid]?.closeSoon() }
        call.resolve()
    }

    @objc func setInfo(_ call: CAPPluginCall) {
        if let s = call.getString("info") { q.async { self.info = s } }
        call.resolve()
    }

    @objc func localIps(_ call: CAPPluginCall) { q.async { call.resolve(self.status()) } }

    private func status() -> [String: Any] { return ["port": port, "ips": LanHostPlugin.localIpList()] }

    static func localIpList() -> [String] {
        var out: [String] = []
        var ifa: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&ifa) == 0, let first = ifa else { return out }
        var p: UnsafeMutablePointer<ifaddrs>? = first
        while let cur = p {
            let f = cur.pointee
            if let a = f.ifa_addr, a.pointee.sa_family == UInt8(AF_INET), (Int32(f.ifa_flags) & IFF_UP) != 0, (Int32(f.ifa_flags) & IFF_LOOPBACK) == 0 {
                var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
                if getnameinfo(a, socklen_t(a.pointee.sa_len), &host, socklen_t(host.count), nil, 0, NI_NUMERICHOST) == 0 {
                    let ip = String(cString: host)
                    let parts = ip.split(separator: ".").compactMap { Int($0) }
                    if parts.count == 4 && (parts[0] == 10 || (parts[0] == 192 && parts[1] == 168) || (parts[0] == 172 && parts[1] >= 16 && parts[1] <= 31)) { out.append(ip) }
                }
            }
            p = f.ifa_next
        }
        freeifaddrs(first)
        return out
    }

    // ---------- الخادم ----------
    private func shutdown() {
        running = false
        listener?.cancel(); listener = nil
        timer?.cancel(); timer = nil
        for c in Array(clients.values) { c.closeNow(false) }
        clients.removeAll()
        DispatchQueue.main.async { UIApplication.shared.isIdleTimerDisabled = false }
    }

    private func startTimer() {
        timer?.cancel()
        let t = DispatchSource.makeTimerSource(queue: q)
        t.schedule(deadline: .now() + 10, repeating: 10)
        t.setEventHandler { [weak self] in
            guard let self = self else { return }
            let now = Date().timeIntervalSince1970
            for c in Array(self.clients.values) {
                if c.upgraded {
                    if now - c.lastRx > 35 { c.closeNow(true) } else { c.enqueue(self.frame(0x9, [])) }
                }
            }
        }
        t.resume()
        timer = t
    }

    private func accept(_ conn: NWConnection) {
        let c = WSClient(self, conn)
        c.begin()
    }

    // ---------- WebSocket ----------
    fileprivate func frame(_ op: UInt8, _ payload: [UInt8]) -> Data {
        var h: [UInt8] = [0x80 | op]
        let n = payload.count
        if n < 126 { h.append(UInt8(n)) }
        else if n < 65536 { h.append(126); h.append(UInt8(n >> 8)); h.append(UInt8(n & 255)) }
        else { h.append(127); for i in stride(from: 7, through: 0, by: -1) { h.append(UInt8((UInt64(n) >> UInt64(8 * i)) & 255)) } }
        return Data(h + payload)
    }

    fileprivate func httpHead(_ code: Int, _ text: String, _ extra: String, _ len: Int) -> Data {
        return Data("HTTP/1.1 \(code) \(text)\r\n\(extra)Content-Length: \(len)\r\nConnection: close\r\n\r\n".utf8)
    }

    fileprivate func handleHttp(_ c: WSClient, _ method: String, _ rawPath: String) {
        var path = rawPath
        if let i = path.firstIndex(of: "?") { path = String(path[path.startIndex..<i]) }
        let cors = "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Methods: GET, OPTIONS\r\n"
        if method == "OPTIONS" { c.sendAndClose(httpHead(204, "No Content", cors, 0)); return }
        if path == "/api/info" {
            var body = info
            if let d = info.data(using: .utf8), var j = (try? JSONSerialization.jsonObject(with: d)) as? [String: Any] {
                j["ips"] = LanHostPlugin.localIpList(); j["port"] = port
                if let nd = try? JSONSerialization.data(withJSONObject: j), let s = String(data: nd, encoding: .utf8) { body = s }
            }
            let b = Data(body.utf8)
            var out = httpHead(200, "OK", cors + "Content-Type: application/json; charset=utf-8\r\nCache-Control: no-store\r\n", b.count)
            out.append(b)
            c.sendAndClose(out); return
        }
        let b = Data("Not Found".utf8)
        var out = httpHead(404, "Not Found", "Content-Type: text/plain\r\n", b.count); out.append(b)
        c.sendAndClose(out)
    }

    fileprivate func register(_ c: WSClient) -> Int {
        let cid = nextCid; nextCid += 1
        clients[cid] = c
        return cid
    }
    fileprivate func unregister(_ cid: Int, _ notify: Bool) {
        if clients.removeValue(forKey: cid) != nil && notify { notifyListeners("close", data: ["cid": cid]) }
    }
    fileprivate func emitOpen(_ cid: Int) { notifyListeners("open", data: ["cid": cid]) }
    fileprivate func emitMessage(_ cid: Int, _ s: String) { notifyListeners("message", data: ["cid": cid, "data": s]) }
    fileprivate func isRunning() -> Bool { return running }
    fileprivate func clientCount() -> Int { return clients.count }

    fileprivate func acceptKey(_ key: String) -> String {
        let h = Insecure.SHA1.hash(data: Data((key + GUID).utf8))
        return Data(h).base64EncodedString()
    }

    fileprivate final class WSClient {
        unowned let owner: LanHostPlugin
        let conn: NWConnection
        var buf: [UInt8] = []
        var msg: [UInt8] = []
        var cid = 0
        var upgraded = false
        var closed = false
        var pending = 0
        var lastRx = Date().timeIntervalSince1970

        init(_ o: LanHostPlugin, _ c: NWConnection) { owner = o; conn = c }

        func begin() {
            conn.stateUpdateHandler = { [weak self] st in
                guard let self = self else { return }
                switch st {
                case .failed(_), .cancelled: self.closeNow(true)
                default: break
                }
            }
            conn.start(queue: owner.q)
            recv()
        }

        func recv() {
            conn.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] data, _, complete, err in
                guard let self = self, !self.closed else { return }
                if let d = data, !d.isEmpty { self.buf.append(contentsOf: d); self.lastRx = Date().timeIntervalSince1970; self.process() }
                if err != nil || complete { self.closeNow(true); return }
                if !self.closed { self.recv() }
            }
        }

        func process() {
            if !upgraded {
                guard let end = findHeaderEnd() else { if buf.count > 8192 { closeNow(false) }; return }
                let head = String(decoding: buf[0..<end], as: UTF8.self)
                buf.removeFirst(end + 4)
                let lines = head.components(separatedBy: "\r\n")
                let rl = (lines.first ?? "").components(separatedBy: " ")
                let method = rl.count > 0 ? rl[0] : "GET"
                let path = rl.count > 1 ? rl[1] : "/"
                var key: String? = nil; var up = false
                for l in lines.dropFirst() {
                    guard let ci = l.firstIndex(of: ":") else { continue }
                    let k = l[l.startIndex..<ci].trimmingCharacters(in: .whitespaces).lowercased()
                    let v = l[l.index(after: ci)...].trimmingCharacters(in: .whitespaces)
                    if k == "sec-websocket-key" { key = v } else if k == "upgrade" && v.lowercased() == "websocket" { up = true }
                }
                if up, let key = key {
                    if owner.clientCount() >= 24 { closeNow(false); return }
                    let resp = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: \(owner.acceptKey(key))\r\n\r\n"
                    upgraded = true
                    cid = owner.register(self)
                    rawSend(Data(resp.utf8))
                    owner.emitOpen(cid)
                } else {
                    owner.handleHttp(self, method, path)
                    return
                }
            }
            parseFrames()
        }

        func findHeaderEnd() -> Int? {
            if buf.count < 4 { return nil }
            for i in 0...(buf.count - 4) where buf[i] == 13 && buf[i + 1] == 10 && buf[i + 2] == 13 && buf[i + 3] == 10 { return i }
            return nil
        }

        func parseFrames() {
            while !closed {
                if buf.count < 2 { return }
                let b0 = buf[0], b1 = buf[1]
                let fin = (b0 & 0x80) != 0, op = b0 & 0x0f, masked = (b1 & 0x80) != 0
                var len = Int(b1 & 0x7f), off = 2
                if len == 126 { if buf.count < 4 { return }; len = Int(buf[2]) << 8 | Int(buf[3]); off = 4 }
                else if len == 127 {
                    if buf.count < 10 { return }
                    var l: UInt64 = 0; for i in 2..<10 { l = (l << 8) | UInt64(buf[i]) }
                    if l > UInt64(owner.MAXP) { closeNow(true); return }
                    len = Int(l); off = 10
                }
                if len > owner.MAXP || !masked { closeNow(true); return }
                if buf.count < off + 4 + len { return }
                let mk = Array(buf[off..<off + 4])
                var data = Array(buf[(off + 4)..<(off + 4 + len)])
                for i in 0..<data.count { data[i] ^= mk[i & 3] }
                buf.removeFirst(off + 4 + len)
                if op == 0x8 { enqueue(owner.frame(0x8, [])); closeSoon(); return }
                if op == 0x9 { enqueue(owner.frame(0xA, data)); continue }
                if op == 0xA { continue }
                if op == 0x1 || op == 0x0 {
                    msg.append(contentsOf: data)
                    if msg.count > owner.MAXP { closeNow(true); return }
                    if fin {
                        let s = String(decoding: msg, as: UTF8.self); msg.removeAll(keepingCapacity: true)
                        owner.emitMessage(cid, s)
                    }
                }
            }
        }

        func rawSend(_ d: Data) {
            pending += 1
            conn.send(content: d, completion: .contentProcessed({ [weak self] e in
                guard let self = self else { return }
                self.pending -= 1
                if e != nil { self.closeNow(true) }
            }))
        }

        func enqueue(_ d: Data) {
            if closed { return }
            if pending > 1500 { closeNow(true); return } // عميل بطيء جدًا
            rawSend(d)
        }

        func sendAndClose(_ d: Data) {
            conn.send(content: d, completion: .contentProcessed({ [weak self] _ in self?.closeNow(false) }))
        }

        func closeSoon() {
            if closed { return }
            conn.send(content: nil, contentContext: .finalMessage, isComplete: true, completion: .contentProcessed({ [weak self] _ in self?.closeNow(true) }))
        }

        func closeNow(_ notify: Bool) {
            if closed { return }
            closed = true
            conn.cancel()
            if upgraded { owner.unregister(cid, notify) }
        }
    }
}

/// واجهة التطبيق: تسجّل إضافة LanHost وتُبقي الشاشة مضاءة.
class WZBridgeViewController: CAPBridgeViewController {
    override open func capacitorDidLoad() {
        bridge?.registerPluginInstance(LanHostPlugin())
    }
}
