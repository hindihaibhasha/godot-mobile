# Godot Easy Mode (mobile + beginner friendly)

Is fork mein Godot editor ke neeche ek **badi buttons wali "Easy Mode" bar** hai.
Phone / tablet par yeh apne aap ON hoti hai. Har zaroori kaam ek tap mein:

| Button  | Kya karta hai |
|---------|---------------|
| New 2D  | Naya 2D game (zameen/ground ke saath) bana deta hai |
| New 3D  | Naya 3D world (sky, sun, camera, floor ke saath) bana deta hai |
| Add     | Badi list kholta hai: Player, Ground, Box, Coin, Text, Picture, Camera, Lamp... |
| Code    | Select kiye object ka code kholta hai. Code nahi hai to ek simple starter script bana deta hai (koi dialog nahi) |
| Delete  | Select kiya object hata deta hai |
| Undo / Redo | Galti theek karo |
| Save    | Bina file-dialog ke save karta hai (pehla scene "main scene" ban jata hai) |
| Play    | Save karke game chala deta hai |
| Help    | Step-by-step guide |
| Hide    | Bar ko chhota kar deta hai ("Show Easy Mode buttons" se wapas) |

"Player" pehle se chalne wale code ke saath aata hai, phone par touch se chalta hai:

* **2D jumping game:** screen ka left/right hissa = chalo, beech = jump
* **2D top-down:** ungli jahan rakho player wahan jaata hai
* **3D:** left half par ungli slide = joystick, right half tap = jump

Scripts `res://scripts/` mein bante hain, unhe kholkar numbers (speed, jump) badal sakte ho.

Easy Mode band/chalu: **Editor Settings → Interface → Easy Mode → Enabled**.

## APK apne aap kaise banta hai

1. Is code ko apne GitHub repo mein push karo.
2. Har push par workflow **"📱 Build Android Editor APK (Easy Mode)"** chalta hai
   (`.github/workflows/easy_android_apk.yml`). Pehli baar ~1.5–3 ghante, baad mein cache se tez.
3. APK milega:
   * **Actions** tab → latest run → *Artifacts* → `godot-easy-mode-apk`, ya
   * **Releases** → `Godot Easy Mode APK (latest)` (sirf default branch par).
4. Phone par `.apk` install karo ("Install unknown apps" allow karo).

APK debug key se signed hai. Apni release key use karni ho to repo secrets mein keystore daal kar
`GODOT_ANDROID_SIGN_KEYSTORE`, `GODOT_ANDROID_KEYSTORE_ALIAS`, `GODOT_ANDROID_SIGN_PASSWORD` env set karo.

Purana bada multi-platform CI (`runner.yml`) ab sirf haath se (workflow_dispatch) chalta hai,
taaki har push par sirf APK bane aur GitHub minutes bachein.

Code: `editor/gui/easy_mode_bar.cpp/.h`, editor mein jodne ki line `editor/editor_node.cpp` mein.
