package com.warzone.lan;

import android.util.Base64;

import com.getcapacitor.JSArray;
import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * خادم LAN داخل التطبيق: يستقبل اتصالات WebSocket من اللاعبين الآخرين ويمرّر الرسائل إلى منطق اللعبة (JavaScript).
 * كما يخدم ملفات اللعبة عبر HTTP (ليتمكن آيفون/متصفح من الانضمام) ويجيب على /api/info لبحث الشبكة.
 */
@CapacitorPlugin(name = "LanHost")
public class LanHostPlugin extends Plugin {
    private static final String GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    private static final int MAX_PAYLOAD = 256 * 1024;

    private volatile ServerSocket server;
    private volatile boolean running = false;
    private volatile String info = "{\"app\":\"warzone\",\"players\":0,\"phase\":\"lobby\"}";
    private final Map<Integer, Client> clients = new ConcurrentHashMap<>();
    private final AtomicInteger nextCid = new AtomicInteger(1);
    private ExecutorService pool;
    private ScheduledExecutorService sched;
    private int port = 3000;

    // ---------- أساليب الإضافة ----------
    @PluginMethod
    public void start(PluginCall call) {
        try {
            if (running) { call.resolve(status()); return; }
            int want = call.getInt("port", 3000);
            ServerSocket ss = null;
            IOException last = null;
            for (int p = want; p < want + 10; p++) {
                try {
                    ss = new ServerSocket();
                    ss.setReuseAddress(true);
                    ss.bind(new InetSocketAddress(p));
                    port = p;
                    break;
                } catch (IOException e) { last = e; try { ss.close(); } catch (Exception ignore) { } ss = null; }
            }
            if (ss == null) { call.reject("تعذّر فتح المنفذ: " + (last != null ? last.getMessage() : "")); return; }
            server = ss;
            running = true;
            pool = Executors.newCachedThreadPool();
            sched = Executors.newSingleThreadScheduledExecutor();
            sched.scheduleWithFixedDelay(this::heartbeat, 10, 10, TimeUnit.SECONDS);
            final ServerSocket fs = ss;
            Thread t = new Thread(() -> acceptLoop(fs), "lan-accept");
            t.setDaemon(true);
            t.start();
            call.resolve(status());
        } catch (Exception e) {
            call.reject("فشل بدء الاستضافة: " + e.getMessage());
        }
    }

    @PluginMethod
    public void stop(PluginCall call) {
        shutdown();
        call.resolve();
    }

    @PluginMethod
    public void send(PluginCall call) {
        Client c = clients.get(call.getInt("cid", -1));
        String d = call.getString("data");
        if (c != null && d != null) c.enqueue(textFrame(d));
        call.resolve();
    }

    @PluginMethod
    public void sendBatch(PluginCall call) {
        try {
            JSArray items = call.getArray("items");
            for (int i = 0; items != null && i < items.length(); i++) {
                org.json.JSONObject o = items.getJSONObject(i);
                Client c = clients.get(o.getInt("c"));
                if (c != null) c.enqueue(textFrame(o.getString("d")));
            }
        } catch (Exception ignore) { }
        call.resolve();
    }

    @PluginMethod
    public void closeClient(PluginCall call) {
        Client c = clients.get(call.getInt("cid", -1));
        if (c != null) c.closeSoon();
        call.resolve();
    }

    @PluginMethod
    public void setInfo(PluginCall call) {
        String s = call.getString("info");
        if (s != null) info = s;
        call.resolve();
    }

    @PluginMethod
    public void localIps(PluginCall call) { call.resolve(status()); }

    @Override
    protected void handleOnDestroy() { shutdown(); }

    private JSObject status() {
        JSObject o = new JSObject();
        o.put("port", port);
        JSArray ips = new JSArray();
        for (String ip : localIps()) ips.put(ip);
        o.put("ips", ips);
        return o;
    }

    static List<String> localIps() {
        List<String> out = new ArrayList<>();
        try {
            Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces();
            for (NetworkInterface ni : Collections.list(en)) {
                if (!ni.isUp() || ni.isLoopback()) continue;
                for (InetAddress a : Collections.list(ni.getInetAddresses())) {
                    if (a instanceof Inet4Address && !a.isLoopbackAddress() && a.isSiteLocalAddress()) out.add(a.getHostAddress());
                }
            }
        } catch (Exception ignore) { }
        return out;
    }

    // ---------- الخادم ----------
    private void shutdown() {
        running = false;
        try { if (server != null) server.close(); } catch (Exception ignore) { }
        server = null;
        for (Client c : new ArrayList<>(clients.values())) c.closeNow(false);
        clients.clear();
        if (sched != null) { sched.shutdownNow(); sched = null; }
        if (pool != null) { pool.shutdownNow(); pool = null; }
    }

