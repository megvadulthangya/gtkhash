# GtkHash CI/CD Documentation

## 1. High-Level Architecture Overview

The GtkHash CI/CD pipeline is designed to guarantee the correctness, portability, and release readiness of the GtkHash application across a diverse set of Linux distributions, GTK versions, and Windows architectures. The system is composed of three primary workflows, each with a clearly defined purpose:

*   **Compatibility Validation** (`compatibility-validation.yml`): Executes on every push, pull request, and manual dispatch to validate that the codebase builds and passes its test suite across a matrix of operating systems and GTK toolkits (GTK3 and GTK4). It also generates official source distribution tarballs as an artifact for inspection. This workflow is the project’s quality gate.
*   **Windows MSYS2 Build & Packaging** (`msys2.yml`): Also triggered on every push and pull request, this workflow uses the MSYS2 environment on GitHub-hosted Windows runners to cross-compile GtkHash for the `mingw64` and `ucrt64` targets. It produces portable ZIP archives and a proper Windows installer (`.exe`) via Inno Setup. It serves both as a continuous build check and as the canonical way to create Windows binaries.
*   **Release Management** (`releases.yml`): This is the automated release pipeline. It is triggered by pushing a version tag (`v*`) or manually via `workflow_dispatch`. It bumps version numbers in source files (when triggered manually), rebuilds the project from scratch to produce source tarballs and Windows artifacts, and creates a GitHub Release with auto-generated notes. It can optionally update the Flathub manifest for the official Flatpak.

The overall philosophy is **self-contained reliability**. The release workflow does not depend on artifacts from the other two workflows; it repeats the necessary build steps to ensure that the exact commit tagged is what gets released. The validation workflows run continuously to catch regressions early, while the release workflow is a carefully orchestrated sequence that requires minimal manual intervention.

## 2. Directory Structure Context

The CI/CD configuration is spread across two main locations:

*   **`.github/workflows/`** – Contains the GitHub Actions workflow definitions (`compatibility-validation.yml`, `msys2.yml`, `releases.yml`). These files describe *what* to execute, on which events, and with which runners.
*   **`msys2/`** – Houses all the configuration and scripts necessary to build, bundle, and package GtkHash for Windows. This directory is referenced extensively by `msys2.yml` and `releases.yml` for the Windows build job. It contains:
    *   `mingw-w64-gtkhash/PKGBUILD` – The MSYS2 package build recipe.
    *   `mingw-w64-gtkhash/gtkhash-x86_64.install` – Post-install/upgrade/remove hooks for the MSYS2 package.
    *   `bundle.sh` – A shell script that gathers all runtime dependencies into a portable directory tree.
    *   `gtkhash.iss` – The Inno Setup script that compiles the final Windows installer.

The root of the repository contains the main build system files (`meson.build`, `configure.ac`) which are modified automatically by the release workflow to reflect the new version.

## 3. Detailed Workflow Breakdown

### 3.1 `compatibility-validation.yml`

#### Trigger Conditions
*   `on: push` – Any push to any branch.
*   `on: pull_request` – Any pull request activity.
*   `on: workflow_dispatch` – Manual trigger from the GitHub Actions UI.

#### Environment & Matrix Configuration
This workflow uses a **staged execution model** to improve reliability by reducing the peak number of concurrent container image pulls. Jobs are organized into sequential stages using the `needs` keyword.

The Linux distribution coverage is as follows:

| Stage | Job Name               | Runner OS             | Container Image           | Compilers        |
|-------|------------------------|-----------------------|---------------------------|------------------|
| 1     | `ubuntu-gtk3`          | `ubuntu-22.04`        | None (native)             | `gcc`, `clang`   |
| 2     | `debian13-gtk3`        | `ubuntu-latest`       | `debian:13`               | Default (gcc)    |
| 2     | `debian13-gtk4`        | `ubuntu-latest`       | `debian:13`               | Default (gcc)    |
| 3     | `archlinux-gtk3`       | `ubuntu-latest`       | `archlinux:latest`        | Default (gcc)    |
| 3     | `archlinux-gtk4`       | `ubuntu-latest`       | `archlinux:latest`        | Default (gcc)    |
| 4     | `manjaro-gtk3`         | `ubuntu-latest`       | `manjarolinux/build:latest` | Default (gcc)  |
| 4     | `manjaro-gtk4`         | `ubuntu-latest`       | `manjarolinux/build:latest` | Default (gcc)  |
| 5     | `fedora44-gtk3`        | `ubuntu-latest`       | `fedora:44`               | Default (gcc)    |
| 5     | `fedora44-gtk4`        | `ubuntu-latest`       | `fedora:44`               | Default (gcc)    |
| 6     | `dist`                 | `ubuntu-22.04`        | None (native)             | Default (gcc)    |

