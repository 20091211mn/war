import UIKit
import CoreGraphics

/// شاشة اللعبة: تعرض صورة المحرك كل إطار وتمرّر اللمس.
final class GameView: UIView {
    private var link: CADisplayLink?
    private var last: CFTimeInterval = 0
    private var started = false
    private var slots = [UITouch?](repeating: nil, count: 12)
    private let space = CGColorSpaceCreateDeviceRGB()

    override init(frame: CGRect) {
        super.init(frame: frame)
        isMultipleTouchEnabled = true
        isOpaque = true
        backgroundColor = .black
        layer.magnificationFilter = .linear
        layer.contentsGravity = .resize
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) not supported") }

    override func layoutSubviews() {
        super.layoutSubviews()
        let w = Int32(bounds.width), h = Int32(bounds.height)
        guard w > 0, h > 0 else { return }
        if !started {
            started = true
            if let d = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first { wz_set_config_path(d.appendingPathComponent("warzone.cfg").path) }
            wz_init(w, h, Int32(ProcessInfo.processInfo.activeProcessorCount))
            AudioOut.shared.start()
        } else {
            wz_set_screen(w, h)
        }
    }
    override func didMoveToWindow() {
        super.didMoveToWindow()
        if window != nil, link == nil {
            let l = CADisplayLink(target: self, selector: #selector(tick(_:)))
            l.preferredFramesPerSecond = 60
            l.add(to: .main, forMode: .common)
            link = l
        } else if window == nil { link?.invalidate(); link = nil }
    }

    @objc private func tick(_ l: CADisplayLink) {
        guard started else { return }
        let now = l.timestamp
        let dt = last > 0 ? Float(now - last) : Float(1.0 / 60.0)
        last = now
        wz_frame(dt)
        let w = Int(wz_width()), h = Int(wz_height())
        guard w > 0, h > 0, let p = wz_pixels() else { return }
        let data = Data(bytes: p, count: w * h * 4) // نسخة، حتى لا يكتب المحرك فوق ما يعرضه النظام
        guard let provider = CGDataProvider(data: data as CFData),
              let img = CGImage(width: w, height: h, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: w * 4, space: space,
                                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.noneSkipLast.rawValue), provider: provider,
                                decode: nil, shouldInterpolate: true, intent: .defaultIntent) else { return }
        layer.contents = img
    }

    private func slot(_ t: UITouch, create: Bool) -> Int {
        if let i = slots.firstIndex(where: { $0 === t }) { return i }
        guard create, let i = slots.firstIndex(where: { $0 == nil }) else { return -1 }
        slots[i] = t
        return i
    }
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches { let i = slot(t, create: true); if i >= 0 { let p = t.location(in: self); wz_touch(0, Int32(i), Float(p.x), Float(p.y)) } }
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches { let i = slot(t, create: false); if i >= 0 { let p = t.location(in: self); wz_touch(1, Int32(i), Float(p.x), Float(p.y)) } }
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches { let i = slot(t, create: false); if i >= 0 { wz_touch(2, Int32(i), 0, 0); slots[i] = nil } }
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        wz_touch(3, 0, 0, 0); for i in 0..<slots.count { slots[i] = nil }
    }
}
