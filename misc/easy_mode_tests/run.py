"""Run the embedded block runtime regression test with a Godot 4 executable."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
source = (root / "editor/gui/easy_logic_editor.cpp").read_text()
runtime = source.split('R"GD(', 1)[1].split(')GD"', 1)[0]
with tempfile.TemporaryDirectory() as directory:
    folder = pathlib.Path(directory)
    (folder / "runtime.gd").write_text("extends Node\n" + runtime)
    (folder / "runtime_test.gd").write_text(pathlib.Path(__file__).with_name("runtime_test.gd").read_text())
    result = subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "godot", "--headless", "--script", str(folder / "runtime_test.gd")])
    sys.exit(result.returncode)