**Ubuntu** is the only distribution tested with both GCC and Clang; all others use the distribution’s default compiler. GTK4 variants include a dynamic detection step to gracefully skip the build if GTK4 UI files are not yet present in the repository.

#### Step-by-Step Deep Dive (Common pattern across Linux jobs)

Each GTK3 job follows this sequence:

1.  **Checkout**: Fetches the repository using `actions/checkout`.
2.  **Install Dependencies**: Runs distribution-specific package manager commands (`apt-get`, `pacman`, `dnf`) to install all build and test prerequisites. Notable dependencies include the GTK development packages, hash libraries (gcrypt, nettle, libb2, mbedtls, openssl), and test tools (xvfb, icon themes, D-Bus).
3.  **Meson Setup**: Configures the build system with a `debug` build type and a selection of hash library options and file manager extensions (Nautilus, Nemo, Caja, Thunar) appropriate for the platform. This step tests the configuration phase of the build system.
4.  **Build**: Compiles the project using `ninja`.
5.  **Test**: Executes the test suite within a virtual X server (`xvfb-run`). For GTK3, tests run with parallel execution; for GTK4, a single process is enforced (`--num-processes 1`) to avoid race conditions, and D-Bus is started explicitly.

GTK4 jobs have an additional step: **Detect GTK4 Support**. This checks for the existence of GTK4-specific UI definition files (e.g., `data/gtkhash-gtk4.ui`). If none are found, the job outputs `enabled=false` and the subsequent build/test steps are skipped, with only a notification message printed. This mechanism allows GTK4 support to be developed incrementally without breaking CI.

The **`dist` job** (stage 6) runs only after all Fedora jobs succeed. It performs:
*   **Meson setup** (with all features enabled)
*   **Meson dist**: Creates `.tar.gz` and `.tar.xz` distribution tarballs using `meson dist --formats gztar,xztar --no-tests`.
*   **SHA256 Checksums**: Generates `.sha256` files for each tarball.
*   **Upload Artifacts**: Stores the tarballs and checksums as a workflow artifact named `gtkhash-dist-<sha>` with a 90-day retention.

### 3.2 `msys2.yml`

#### Trigger Conditions
*   `on: push`
*   `on: pull_request`
*   `on: workflow_dispatch`

#### Environment & Matrix Configuration
This workflow runs on `windows-latest` and uses a matrix to target two MSYS2 environments:

| `sys`     | Description                      |
|-----------|----------------------------------|
| `mingw64` | Classic MinGW-w64 (MSVCRT)      |
| `ucrt64`  | Modern UCRT (Universal C Runtime) |

Both build jobs use the `msys2/setup-msys2@v2` action to provision the environment. The shell is set to `msys2 {0}` for all run steps.

#### Step-by-Step Deep Dive

1.  **Disable Git autocrlf**: Prevents line-ending conversion on checkout, which is critical for shell scripts and MSYS2 packages.
2.  **Checkout**: Fetches the full repository (with `fetch-depth: 0` to allow `git archive`).
3.  **Setup MSYS2**: Installs the base development tools (`base-devel`, `git`, `tree`, `zip`) and additional packages via `pacboy` (a helper that installs both the native and mingw-w64 variants of packages). Key packages include `toolchain`, `gtk3`, `adwaita-icon-theme`, `libgcrypt`, `meson`, and `imagemagick`.
4.  **Run makepkg-mingw (Build & Test & Install)**:
    *   `git archive` creates a source tarball (`gtkhash.tar.gz`) from the current HEAD and places it inside `msys2/mingw-w64-gtkhash/`. This is required by the PKGBUILD.
    *   `makepkg-mingw` builds the package, runs the test suite (as defined in the PKGBUILD), and creates a `.pkg.tar.xz` package.
    *   `pacman -U` installs the just-built package into the MSYS2 environment. This step effectively performs a system-wide installation, making the binaries, libraries, and data files available for bundling.
