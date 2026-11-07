// آيفون أصلي بالكامل (UIKit + C++): لا WebView ولا HTML. العرض: CALayer.contents من صورة البكسلات ، الصوت: AudioUnit ، اللمس: UITouch.
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include "../mobile.h"

static Mobile* M=nullptr; static AudioUnit gAU=NULL;
static OSStatus audioCb(void*,AudioUnitRenderActionFlags*,const AudioTimeStamp*,UInt32,UInt32 frames,AudioBufferList*io){ SFX::mix((int16_t*)io->mBuffers[0].mData,(int)frames); return noErr; }
static void startAudio(){
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
    SFX::initBuffers();
    AVAudioSession*s=[AVAudioSession sharedInstance]; [s setCategory:AVAudioSessionCategoryPlayback error:nil]; [s setActive:YES error:nil];
    AudioComponentDescription d={kAudioUnitType_Output,kAudioUnitSubType_RemoteIO,kAudioUnitManufacturer_Apple,0,0}; AudioComponent c=AudioComponentFindNextComponent(NULL,&d); if(!c)return;
    if(AudioComponentInstanceNew(c,&gAU)!=noErr)return;
    AudioStreamBasicDescription f={}; f.mSampleRate=48000; f.mFormatID=kAudioFormatLinearPCM; f.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked; f.mChannelsPerFrame=2; f.mBitsPerChannel=16; f.mBytesPerFrame=4; f.mBytesPerPacket=4; f.mFramesPerPacket=1;
    AudioUnitSetProperty(gAU,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&f,sizeof f);
    AURenderCallbackStruct cb={audioCb,NULL}; AudioUnitSetProperty(gAU,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&cb,sizeof cb);
    AudioUnitInitialize(gAU); AudioOutputUnitStart(gAU); });
}

static UITouch* gSlots[12];
static int slotOf(UITouch*t,bool create){ for(int i=0;i<12;i++)if(gSlots[i]==t)return i; if(!create)return -1; for(int i=0;i<12;i++)if(!gSlots[i]){ gSlots[i]=t; return i; } return -1; }

@interface WZView : UIView @end
@implementation WZView { CADisplayLink*_dl; CFTimeInterval _last; CGColorSpaceRef _cs; }
- (instancetype)initWithFrame:(CGRect)f{
  self=[super initWithFrame:f]; if(!self)return nil; self.multipleTouchEnabled=YES; self.opaque=YES; self.backgroundColor=[UIColor blackColor]; self.layer.magnificationFilter=kCAFilterLinear; self.layer.contentsGravity=kCAGravityResize; self.layer.opaque=YES; _cs=CGColorSpaceCreateDeviceRGB(); return self; }
- (void)layoutSubviews{ [super layoutSubviews]; CGSize s=self.bounds.size; if(!M){ int cores=(int)[[NSProcessInfo processInfo] activeProcessorCount]; M=new Mobile(); M->start((int)s.width,(int)s.height,cores); startAudio(); } else M->setScreen((int)s.width,(int)s.height); }
- (void)didMoveToWindow{ if(self.window&&!_dl){ _dl=[CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)]; _dl.preferredFramesPerSecond=60; [_dl addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes]; _last=0; } }
- (void)tick:(CADisplayLink*)l{
  if(!M)return; CFTimeInterval now=l.timestamp; float dt=_last>0?(float)(now-_last):1.f/60.f; _last=now; M->frame(dt);
  int W=M->W,H=M->H; CFDataRef d=CFDataCreate(NULL,(const UInt8*)M->out.data(),(CFIndex)W*H*4); CGDataProviderRef p=CGDataProviderCreateWithCFData(d);
  CGImageRef img=CGImageCreate(W,H,8,32,(size_t)W*4,_cs,kCGBitmapByteOrderDefault|kCGImageAlphaNoneSkipLast,p,NULL,false,kCGRenderingIntentDefault);
  self.layer.contents=(__bridge id)img; CGImageRelease(img); CGDataProviderRelease(p); CFRelease(d); }
- (void)touchesBegan:(NSSet<UITouch*>*)ts withEvent:(UIEvent*)e{ if(!M)return; for(UITouch*t in ts){ int i=slotOf(t,true); if(i<0)continue; CGPoint p=[t locationInView:self]; M->touchDown(i,(float)p.x,(float)p.y); } }
- (void)touchesMoved:(NSSet<UITouch*>*)ts withEvent:(UIEvent*)e{ if(!M)return; for(UITouch*t in ts){ int i=slotOf(t,false); if(i<0)continue; CGPoint p=[t locationInView:self]; M->touchMove(i,(float)p.x,(float)p.y); } }
- (void)touchesEnded:(NSSet<UITouch*>*)ts withEvent:(UIEvent*)e{ if(!M)return; for(UITouch*t in ts){ int i=slotOf(t,false); if(i<0)continue; M->touchUp(i); gSlots[i]=nil; } }
- (void)touchesCancelled:(NSSet<UITouch*>*)ts withEvent:(UIEvent*)e{ if(!M)return; M->releaseAll(); for(int i=0;i<12;i++)gSlots[i]=nil; }
@end

@interface WZVC : UIViewController @end
@implementation WZVC { WZView*_v; }
- (void)loadView{ UIView*root=[[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds]; root.backgroundColor=[UIColor blackColor]; self.view=root; _v=[[WZView alloc] initWithFrame:root.bounds]; [root addSubview:_v]; }
- (void)viewDidLayoutSubviews{ [super viewDidLayoutSubviews]; UIEdgeInsets in=self.view.safeAreaInsets; CGRect b=self.view.bounds; _v.frame=CGRectMake(in.left,0,b.size.width-in.left-in.right,b.size.height); }
- (BOOL)prefersStatusBarHidden{ return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden{ return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations{ return UIInterfaceOrientationMaskLandscape; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures{ return UIRectEdgeAll; }
@end

@interface WZApp : UIResponder <UIApplicationDelegate> @property(strong) UIWindow*window; @end
@implementation WZApp
- (BOOL)application:(UIApplication*)a didFinishLaunchingWithOptions:(NSDictionary*)o{
  self.window=[[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds]; self.window.rootViewController=[WZVC new]; [self.window makeKeyAndVisible]; a.idleTimerDisabled=YES; return YES; }
- (void)applicationWillResignActive:(UIApplication*)a{ if(M){ M->releaseAll(); if(!M->g.menu&&!M->g.over&&M->g.role==0)M->g.paused=true; } if(gAU)AudioOutputUnitStop(gAU); }
- (void)applicationDidBecomeActive:(UIApplication*)a{ if(gAU)AudioOutputUnitStart(gAU); }
@end

int main(int argc,char**argv){ @autoreleasepool{ return UIApplicationMain(argc,argv,nil,@"WZApp"); } }
