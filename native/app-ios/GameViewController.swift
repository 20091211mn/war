import UIKit

/// يضع شاشة اللعبة داخل المنطقة الآمنة (بعيداً عن الحاجب) ويثبّت الاتجاه أفقياً.
final class GameViewController: UIViewController {
    private let gameView = GameView(frame: .zero)

    override func loadView() {
        let root = UIView(frame: UIScreen.main.bounds)
        root.backgroundColor = .black
        gameView.frame = root.bounds
        root.addSubview(gameView)
        view = root
    }
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        let i = view.safeAreaInsets, b = view.bounds
        gameView.frame = CGRect(x: i.left, y: 0, width: b.width - i.left - i.right, height: b.height)
    }
    override var prefersStatusBarHidden: Bool { true }
    override var prefersHomeIndicatorAutoHidden: Bool { true }
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask { .landscape }
    override var preferredScreenEdgesDeferringSystemGestures: UIRectEdge { .all }
}