    private void acceptLoop(ServerSocket ss) {
        while (running) {
            try {
                Socket s = ss.accept();
                s.setTcpNoDelay(true);
                s.setKeepAlive(true);
                ExecutorService p = pool;
                if (p == null) { s.close(); continue; }
                p.execute(() -> handle(s));
            } catch (IOException e) {
                if (!running) return;
            } catch (Exception ignore) { }
        }
    }

    private void heartbeat() {
        long now = System.currentTimeMillis();
        for (Client c : new ArrayList<>(clients.values())) {
            if (now - c.lastRx > 35000) c.closeNow(true);
            else c.enqueue(frame(0x9, new byte[0]));
        }
    }

    private void handle(Socket s) {
        try {
            s.setSoTimeout(15000);
            InputStream in = s.getInputStream();
            OutputStream out = s.getOutputStream();
            // قراءة ترويسة HTTP
            ByteArrayOutputStream hb = new ByteArrayOutputStream();
            int state = 0;
            while (hb.size() < 8192) {
                int b = in.read();
                if (b < 0) { s.close(); return; }
                hb.write(b);
                if (b == '\r' && (state == 0 || state == 2)) state++;
                else if (b == '\n' && (state == 1 || state == 3)) state++;
                else state = 0;
                if (state == 4) break;
            }
            if (state != 4) { s.close(); return; }
            String head = hb.toString("UTF-8");
            String[] lines = head.split("\r\n");
            String[] rl = lines[0].split(" ");
            String method = rl.length > 0 ? rl[0] : "GET";
            String path = rl.length > 1 ? rl[1] : "/";
            String key = null;
            boolean upgrade = false;
            for (int i = 1; i < lines.length; i++) {
                int c = lines[i].indexOf(':');
                if (c < 0) continue;
                String k = lines[i].substring(0, c).trim().toLowerCase();
                String v = lines[i].substring(c + 1).trim();
                if (k.equals("sec-websocket-key")) key = v;
                else if (k.equals("upgrade") && v.equalsIgnoreCase("websocket")) upgrade = true;
            }
            if (upgrade && key != null) {
                if (clients.size() >= 24) { s.close(); return; }
                MessageDigest md = MessageDigest.getInstance("SHA-1");
                String acc = Base64.encodeToString(md.digest((key + GUID).getBytes(StandardCharsets.US_ASCII)), Base64.NO_WRAP);
                String resp = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + acc + "\r\n\r\n";
                out.write(resp.getBytes(StandardCharsets.US_ASCII));
                out.flush();
                s.setSoTimeout(0);
                wsLoop(s, in, out);
            } else {
                httpResponse(method, path, out);
                s.close();
            }
        } catch (Exception e) {
            try { s.close(); } catch (Exception ignore) { }
        }
    }

