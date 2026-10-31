#!/usr/bin/env python3
"""Verify current AirPort bundle metadata and forbidden imports without loading a kext."""
import pathlib, plistlib, subprocess, sys

root = pathlib.Path(__file__).resolve().parents[1]
bundle = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "build/out/AirPort_RTW88.kext"
plist_path = bundle / "Contents/Info.plist"
plist = plistlib.loads(plist_path.read_bytes())
template = plistlib.loads((root / "AirPort_RTW88.kext/Contents/Info.plist").read_bytes())

assert plist == template, "Packaged plist differs from source"
assert plist["CFBundleIdentifier"] == "com.gptmade.wifi"
assert plist["CFBundleName"] == "GPTMadeAWiFiDriver"
assert plist["CFBundleVersion"] == plist["CFBundleShortVersionString"] == "1.0.0"
assert plist["CFBundlePackageType"] == "KEXT"
assert "com.apple.kpi.private" not in plist["OSBundleLibraries"]

executable = bundle / "Contents/MacOS" / plist["CFBundleExecutable"]
assert executable.is_file(), f"Missing executable: {executable}"
binary = executable.read_bytes()
assert b"com.gptmade.wifi" in binary, "kmod identifier does not match bundle identifier"
assert b"1.0.0" in binary, "Unexpected kmod version"

for name, personality in plist["IOKitPersonalities"].items():
    assert personality["CFBundleIdentifier"] == plist["CFBundleIdentifier"], name
    assert personality["IOProviderClass"] == "IOPCIDevice", name
    assert set(personality["IOPCIMatch"].split()) == {"0xC82110EC", "0xB82110EC"}, name

undefined = subprocess.check_output(["xcrun", "nm", "-uj", str(executable)], text=True).splitlines()
for symbol in ["_thread_call_cancel_wait", "_OSRuntimeInitializeCPP", "_OSRuntimeFinalizeCPP"]:
    assert symbol not in undefined, symbol

subprocess.run(["codesign", "--verify", "--strict", str(bundle)], check=True)
print("PASS: bundle metadata, kmod identity/version, PCI personalities, signature and forbidden imports")
