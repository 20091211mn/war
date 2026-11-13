class_name C4
extends Node3D
# عبوة C4: تُلصق على السطح وتنفجر بزر التفجير؛ ضوء أحمر يومض

var src: Fighter
var led: MeshInstance3D
var t := 0.0

static func make(g: Node, s: Fighter, pos: Vector3, nrm: Vector3) -> C4:
	var n := C4.new(); n.src = s
	var b := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = Vector3(0.22, 0.05, 0.14); b.mesh = bm
	var m := StandardMaterial3D.new(); m.albedo_color = Color(0.62, 0.58, 0.45); m.roughness = 0.8; b.material_override = m; n.add_child(b)
	n.led = MeshInstance3D.new(); var lm := SphereMesh.new(); lm.radius = 0.014; lm.height = 0.028; n.led.mesh = lm; n.led.position = Vector3(0.07, 0.035, 0)
	var lmat := StandardMaterial3D.new(); lmat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED; lmat.albedo_color = Color(1, 0.1, 0.05); n.led.material_override = lmat; n.add_child(n.led)
	g.world.add_child(n); n.global_position = pos + nrm * 0.025
	var up := nrm.normalized(); var ax := Vector3.RIGHT if absf(up.dot(Vector3.RIGHT)) < 0.9 else Vector3.FORWARD
	var fw := up.cross(ax).normalized(); n.global_transform.basis = Basis(fw.cross(up).normalized(), up, fw)
	return n

func _process(dt: float) -> void:
	t += dt; led.visible = int(t * 2.5) % 2 == 0
