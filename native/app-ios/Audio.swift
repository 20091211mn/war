import AudioToolbox
import AVFoundation

private let renderCallback: AURenderCallback = { _, _, _, _, frames, ioData in
    guard let io = ioData else { return noErr }
    let buf = UnsafeMutableAudioBufferListPointer(io)[0]
    if let p = buf.mData { wz_mix(p.assumingMemoryBound(to: Int16.self), Int32(frames)) }
    return noErr
}

/// صوت اللعبة: مولّد في المحرك (48kHz ستيريو 16 بت) ويُشغَّل عبر AudioUnit.
final class AudioOut {
    static let shared = AudioOut()
    private var unit: AudioUnit?
    private var running = false

    func start() {
        if running { return }
        if unit == nil {
            let session = AVAudioSession.sharedInstance()
            try? session.setCategory(.playback)
            try? session.setActive(true)
            var desc = AudioComponentDescription(componentType: kAudioUnitType_Output, componentSubType: kAudioUnitSubType_RemoteIO,
                                                 componentManufacturer: kAudioUnitManufacturer_Apple, componentFlags: 0, componentFlagsMask: 0)
            guard let comp = AudioComponentFindNext(nil, &desc) else { return }
            var u: AudioUnit?
            guard AudioComponentInstanceNew(comp, &u) == noErr, let au = u else { return }
            var fmt = AudioStreamBasicDescription(mSampleRate: 48000, mFormatID: kAudioFormatLinearPCM,
                                                  mFormatFlags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
                                                  mBytesPerPacket: 4, mFramesPerPacket: 1, mBytesPerFrame: 4, mChannelsPerFrame: 2, mBitsPerChannel: 16, mReserved: 0)
            AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &fmt, UInt32(MemoryLayout<AudioStreamBasicDescription>.size))
            var cb = AURenderCallbackStruct(inputProc: renderCallback, inputProcRefCon: nil)
            AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, UInt32(MemoryLayout<AURenderCallbackStruct>.size))
            guard AudioUnitInitialize(au) == noErr else { return }
            unit = au
        }
        if let au = unit, AudioOutputUnitStart(au) == noErr { running = true }
    }
    func stop() { if running, let au = unit { AudioOutputUnitStop(au); running = false } }
}
