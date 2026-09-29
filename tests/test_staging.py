"""Focused checks for the portable game assembly and archive gates."""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import stage
import package


RULES = """[VehicleTypes]
0=TANK
1=APC
2=HARV
[InfantryTypes]
0=MEDIC
1=E1
[TANK]
Speed=6
Primary=Gun
[APC]
Speed=8
Passengers=5
Primary=Blank
[HARV]
Speed=6
Primary=Gun
Harvester=yes
[MEDIC]
Speed=4
Primary=Heal
[E1]
Speed=5
Primary=Gun
[Gun]
Damage=25
[Heal]
Damage=-10
[Blank]
Damage=0
"""


class StageTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.game = self.root / "installed"
        self.out = self.root / "candidate"
        self.dll = self.root / "new.dll"
        for name in ("game.exe", "Game.edb", "TiberianSun.exe", "LaunchVinifera.dat", "Vinifera.dll"):
            path = self.game / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
        self.dll.write_bytes(b"sanitized replacement")
        self.put("INI/Rules.ini", RULES)
        self.put("INI/Firestrm.ini", "[General]\nName=Firestorm\n")
        self.put("INI/Keyboard.ini", "[Hotkey]\nDeployObject=68\nSelectView=577\n")
        self.put("Resources/KeyboardCommands.ini", "[DeployObject]\nDefaultKey=68\n")
        self.put("Resources/ClientDefinitions.ini", "[Settings]\nKeyboardINI=Keyboard.ini\n")
        self.put("Maps/Missions/test.map", "[VehicleTypes]\n0=TANK\n1=APC\n[APC]\nPrimary=Gun\n")
        self.put("Maps/Missions/weapon-only.map", "[Blank]\nDamage=25\n")
        self.put("Client/client.log", "private log")
        self.put("SUN.ini", "[MultiPlayer]\nHandle=private\n")
        self.put("Saved Games/save.sav", "private save")
        self.put("Resources/clientxna.pdb", "debug symbol")
        self.put("Map Editor/FinalSun/FinalSun.ini", "[TS]\nExe=private-installation\n")
        self.put("Map Editor/UpdaterLog.txt", "private updater history")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def put(self, name: str, text: str) -> None:
        path = self.game / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("cp1252"))

    def test_stage_keeps_load_bearing_base_and_uses_clean_defaults(self) -> None:
        report = stage.stage(self.game, self.dll, self.out)
        self.assertEqual((self.out / "Vinifera.dll").read_bytes(), self.dll.read_bytes())
        self.assertTrue((self.out / "Game.edb").is_file())
        self.assertFalse((self.out / "Client").exists())
        self.assertFalse((self.out / "Saved Games").exists())
        self.assertFalse((self.out / "Resources/clientxna.pdb").exists())
        self.assertFalse((self.out / "Map Editor/UpdaterLog.txt").exists())
        editor = stage.read_ini(self.out / "Map Editor/FinalSun/FinalSun.ini")
        self.assertNotIn("private-installation", editor)
        self.assertEqual(stage.sections(editor)["TS"]["exe"], "..\\..\\")
        self.assertNotIn("private", (self.out / "SUN.ini").read_text())
        video = stage.sections(stage.read_ini(self.out / "SUN.ini"))["VIDEO"]
        self.assertGreater(int(video["screenwidth"]), 0)
        self.assertGreater(int(video["screenheight"]), 0)
        self.assertEqual(video["usegraphicspatch"].lower(), "true")
        self.assertEqual(stage.sections(stage.read_ini(self.out / "Resources/ClientDefinitions.ini"))["SETTINGS"]["modmode"], "true")
        self.assertIn("experimental", (self.out / "README - Attack Move.txt").read_text(encoding="cp1252"))
        for name in ("Keyboard.ini", "INI/Keyboard.ini"):
            keys = stage.sections(stage.read_ini(self.out / name))["HOTKEY"]
            self.assertEqual(keys["attackmove"], "65")
            self.assertEqual(keys["selectview"], "577")
        self.assertIn("[AttackMove]", stage.read_ini(self.out / "Resources/KeyboardCommands.ini"))
        self.assertIn("TANK", report["enabled_rules_types"])
        self.assertIn("E1", report["enabled_rules_types"])
        self.assertNotIn("HARV", report["enabled_rules_types"])
        self.assertNotIn("MEDIC", report["enabled_rules_types"])
        self.assertNotIn("APC", report["enabled_rules_types"])
        self.assertIn("APC", report["map_overrides_enabled"]["Maps/Missions/test.map"])
        self.assertIn("APC", report["map_overrides_enabled"]["Maps/Missions/weapon-only.map"])
        self.assertEqual(stage.sections(stage.read_ini(self.out / "Maps/Missions/test.map"))["APC"]["attackmovescaninterval"], "15")
        manifest = json.loads((self.out / "manifest.json").read_text())
        self.assertIn("Game.edb", manifest["files"])
        self.assertEqual(manifest["native_gameplay"], "pending")

    def test_no_overwrite_and_private_utf16_rejection(self) -> None:
        stage.stage(self.game, self.dll, self.out)
        with self.assertRaises(FileExistsError):
            stage.stage(self.game, self.dll, self.out)
        private = self.root / "private.bin"
        private.write_bytes("secret-marker".encode("utf-16le"))
        self.assertTrue(package.privacy_hits([private], self.root, ["secret-marker"]))

    def test_archive_is_deterministic_and_verified(self) -> None:
        source = self.root / "data"
        source.mkdir()
        (source / "one.txt").write_text("one")
        first = package.write_zip(self.root / "first.zip", "root", source, [source / "one.txt"])
        second = package.write_zip(self.root / "second.zip", "root", source, [source / "one.txt"])
        self.assertEqual(first["sha256"], second["sha256"])

    def test_source_archive_keeps_clean_template_and_excludes_runtime_settings(self) -> None:
        source = self.root / "source"
        for name in ("content/SUN.ini", "SUN.ini", "runtime/SUN.ini", "Keyboard.ini", "spawn.ini",
                     "engine/src/debug/debughandler.h", "build/generated.cpp", "debug/run.log"):
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("fixture")
        included = {path.relative_to(source).as_posix() for path in package.source_files(source)}
        self.assertEqual(included, {"content/SUN.ini", "engine/src/debug/debughandler.h"})

    def test_zero_damage_payload_classifier(self) -> None:
        rules = {
            "FLAME": {"damage": "0", "ambientdamage": "2", "attachedparticlesystem": "FireStream", "usefireparticles": "yes"},
            "RAIL": {"damage": "0", "ambientdamage": "150", "attachedparticlesystem": "Rail", "israilgun": "true"},
            "ROCKET": {"damage": "0", "projectile": "Cluster"},
            "CLUSTER": {"splits": "yes", "airburstweapon": "Child"},
            "CHILD": {"damage": "5"},
            "WEB": {"damage": "0", "projectile": "Capsule", "warhead": "WebMass"},
            "HEAL": {"damage": "-50"},
        }
        for name in ("FLAME", "RAIL", "ROCKET"):
            self.assertTrue(stage.weapon_is_combat(name, rules), name)
        for name in ("WEB", "HEAL"):
            self.assertFalse(stage.weapon_is_combat(name, rules), name)

    def test_native_receipt_must_match_dll(self) -> None:
        receipt = self.root / "receipt.json"
        receipt.write_text(json.dumps({"dll_sha256": "a" * 64, "native_gameplay": "pass", "checks": {"load": "pass"}}))
        with self.assertRaises(ValueError):
            package.receipt_status(receipt, "b" * 64)
        result = package.receipt_status(receipt, "a" * 64)
        self.assertEqual(result["native_gameplay"], "pass")
        self.assertEqual(result["multiplayer"], "unverified")


if __name__ == "__main__":
    unittest.main()