5.  **Run Modern Bundle Script**:
    *   Executes `msys2/bundle.sh` (detailed in section 4.3). This script creates a portable `dist/` directory containing the executable, all required DLLs, GTK resources, and a native Windows launcher.
6.  **Create ZIP Archive**:
    *   Zips the contents of `msys2/dist/` into a portable archive: `gtkhash-<sys>-portable.zip`.
7.  **Build Inno Setup Installer (.exe)**:
    *   Switches the shell to `cmd` (Windows Command Prompt) and invokes the Inno Setup 6 compiler (`ISCC.exe`) on the `msys2/gtkhash.iss` script. This produces `gtkhash-installer.exe` inside `msys2/`.
    *   The file is then moved to the repository root as `gtkhash-<sys>-installer.exe` for artifact upload.
8.  **Upload Artifacts**: Both the portable ZIP and the installer are uploaded as a single artifact named `gtkhash-<sys>-packages`.

### 3.3 `releases.yml`

#### Trigger Conditions
*   `on: push` with `tags: - 'v*'` (e.g., `v1.5.1`)
*   `on: workflow_dispatch` with a required input `tag` (string) specifying the version tag to release.

#### Environment & Matrix Configuration
The release workflow orchestrates three job types:

*   **`bump-version`** (Ubuntu latest) – only on manual dispatch.
*   **`source-dist`** (Ubuntu 22.04) – depends on `bump-version`.
*   **`windows-build`** (Windows latest, matrix `mingw64`/`ucrt64`) – depends on `bump-version`.
*   **`create-release`** (Ubuntu latest) – depends on both `source-dist` and `windows-build` success.

#### Step-by-Step Deep Dive

**Job: `bump-version` (only on `workflow_dispatch`)**

This job automates version number updates across the codebase before building the release.

1.  **Checkout**: Fetches the repository at the current ref.
2.  **Determine release version**: Strips the `v` prefix from the tag input to get the plain version (e.g., `1.5.1`).
3.  **Update `meson.build`**: Uses `sed` to replace the `version` field in the `project()` call.
4.  **Update `configure.ac`**: Updates `AC_INIT` version and `RELEASE_DATE` to the current date.
5.  **Update `NEWS`**: Prepends a version header line.
6.  **Update Inno Setup version**: Modifies `AppVersion` in `msys2/gtkhash.iss`.
7.  **Commit and push**: Configures git credentials and pushes the changes to the same branch. It outputs the new commit SHA, which the subsequent build jobs will check out.

**Job: `source-dist`**

1.  **Checkout**: Checks out the commit determined by `bump-version` (if that job ran) or the tagged commit (if triggered by push).
2.  **Determine version**: Extracts the version from the tag.
3.  **Install dependencies**: Installs all build tools and libraries on Ubuntu 22.04.
4.  **Meson setup**: Configures the build with all hash libraries and file manager extensions enabled.
5.  **Create distribution tarballs**: Runs `meson dist` to generate `.tar.gz` and `.tar.xz`.
6.  **Generate SHA256**: Produces checksum files.
7.  **Upload source artifacts**: Uploads the tarballs and checksums as the `source-dist` artifact.

**Job: `windows-build`**

This is functionally identical to the `msys2.yml` workflow, but it also checks out the correct commit (from `bump-version` if needed) and uploads artifacts with a 30-day retention. Artifacts are named `windows-<sys>`.

**Job: `create-release`**

1.  **Checkout**: Fetches the code at the correct commit for release note generation.
2.  **Determine version and tag**: Extracts metadata.
3.  **Download all artifacts**: Gathers the `source-dist` and both `windows-*` artifacts into an `artifacts/` directory.
4.  **Prepare release assets directory**: Copies and renames all files into a flat `release-assets/` directory, with the version inserted into filenames (e.g., `gtkhash-1.5.1-ucrt64-installer.exe`).
5.  **Generate release notes**:
    *   Uses the content of the `NEWS` file as the primary changelog.
    *   Appends a commit log since the previous tag (or all commits if no previous tag exists).
    *   Writes the notes to `release-notes.md`.
