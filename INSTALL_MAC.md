# Installing MathEngine on macOS

## 1. Download the DMG

Get the latest `MathEngine-macOS.dmg` from the [releases page](https://github.com/theomirzakhanian/mathengine/releases).

## 2. Install the app

Open the DMG and drag **MathEngine** onto the **Applications** folder.

## 3. Open it the first time

Because the app isn't signed with an Apple Developer certificate (open-source apps usually aren't), macOS will block it on first launch with a message like:

> "MathEngine" can't be opened because Apple cannot check it for malicious software.

This is **not** a real warning about the app. It just means macOS hasn't verified the developer. To get past it:

### Option A — Right-click open (easiest)

1. Open your **Applications** folder.
2. **Right-click** (or hold Control and click) on **MathEngine**.
3. Choose **Open** from the menu.
4. In the popup that appears, click **Open** again.

You only need to do this once. After that, double-click works normally.

### Option B — System Settings

If you double-clicked first and got blocked:

1. Open **System Settings** → **Privacy & Security**.
2. Scroll down to the **Security** section.
3. You should see "MathEngine was blocked..." with an **Open Anyway** button.
4. Click **Open Anyway**, then click **Open** in the next dialog.

### Option C — Terminal (advanced)

If Options A and B don't work, the download quarantine flag may need to be stripped:

```bash
xattr -cr /Applications/MathEngine.app
```

Then double-click the app normally.

## That's it

Once the app opens, it should launch immediately on every future click.

---

If something else goes wrong, [open an issue](https://github.com/theomirzakhanian/mathengine/issues).
