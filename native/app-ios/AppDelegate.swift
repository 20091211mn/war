import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        let w = UIWindow(frame: UIScreen.main.bounds)
        w.rootViewController = GameViewController()
        w.makeKeyAndVisible()
        window = w
        application.isIdleTimerDisabled = true
        return true
    }
    func applicationWillResignActive(_ application: UIApplication) { wz_pause(); AudioOut.shared.stop() }
    func applicationDidBecomeActive(_ application: UIApplication) { AudioOut.shared.start() }
}
