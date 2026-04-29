Import("env")
import subprocess
import time

def wait_for_dfu():
    print("⏳ Waiting for DFU device...")
    while True:
        try:
            out = subprocess.check_output("lsusb", shell=True).decode()
            if "0483:df11" in out:
                print("✅ DFU device found")
                return
        except:
            pass
        time.sleep(0.3)

def dfu_upload(source, target, env):
    firmware_path = str(source[0])

    wait_for_dfu()

    print("🚀 Uploading via DFU...")
    cmd = f"dfu-util -d 0483:df11 -a 0 -s 0x08040000 -D {firmware_path}"
    result = subprocess.call(cmd, shell=True)

    if result != 0:
        print("❌ Upload failed")
    else:
        print("✅ Upload done (press RESET to run)")

env.Replace(UPLOADCMD=dfu_upload)