6.  **Create release**: Uses the GitHub CLI (`gh`) to create a GitHub Release.
    *   For a tag push, the release is created as a **draft** so the maintainer can review and publish manually.
    *   For a manual dispatch, the release is **published immediately**.
    *   All files from `release-assets/` are attached.
7.  **Flathub manifest update (optional)**:
    *   This step runs only when the `FLATHUB_TOKEN` secret is available in the repository.
    *   **Required secret configuration:** The `FLATHUB_TOKEN` must be added to the repository settings under **Settings → Secrets and variables → Actions**. The secret value must be a **GitHub personal access token** (classic or fine‑grained) that has **write access** to the `flathub/org.gtkhash.gtkhash` repository. If using a classic token, the `repo` scope is sufficient. The workflow uses this token as the password (`https://x-access-token:${FLATHUB_TOKEN}@github.com/…`) to push the updated manifest.
    *   The job clones the Flathub repository, updates the tarball URL and SHA256 in `org.gtkhash.gtkhash.yaml` based on the artifacts produced during the run, commits the change, and pushes it back.
    *   > **Warning:** This Flathub automation step is currently **untested**. It may require manual adjustments to the manifest file path or the update logic once it is activated in production. The maintainer should validate the first automated update manually.
8.  **Warn if Flathub token missing**: If the `FLATHUB_TOKEN` secret is not configured, a warning message is printed, and the workflow continues without failing. The warning explains that the Flathub update was skipped because the token was missing.

## 4. The Windows & MSYS2 Packaging Ecosystem

The Windows build pipeline relies on a tight integration between the `msys2.yml`/`releases.yml` workflows and the files in the `msys2/` directory. Here is how each piece contributes to the final deliverables.

### 4.1 `mingw-w64-gtkhash/PKGBUILD`

This file is an MSYS2 package build script (using the `makepkg-mingw` system) that compiles GtkHash from source and produces a standard MSYS2 package (`.pkg.tar.xz`). Its key roles:

*   **Metadata**: Defines the package base name (`mingw-w64-gtkhash`), versioning (dynamic via `git describe`), dependencies (`mingw-w64-gtk3`, `adwaita-icon-theme`, `libgcrypt`), and build dependencies (`meson`, `pkg-config`).
*   **`build()` function**: Configures Meson with a minimal set of features (`-Dappstream=false`, `-Dblake2=false`) and an optimized `minsize` build type with link-time optimization (`-Db_lto=true`). This yields a small, statically-optimized binary.
*   **`check()` function**: Runs the test suite with a 2-second timeout per test (`-t 2`), ensuring basic functionality.
*   **`package()` function**: Installs the built files into the package staging directory using `meson install --destdir`.

The `makepkg-mingw` command used in the workflow (`PKGEXT=.pkg.tar.xz makepkg-mingw --syncdeps --clean --cleanbuild --force`) ensures all MSYS2 dependencies are installed, the build environment is clean, and a `.pkg.tar.xz` archive is created. The subsequent `pacman -U` step installs this package into the MSYS2 system root (`${MINGW_PREFIX}`), populating directories like `/mingw64/bin`, `/mingw64/share`, etc., exactly as if the user had installed it from the MSYS2 repositories.

### 4.2 `gtkhash-x86_64.install`

This is the MSYS2 install script that runs post-transaction hooks:

*   `post_install()`, `post_upgrade()`, `post_remove()`: All three execute the same commands:
    *   `glib-compile-schemas`: Compiles the GSettings schemas so that GTK can find them.
    *   `gtk-update-icon-cache-3.0`: Updates the GTK icon theme cache to ensure the application icon is properly registered.

These hooks are crucial because the MSYS2 package manager does not automatically compile schemas or icon caches. After the workflow installs the package with `pacman -U`, these hooks are triggered, making the installed GtkHash fully functional within the MSYS2 environment—a prerequisite for the bundling step.

### 4.3 `bundle.sh`

This shell script creates a self-contained, portable directory (`dist/`) that can run independently of the MSYS2 environment. It is executed after the MSYS2 package is installed. The script performs the following operations in sequence:

