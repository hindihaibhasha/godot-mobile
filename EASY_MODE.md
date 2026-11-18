# Godot Mobile — Easy Mode

Phone par beginner workspace: **Home · Map · Logic · Play/Stop · Tools**.
Home mein Jump & Collect, Explore a Room aur My First 3D World starters hain.
Tools mein New 2D, New 3D, Objects, Add object, Canvas, Edit, Duplicate, Delete, Undo/Redo, Save aur Help hain.
Full editor se advanced Godot features milte hain; Back to mobile workspace se wapas aayen.

## 1. Jaldi level banayein

1. **More → New 2D / New 3D**: nayi scene aur Map builder khulega.
2. Platform layout / Room layout chunen, ya khud grid par paint karein.
3. Brush: Ground, Wall, Player, Coin, Box; Eraser se cell saaf karein.
4. Ungli se paint karein; scrollbars se map ke doosre hisse tak jayein.
5. **Build map → Play**. Collision, child nodes, ownership aur player scripts automatically bante hain.

2D map X/Y plane par hai; 3D map upar se X/Z plane par dikhta hai.
Map abhi 24 × 16 cells ka hai. Cell size 2D mein normally 64 pixels aur 3D mein 2 meters hai.
Objects menu se created objects chun kar Edit se position/size badal sakte hain.
Duplicate selected objects ki copy ko 2D mein 64 pixels / 3D mein 2 meters side mein rakhta hai.

**Rebuild note:** Map builder `PaintedMap` ko regenerate karta hai. Iske andar manually badle transforms,
blocks ya added objects agle rebuild par replace honge. Scene ke doosre objects preserve hote hain.
Map layout pehle finalize karein, phir object logic lagayein. Undo purana map restore karta hai.
Existing scenes ka automatic grid conversion abhi nahi hota.

## 2. Bina code behavior banayein

1. **Logic** tap karein aur object choose karein.
2. Rang-wise drawer: Events, Control, Movement, Variables, Looks / Sound, Scenes.
3. Puzzle block ko workspace ke **+ slot** par drag karein. Phone par **+ slot tap → block tap** bhi supported hai.
4. Placed block tap karke relevant values edit karein; isi dialog se block/section delete karein.
5. IF/REPEAT aur events apne children ke saath move/delete hote hain. Invalid connections reject hote hain.
6. Workspace Undo/Redo se block edits reverse karein. **Apply blocks**, phir **Tools → Save / Play**.

Drawer ko hide karke workspace ke liye zyada jagah milegi. Existing scene block data supported hai.

### Available blocks

- Events: game start, every physics frame, screen tap, body entered, repeating timer.
- Logic: IF variable >= / < value, ELSE, REPEAT (1–100), END.
- Input / physics: IF input held, IF on floor, velocity, jump, gravity, move and slide.
- Transform: move, move per second, rotate, set position (2D aur 3D).
- State: set/add shared numeric variable, show/hide, delete.
- Content: label text, sound/animation, spawn saved scene, change/reload scene.
- Numeric property: existing property ka value set karein, jaise player ka speed.

IF / REPEAT ka END automatically add hota hai. Nested sections supported hain.
Variable names scene-wide hain; missing variables zero se start hote hain.
Body entered ke liye selected target par `body_entered` signal hona chahiye (normally Area2D/3D).
Scene load/spawn blocks mein saved `.tscn` ka `res://` path chahiye.

### Example: tap par score

- WHEN screen tapped
- Add to variable: text `score`, X `1`
- IF variable >= number: text `score`, X `10`
- Set label text: target apna Label, text `You win!`
- END section

Move preset aur Tap score buttons independent example events append karte hain.
Existing object scripts replace nahi hote: behavior ek `LogicBlocks` child aur embedded GDScript mein save hota hai.
Block data scene metadata mein rehta hai; Apply Undo/Redo supported hai.
Ready-made player par extra movement/physics blocks uske movement script se conflict kar sakte hain;
custom physics ke liye unscripted CharacterBody use karein.

Yeh App Inventor-inspired category drawer aur structured snap-slot workspace hai; MIT App Inventor/Blockly engine ka port nahi hai.
Free-position canvas, expression/value blocks ko sockets mein drag karna, custom functions,
multiplayer blocks aur har Godot API ka block abhi available nahi hai.
Map builder level layout ke liye hai; Blender-style mesh modeling / terrain sculpting nahi hai.

## Player touch controls

- 2D platformer: left/right screen = walk, middle = jump.
- 2D top-down: finger ki taraf walk.
- 3D: left half slide = movement, right half tap = jump.

## Verification and APK

Runtime regression test: `python3 misc/easy_mode_tests/run.py /path/to/godot`.
Tests cover movement, variables, visibility, labels, physics, collision/timer callbacks and scene persistence.

Push to `main` triggers `.github/workflows/easy_android_apk.yml` on GitHub Actions.
Latest APK: repository Releases → `easy-apk-latest`.
Editor settings: Interface → Easy Mode → Enabled.
