package com.warzone.core

import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.InetSocketAddress
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.ConcurrentLinkedQueue

/** بروتوكول LAN (UDP) نفسه المستخدم في PS4 وآيفون (NET_VER=3) — فيدخل أي إصدار على أي إصدار. */
object NetC {
    const val PORT_GAME = 9330; const val PORT_BEACON = 9331; const val NET_VER = 3
    const val T_HELLO = 1; const val T_WELCOME = 2; const val T_INPUT = 3; const val T_SNAP = 4; const val T_BYE = 5; const val T_BEACON = 6; const val T_PROBE = 7
    const val SZ_INPUT = 45; const val SZ_ENT = 44; const val SZ_VEH = 24; const val SZ_C4 = 14; const val SZ_EVT = 4; const val SZ_TR = 24; const val SZ_HEAD = 16; const val SZ_BEACON = 20
    fun buf(n: Int): ByteBuffer = ByteBuffer.allocate(n).order(ByteOrder.LITTLE_ENDIAN)
    fun wrap(d: ByteArray): ByteBuffer = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN)
    fun u8(b: ByteBuffer): Int = b.get().toInt() and 255
    fun u16(b: ByteBuffer): Int = b.short.toInt() and 0xFFFF
    /** عنوان الجهاز المحلي (بدون إرسال فعلي) */
    fun localIp(): ByteArray? = try { DatagramSocket().use { it.connect(InetAddress.getByName("192.168.1.1"), 9); it.localAddress.address } } catch (e: Exception) { null }
}

class Pkt(@JvmField val d: ByteArray, @JvmField val n: Int, @JvmField val addr: InetAddress, @JvmField val port: Int)

/** مقبس UDP بخيط استقبال مستقل؛ الإرسال والاستقبال لا يحجبان خيط اللعبة. */
class Udp {
    @Volatile private var sock: DatagramSocket? = null
    @JvmField val q = ConcurrentLinkedQueue<Pkt>()
    @Volatile @JvmField var state = 0   // 0 لم يبدأ ، 1 جاهز ، -1 خطأ
    @Volatile @JvmField var err = ""
    fun open(port: Int, bcast: Boolean) {
        close(); state = 0; err = ""; q.clear()
        Thread({
            try {
                val s = DatagramSocket(null); s.reuseAddress = true; s.broadcast = bcast; s.bind(InetSocketAddress(port)); sock = s; state = 1
                val buf = ByteArray(1500)
                while (!s.isClosed) { val p = DatagramPacket(buf, buf.size); s.receive(p); if (q.size < 256) q.add(Pkt(p.data.copyOf(p.length), p.length, p.address, p.port)) }
            } catch (e: Exception) { if (sock?.isClosed != true) { state = -1; err = e.toString() } }
        }, "wz-udp").apply { isDaemon = true; start() }
    }
    fun ready() = state == 1 && sock != null
    fun recv(): Pkt? = q.poll()
    fun send(a: InetAddress, port: Int, d: ByteArray, n: Int = d.size) { val s = sock ?: return; try { s.send(DatagramPacket(d, n, a, port)) } catch (_: Exception) {} }
    fun close() { try { sock?.close() } catch (_: Exception) {}; sock = null; state = 0 }
}

class NetIn {
    @JvmField var valid = false; @JvmField var seq = 0
    @JvmField var x = 0f; @JvmField var y = 0f; @JvmField var z = 0f; @JvmField var yaw = 0f; @JvmField var pitch = 0f
    @JvmField var flags = 0; @JvmField var wpn = 0; @JvmField var vx = 0f; @JvmField var vz = 0f; @JvmField var vh = 0f; @JvmField var vsp = 0f
    @JvmField var veh = 0; @JvmField var c4p = 0; @JvmField var c4d = 0
}
class NetEvt(@JvmField val id: Int, @JvmField val killer: Int, @JvmField val victim: Int, @JvmField val head: Int)
class NetTr(@JvmField val x0: Float, @JvmField val y0: Float, @JvmField val z0: Float, @JvmField val x1: Float, @JvmField val y1: Float, @JvmField val z1: Float)
class Cli { @JvmField var on = false; @JvmField var addr: InetAddress? = null; @JvmField var port = 0; @JvmField var ent = -1; @JvmField var last = 0f }
class HostEnt(@JvmField val addr: InetAddress, @JvmField var map: Int, @JvmField var players: Int, @JvmField val maxp: Int, @JvmField val name: String) { @JvmField var seen = 0f }

class NetState {
    @JvmField val sock = Udp(); @JvmField val beaconRx = Udp(); @JvmField val beaconTx = Udp()
    @JvmField var hostAddr: InetAddress? = null
    @JvmField var joining = false; @JvmField var joinT = 0f; @JvmField var retryT = 0f; @JvmField var srvSilence = 0f; @JvmField var inT = 0f; @JvmField var snapT = 0f; @JvmField var beaconT = 0f
    @JvmField var netSeq = 0; @JvmField var matchId = 0; @JvmField var evtId = 0; @JvmField var lastEvt = 0; @JvmField var vConf = 0
    @JvmField var c4pCnt = 0; @JvmField var c4dCnt = 0
    @JvmField val evq = ArrayList<NetEvt>(); @JvmField val trOut = ArrayList<NetTr>()
    @JvmField val cli = Array(8) { Cli() }; @JvmField val hosts = ArrayList<HostEnt>(); @JvmField var hostSel = 0
    @JvmField var myIp: ByteArray? = null; @JvmField var sweepIdx = 0; @JvmField var ipT = 0f
    @JvmField val lastIn = Input()
    @JvmField var status = ""
}
