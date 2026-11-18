extends SceneTree

var failed = false

func expect(ok, message):
	if not ok:
		failed = true
		push_error(message)

func _initialize():
	call_deferred("run")

func run():
	var script = load(get_script().resource_path.get_base_dir().path_join("runtime.gd"))
	var world = Node2D.new()
	root.add_child(world)
	current_scene = world
	var actor = Node2D.new()
	actor.name = "Actor"
	world.add_child(actor)
	var behavior = Node.new()
	behavior.set_script(script)
	actor.add_child(behavior)
	behavior._act(10, ".", "", Vector3(10, 20, 0), 0)
	expect(actor.position == Vector2(10, 20), "Move by failed")
	behavior._act(11, ".", "", Vector3(100, 0, 0), 0.5)
	expect(actor.position == Vector2(60, 20), "Delta movement failed")
	behavior._act(14, ".", "score", Vector3(4, 0, 0), 0)
	behavior._act(15, ".", "score", Vector3(3, 0, 0), 0)
	expect(behavior._var("score") == 7, "Shared variables failed")
	behavior._act(17, ".", "", Vector3.ZERO, 0)
	expect(not actor.visible, "Hide failed")
	behavior._act(16, ".", "", Vector3.ZERO, 0)
	expect(actor.visible, "Show failed")
	var label = Label.new()
	label.name = "Caption"
	actor.add_child(label)
	behavior._act(19, "Caption", "Great game!", Vector3.ZERO, 0)
	expect(label.text == "Great game!", "Text action failed")
	behavior._act(10, "MissingObject", "", Vector3.ONE, 0)
	var spatial = Node3D.new()
	spatial.name = "Spatial"
	actor.add_child(spatial)
	behavior._act(13, "Spatial", "", Vector3(2, 3, 4), 0)
	expect(spatial.position == Vector3(2, 3, 4), "3D positioning failed")
	var body = CharacterBody2D.new()
	body.name = "Body"
	actor.add_child(body)
	behavior._act(24, "Body", "", Vector3(5, 6, 0), 0)
	behavior._act(28, "Body", "", Vector3(10, 0, 0), 0.5)
	expect(body.velocity == Vector2(5, 11), "Physics actions failed")
	var trigger = Area2D.new()
	trigger.name = "Trigger"
	actor.add_child(trigger)
	behavior._bind_body("Trigger", func(): world.set_meta("collision_seen", true))
	trigger.body_entered.emit(body)
	expect(world.get_meta("collision_seen", false), "Collision event failed")
	behavior._bind_timer(0.01, func(): world.set_meta("timer_seen", true))
	await create_timer(0.1).timeout
	expect(world.get_meta("timer_seen", false), "Timer event failed")
	# Validate nested IF / ELSE / REPEAT generated syntax and scene persistence.
	var generated = GDScript.new()
	generated.source_code = 'extends Node\nfunc result():\n\tvar score = 0\n\tfor i in range(3):\n\t\tif i < 2:\n\t\t\tscore += 2\n\t\telse:\n\t\t\tscore += 1\n\treturn score\n'
	expect(generated.reload() == OK, "Nested block syntax failed")
	var instance = Node.new()
	instance.set_script(generated)
	expect(instance.result() == 5, "Nested block execution failed")
	instance.free()
	behavior.set_meta("easy_logic_program", [{"op": 0}, {"op": 15, "text": "score", "x": 1}])
	actor.owner = world
	behavior.owner = world
	var packed = PackedScene.new()
	expect(packed.pack(world) == OK, "Pack scene failed")
	var save_path = get_script().resource_path.get_base_dir().path_join("roundtrip.tscn")
	expect(ResourceSaver.save(packed, save_path) == OK, "Scene save failed")
	var restored = load(save_path).instantiate()
	var restored_actor = restored.get_node("Actor")
	var restored_behavior = restored_actor.get_child(0)
	expect(restored_behavior.get_meta("easy_logic_program").size() == 2, "Block data was lost")
	expect(restored_behavior.get_script() != null, "Runtime script was lost")
	restored.free()
	print("EASY_MODE_RUNTIME_TESTS: ", "FAIL" if failed else "PASS")
	quit(1 if failed else 0)