1.  **Copy main executable**: Copies `gtkhash.exe` from the MSYS2 binary directory into `dist/bin/`. It then uses `objcopy --subsystem windows` to set the subsystem from console to GUI, suppressing the command prompt window when the application is launched.
2.  **Generate Windows icon**: Converts the 256x256 PNG icon (installed by the package) into a native `.ico` file using ImageMagick (`magick` or `convert`). This step is mandatory; the script fails CI if the tool is absent or the source icon is missing.
3.  **Copy GTK loaders/modules**:
    *   Copies `gdk-pixbuf` loaders (`.dll` files) and their cache file into a mirrored subdirectory structure (`lib/gdk-pixbuf-2.0/2.10.0/loaders`).
    *   Patches the cache file to use relative paths (`\lib\...`) instead of absolute MSYS2 paths, ensuring the loaders can be found in the portable tree.
    *   Copies GIO modules (`.dll` files) into `lib/gio/modules/`.
4.  **Resolve and copy all DLL dependencies**: Defines a function `resolve_deps` that iteratively scans all `.exe` and `.dll` files in the `dist/` directory using `ldd`. It identifies dependencies located in the MSYS2 `bin/` directory and copies any missing ones into `dist/bin/`. This repeats until no new DLLs are added, guaranteeing that the portable tree contains every shared library the application needs.
5.  **Copy data files**:
    *   Copies all GSettings XML schema files, then compiles them inside the `dist/` tree using `glib-compile-schemas`.
    *   Recursively copies the entire icon themes from the MSYS2 installation.
    *   Copies only the `gtkhash.mo` translation files (preserving their locale directory structure) from the MSYS2 locale tree.
6.  **Copy license and readme**: Copies `COPYING` and `README.md` from the repository root as `.txt` files into the `dist/` root. This ensures the installer and portable archive contain the necessary legal and informational documents.
7.  **Build native Windows launcher** (`GtkHash.exe`):
    *   Writes a small C program that uses `GetModuleFileNameA` to determine its own location, constructs the path to `bin\gtkhash.exe`, and launches it via `ShellExecuteExA`. This provides a proper GUI process start.
    *   Embeds the `.ico` file as a resource so the launcher has the correct icon.
    *   Compiles the C program and resource into `dist/GtkHash.exe`. This is the executable that the user double-clicks; it resides in the root of the portable tree and the installed directory.

The final `dist/` directory is a complete, portable GtkHash environment.

### 4.4 `gtkhash.iss`

This Inno Setup script defines how the `dist/` directory produced by `bundle.sh` is packaged into a Windows installer (`.exe`). Its settings:

*   **App metadata**: Name, version, default installation directory (`{autopf}\GtkHash`), and Start Menu group.
*   **Icon**: Uses `dist\bin\gtkhash.ico` for the installer itself and for the uninstall display icon.
*   **License**: Includes `dist\COPYING.txt` as the license agreement shown during installation.
*   **Compression**: Uses LZMA2 solid compression for small output size.
*   **Architecture**: 64-bit only (`ArchitecturesAllowed=x64`, `ArchitecturesInstallIn64BitMode=x64`).
*   **Files**: The `[Files]` section recursively copies the entire `dist\` directory into the application folder.
*   **Icons**: Creates a Start Menu shortcut and an optional Desktop shortcut, both pointing to the root `GtkHash.exe` launcher.
*   **Tasks**: Offers a checkbox to create a desktop icon (unchecked by default).
*   **Run**: After installation, offers to launch GtkHash immediately.

In the workflow, the Inno Setup compiler (`ISCC.exe`) reads this script and bundles the `dist/` contents into a single `gtkhash-installer.exe`. The installer is self-contained and does not require MSYS2 to be present on the target system.

## 5. Maintenance & Troubleshooting Guide

### 5.1 Adding a New Linux Distribution to the Compatibility Matrix

To add a new Linux distribution and GTK version to `compatibility-validation.yml`:

1.  **Choose an appropriate container image** from Docker Hub (e.g., `opensuse/leap:latest`). Ensure it provides a recent enough version of the required libraries.
2.  **Determine the package manager and package names**. Compare the existing jobs for a model; you will need equivalents for build tools (`meson`, `ninja`, `gettext`, `appstream`, `pkg-config`), GTK (`gtk3-dev` / `gtk4-dev`), hash libraries, icon themes, Xvfb, and optionally file manager extension packages.
3.  **Insert a new job** in the workflow YAML, following the pattern of an existing job like `debian13-gtk3` or `archlinux-gtk3`.
    *   Set the `needs` field to align with the staged execution order. Place it in the appropriate stage to avoid excessive concurrency (the existing stages are Ubuntu → Debian → Arch → Manjaro → Fedora → dist).
    *   Use the `container:` key to specify your image.
    *   Write the “Install dependencies” step using the correct package manager commands. Note that some distributions require `dbus` to be installed explicitly for GTK4 tests.
    *   Adjust the “Meson setup” options based on what libraries are available (e.g., some distros may not ship `libnautilus-extension` by default).
    *   Ensure the test step uses `xvfb-run` and, for GTK4, sets `--num-processes 1` and runs inside `dbus-run-session`.
4.  **Add a GTK4 variant** if your distribution ships GTK4 and you want to validate the experimental support. Use the “Detect GTK4 support” step to gate the build.
5.  **Update the `dist` job’s `needs`** to include your new job(s) if they should gate the creation of the distribution tarball (current pattern requires all Fedora jobs to succeed).
6.  **Test the change** by pushing to a branch and observing the workflow run.

### 5.2 Updating a Windows Dependency or the MSYS2 Build Environment

The Windows build environment is pinned largely to what `msys2/setup-msys2@v2` installs, but dependencies are listed explicitly:

*   **To upgrade a library version**: Modify the `pacboy` list in `msys2.yml` or the `depends` / `makedepends` arrays in the `PKGBUILD`. The `--syncdeps` flag in `makepkg-mingw` will pull the latest available versions.
*   **To add a new DLL to the portable bundle**: The `bundle.sh` script automatically discovers dependencies via `ldd`. If a new component is required (e.g., a new GTK module), it might be necessary to add a copy step for its specific files (like loaders or modules) in the script. The script already handles the common patterns for GdkPixbuf loaders and GIO modules; adding a new module type would involve extending those sections.
*   **To change Inno Setup version or behavior**: Update the Inno Setup compiler path in the workflow (currently `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`) if a different version is pre-installed on the runners. For script changes, edit `msys2/gtkhash.iss`. Common adjustments include changing the default installation directory, adding file associations, or modifying the Start Menu entries.
*   **Troubleshooting**: If `bundle.sh` fails due to a missing icon or ImageMagick, verify that `adwaita-icon-theme:p` and `imagemagick:p` are in the `pacboy` install list. If the launcher fails to find DLLs, check the `resolve_deps` function’s `ldd` filtering—it currently greps for the MSYS2 binary directory path (`${MINGW_PREFIX}/bin`). Ensure that the `MINGW_PREFIX` environment variable is set correctly by the MSYS2 setup action.

### 5.3 Release Workflow Troubleshooting

*   **Flathub update fails**: This step requires a repository secret named `FLATHUB_TOKEN` that contains a GitHub personal access token with write access to `flathub/org.gtkhash.gtkhash`. The token must be created in your GitHub account settings and added to the repository’s **Settings → Secrets and variables → Actions** as a new secret with the exact name `FLATHUB_TOKEN`. When authenticating, the workflow uses this token as the password in the HTTPS clone URL (`https://x-access-token:${FLATHUB_TOKEN}@github.com/…`). If the token is missing, the update is skipped with a warning. If the step fails even when the token is present, check that the token has the required permissions (classic token with `repo` scope, or a fine‑grained token with read/write access to the Flathub repository) and that the path to the manifest file (`org.gtkhash.gtkhash.yaml`) is still correct inside the cloned repository.  
    > **Warning:** This Flathub automation step is currently **untested**. The maintainer should verify the first run carefully and be prepared to adjust the scripted update logic.

*   **Version bump conflicts**: If the `bump-version` job fails to push, check that the GitHub Actions permissions allow `contents: write`. Also ensure that no branch protection rules prevent the bot from pushing directly.
*   **Missing artifacts**: The `create-release` job requires that both `source-dist` and all `windows-build` jobs succeed (`needs: [source-dist, windows-build]`). If any fail, the release will not be created. Check the logs of the failed job for compilation or test failures.

### 5.4 Modifying Build Options (Hash Libraries, Extensions)

The build configuration in all workflows is controlled by Meson options passed during `meson setup`. To enable or disable a hash library globally, modify the `meson setup` line in each relevant job. The same applies to file manager extensions. The values used reflect the maintainer’s desired test coverage; changes should be tested across all distributions to ensure compatibility.