    // ---------- HTTP ----------
    private void httpResponse(String method, String rawPath, OutputStream out) throws IOException {
        String path = rawPath;
        int q = path.indexOf('?'); if (q >= 0) path = path.substring(0, q);
        String cors = "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Methods: GET, OPTIONS\r\n";
        if (method.equals("OPTIONS")) {
            out.write(("HTTP/1.1 204 No Content\r\n" + cors + "Content-Length: 0\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.US_ASCII));
            out.flush();
            return;
        }
        if (path.equals("/api/info")) {
            String body;
            try {
                org.json.JSONObject j = new org.json.JSONObject(info);
                j.put("ips", new org.json.JSONArray(localIps()));
                j.put("port", port);
                body = j.toString();
            } catch (Exception e) { body = info; }
            byte[] b = body.getBytes(StandardCharsets.UTF_8);
            out.write(("HTTP/1.1 200 OK\r\n" + cors + "Content-Type: application/json; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: " + b.length + "\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.US_ASCII));
            out.write(b); out.flush();
            return;
        }
        simple(out, 404, "Not Found"); // لا نقدّم صفحة ويب: اللعبة داخل التطبيق فقط
    }

    private static void simple(OutputStream out, int code, String text) throws IOException {
        byte[] b = text.getBytes(StandardCharsets.UTF_8);
        out.write(("HTTP/1.1 " + code + " " + text + "\r\nContent-Type: text/plain\r\nContent-Length: " + b.length + "\r\nConnection: close\r\n\r\n").getBytes(StandardCharsets.US_ASCII));
        out.write(b); out.flush();
    }

    private static String mime(String p) {
        p = p.toLowerCase();
        if (p.endsWith(".html")) return "text/html; charset=utf-8";
        if (p.endsWith(".js") || p.endsWith(".mjs")) return "text/javascript; charset=utf-8";
        if (p.endsWith(".css")) return "text/css; charset=utf-8";
        if (p.endsWith(".json") || p.endsWith(".webmanifest")) return "application/json; charset=utf-8";
        if (p.endsWith(".png")) return "image/png";
        if (p.endsWith(".svg")) return "image/svg+xml";
        if (p.endsWith(".ico")) return "image/x-icon";
        return "application/octet-stream";
    }

    // ---------- WebSocket ----------
    static byte[] frame(int opcode, byte[] payload) {
        int n = payload.length;
        ByteArrayOutputStream bo = new ByteArrayOutputStream(n + 10);
        bo.write(0x80 | opcode);
        if (n < 126) bo.write(n);
        else if (n < 65536) { bo.write(126); bo.write(n >> 8); bo.write(n & 255); }
        else { bo.write(127); for (int i = 7; i >= 0; i--) bo.write((int) (((long) n >> (8 * i)) & 255)); }
        bo.write(payload, 0, n);
        return bo.toByteArray();
    }
    static byte[] textFrame(String s) { return frame(0x1, s.getBytes(StandardCharsets.UTF_8)); }

    private void wsLoop(Socket s, InputStream rawIn, OutputStream out) {
        int cid = nextCid.getAndIncrement();
        Client c = new Client(cid, s, out);
        clients.put(cid, c);
        JSObject o = new JSObject(); o.put("cid", cid);
        notifyListeners("open", o);
        try {
            DataInputStream in = new DataInputStream(rawIn);
            ByteArrayOutputStream msg = new ByteArrayOutputStream();
            while (running && !c.closed) {
                int b0 = in.read();
                if (b0 < 0) break;
                int b1 = in.read();
                if (b1 < 0) break;
                boolean fin = (b0 & 0x80) != 0;
                int op = b0 & 0x0f;
                boolean mask = (b1 & 0x80) != 0;
                long len = b1 & 0x7f;
                if (len == 126) len = in.readUnsignedShort();
                else if (len == 127) len = in.readLong();
                if (len < 0 || len > MAX_PAYLOAD || !mask) break; // العميل يجب أن يقنّع
                byte[] mk = new byte[4];
                in.readFully(mk);
                byte[] data = new byte[(int) len];
                in.readFully(data);
                for (int i = 0; i < data.length; i++) data[i] ^= mk[i & 3];
                c.lastRx = System.currentTimeMillis();
                if (op == 0x8) { c.enqueue(frame(0x8, new byte[0])); break; }
                if (op == 0x9) { c.enqueue(frame(0xA, data)); continue; }
                if (op == 0xA) continue;
                if (op == 0x1 || op == 0x0) {
                    msg.write(data);
                    if (msg.size() > MAX_PAYLOAD) break;
                    if (fin) {
                        JSObject m = new JSObject();
                        m.put("cid", cid);
                        m.put("data", new String(msg.toByteArray(), StandardCharsets.UTF_8));
                        msg.reset();
                        notifyListeners("message", m);
                    }
                }
            }
        } catch (Exception ignore) {
        } finally {
            c.closeNow(true);
        }
    }

    private class Client {
        final int cid;
        final Socket sock;
        final OutputStream out;
        final LinkedBlockingQueue<byte[]> q = new LinkedBlockingQueue<>();
        volatile boolean closed = false;
        volatile long lastRx = System.currentTimeMillis();
        private final Thread writer;

        Client(int cid, Socket sock, OutputStream out) {
            this.cid = cid; this.sock = sock; this.out = out;
            writer = new Thread(this::writeLoop, "lan-write-" + cid);
            writer.setDaemon(true);
            writer.start();
        }

        void enqueue(byte[] f) {
            if (closed) return;
            if (q.size() > 1500) { closeNow(true); return; } // عميل بطيء جدًا
            q.offer(f);
        }

        private void writeLoop() {
            try {
                while (!closed) {
                    byte[] f = q.poll(1, TimeUnit.SECONDS);
                    if (f == null) continue;
                    if (f == POISON) break;
                    out.write(f);
                    if (q.isEmpty()) out.flush();
                }
            } catch (Exception ignore) {
            } finally {
                closeNow(true);
            }
        }

        void closeSoon() {
            q.offer(frame(0x8, new byte[0]));
            q.offer(POISON);
        }

        void closeNow(boolean notify) {
            if (closed) return;
            closed = true;
            q.offer(POISON);
            try { sock.close(); } catch (Exception ignore) { }
            if (clients.remove(cid) != null && notify) {
                JSObject o = new JSObject(); o.put("cid", cid);
                notifyListeners("close", o);
            }
        }
    }

    private static final byte[] POISON = new byte[0];
}
