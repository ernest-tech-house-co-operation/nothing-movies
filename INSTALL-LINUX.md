# Installing NothingMovies on Linux

---

## Option 1 — Quick install (recommended)

### Step 1 — Install runtime dependencies

```bash
sudo apt update
sudo apt install -y \
  libqt6webenginewidgets6 \
  libqt6webenginecore6 \
  libqt6widgets6 \
  libqt6network6 \
  libqt6declarative6 \
  qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts \
  libmpv2 \
  libtorrent-rasterbar2.0 \
  libcurl4 \
  libzip4 \
  libssl3 \
  libboost-system1.83.0 \
  libboost-filesystem1.83.0
```

> **Ubuntu 22.04?** Replace `libboost-system1.83.0` and `libboost-filesystem1.83.0` with `libboost-system1.74.0` and `libboost-filesystem1.74.0`, and `libtorrent-rasterbar2.0` with `libtorrent-rasterbar2`.

---

### Step 2 — Download the binary

Go to the [Releases page](https://github.com/ernest-tech-house-co-operation/nothing-movies/releases) and download the latest `nothingmovies-vX.X.X-linux` file.

Or via terminal (replace the version tag with the one you want):

```bash
curl -L https://github.com/ernest-tech-house-co-operation/nothing-movies/releases/download/v0.0.1-9b612e060a403f11ae874de099cf1a2e5e24b359/nothingmovies-v0.0.1-linux \
  -o nothingmovies
```

---

### Step 3 — Install the binary

```bash
chmod +x nothingmovies
sudo mv nothingmovies /usr/local/bin/nothingmovies
```

---

### Step 4 — Add to app launcher

This makes NothingMovies appear in your desktop application list (GNOME, KDE, etc.):

```bash
sudo tee /usr/share/applications/nothingmovies.desktop > /dev/null <<EOF
[Desktop Entry]
Name=NothingMovies
Comment=Your movie companion
Exec=/usr/local/bin/nothingmovies
Icon=video
Type=Application
Categories=AudioVideo;Video;
Terminal=false
EOF
```

Then update the app database:

```bash
sudo update-desktop-database /usr/share/applications
```

NothingMovies will now appear in your app list. You may need to log out and back in on some desktop environments.

---

## Option 2 — Run without installing

If you just want to try it without touching system files:

```bash
# Install deps (same as Step 1 above, still required)
sudo apt update && sudo apt install -y libqt6webenginewidgets6 libqt6webenginecore6 \
  libqt6widgets6 libqt6network6 libqt6declarative6 \
  qml6-module-qtquick-controls qml6-module-qtquick-layouts \
  libmpv2 libtorrent-rasterbar2.0 libcurl4 libzip4 libssl3

# Download and run
chmod +x nothingmovies-v0.0.1-linux
./nothingmovies-v0.0.1-linux
```

---

## Uninstall

```bash
sudo rm /usr/local/bin/nothingmovies
sudo rm /usr/share/applications/nothingmovies.desktop
sudo update-desktop-database /usr/share/applications
```
